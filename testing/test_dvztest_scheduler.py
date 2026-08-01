#!/usr/bin/env python3
"""Tests for dvztest scheduler sharding and JSON merge behavior."""

from __future__ import annotations

import json
import os
from pathlib import Path
import platform
import subprocess


ROOT_DIR = Path(__file__).resolve().parents[1]


def _runner_path() -> Path:
    suffix = ".exe" if platform.system() == "Windows" else ""
    path = ROOT_DIR / "build" / "testing" / f"dvztest_scheduler{suffix}"
    if not path.exists():
        raise RuntimeError(f"missing scheduler runner: {path}; run `just build` first")
    return path


def _run_scheduler(
    args: list[str], env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(_runner_path()), *args],
        cwd=ROOT_DIR,
        env=env,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def _run_scheduler_failure(
    args: list[str], env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(_runner_path()), *args],
        cwd=ROOT_DIR,
        env=env,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def _load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def test_dvztest_scheduler_parent_jobs_keep_serial_cases_in_serial_phase(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-parent.json"

    completed = _run_scheduler(["--jobs", "3", "--parent-json", str(json_path)])
    data = _load_json(json_path)

    assert "8/8 tests passed" in completed.stdout
    assert "shards 4/4" in completed.stderr
    assert "cases 8/8" in completed.stderr
    assert "fail 0/8 (0.0%)" in completed.stderr
    assert data["summary"]["selected"] == 8
    assert data["summary"]["passed"] == 8
    assert data["summary"]["failed"] == 0

    replay_lines = [
        line
        for line in completed.stdout.splitlines()
        if line.startswith("PASS  scheduler/policy/")
    ]
    assert [line.split()[1].removeprefix("scheduler/policy/") for line in replay_lines] == [
        "parallel-cpu-a",
        "parallel-cpu-b",
        "parallel-process-fixture",
        "process-isolated-child",
        "serial-env",
        "serial-log-capture",
        "serial-exclusive-isolation",
        "serial-exclusive-fixture",
    ]

    cases = data["cases"]
    assert [case["name"] for case in cases] == [
        "parallel-cpu-a",
        "parallel-cpu-b",
        "parallel-process-fixture",
        "process-isolated-child",
        "serial-env",
        "serial-log-capture",
        "serial-exclusive-isolation",
        "serial-exclusive-fixture",
    ]

    parallel = cases[:4]
    serial = cases[4:]
    assert {case["shard_index"] for case in parallel} == {0, 1, 2}
    assert {case["shard_index"] for case in serial} == {0}
    assert all(case["status"] == "PASS" for case in cases)
    assert all(case["order_index"] == index for index, case in enumerate(cases))


def test_dvztest_scheduler_child_parallel_policy_filters_after_sharding(
    tmp_path: Path,
) -> None:
    json_path = tmp_path / "scheduler-child.json"

    completed = _run_scheduler(
        [
            "--shard-index",
            "1",
            "--shard-count",
            "2",
            "--shard-policy",
            "parallel-safe",
            "--child-json",
            str(json_path),
        ]
    )
    data = _load_json(json_path)

    assert completed.stdout == ""
    assert data["summary"]["selected"] == 2
    assert data["summary"]["passed"] == 2
    assert [(case["name"], case["order_index"], case["shard_index"]) for case in data["cases"]] == [
        ("parallel-cpu-b", 1, 1),
        ("process-isolated-child", 3, 1),
    ]


def test_dvztest_scheduler_serial_policy_reports_fixture_setup_time(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-serial.json"

    _run_scheduler(["--shard-policy", "serial-only", "--json", str(json_path)])
    data = _load_json(json_path)

    assert data["summary"]["selected"] == 4
    assert [case["name"] for case in data["cases"]] == [
        "serial-env",
        "serial-log-capture",
        "serial-exclusive-isolation",
        "serial-exclusive-fixture",
    ]
    assert all(case["shard_index"] == 0 for case in data["cases"])

    fixture_case = data["cases"][-1]
    assert fixture_case["fixture"] == "exclusive-fixture"
    assert fixture_case["fixture_scope"] == "exclusive"
    assert fixture_case["fixture_setup_ns"] > 0


def test_dvztest_scheduler_process_isolation_without_jobs_runs_in_child(
    tmp_path: Path,
) -> None:
    json_path = tmp_path / "scheduler-process.json"
    env = os.environ.copy()
    env["DVZTEST_SCHEDULER_REQUIRE_CHILD"] = "1"

    completed = _run_scheduler(["--json", str(json_path)], env=env)
    data = _load_json(json_path)

    assert "8/8 tests passed" in completed.stdout
    assert [case["name"] for case in data["cases"]] == [
        "parallel-cpu-a",
        "parallel-cpu-b",
        "parallel-process-fixture",
        "process-isolated-child",
        "serial-env",
        "serial-log-capture",
        "serial-exclusive-isolation",
        "serial-exclusive-fixture",
    ]

    process_case = data["cases"][3]
    assert process_case["isolation"] == "process"
    assert process_case["status"] == "PASS"
    assert process_case["order_index"] == 3
    assert process_case["repeat_index"] == 0

    exclusive_case = data["cases"][6]
    assert exclusive_case["isolation"] == "exclusive"
    assert exclusive_case["status"] == "PASS"
    assert exclusive_case["order_index"] == 6
    assert exclusive_case["repeat_index"] == 0

    assert data["summary"]["selected"] == 8
    assert data["summary"]["passed"] == 8


def test_dvztest_scheduler_process_isolation_preserves_repeat_order(
    tmp_path: Path,
) -> None:
    json_path = tmp_path / "scheduler-process-repeat.json"
    env = os.environ.copy()
    env["DVZTEST_SCHEDULER_REQUIRE_CHILD"] = "1"

    _run_scheduler(["--repeat", "2", "--json", str(json_path)], env=env)
    data = _load_json(json_path)
    cases = data["cases"]

    assert data["summary"]["selected"] == 16
    assert [(case["repeat_index"], case["order_index"]) for case in cases] == sorted(
        (case["repeat_index"], case["order_index"]) for case in cases
    )

    process_cases = [case for case in cases if case["name"] == "process-isolated-child"]
    assert [(case["repeat_index"], case["order_index"]) for case in process_cases] == [
        (0, 3),
        (1, 3),
    ]
    assert all(case["status"] == "PASS" for case in process_cases)


def test_dvztest_scheduler_process_child_exit_overrides_skip(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-process-child-failure.json"
    env = os.environ.copy()
    env["DVZTEST_SCHEDULER_CHILD_SKIP_THEN_FAIL"] = "1"

    completed = _run_scheduler_failure(
        ["--case", "process-isolated-child", "--json", str(json_path)], env=env
    )
    data = _load_json(json_path)

    assert completed.returncode != 0
    assert "process-isolated case 0 failed with exit code 7" in completed.stderr
    assert data["summary"]["selected"] == 1
    assert data["summary"]["failed"] == 1
    assert data["summary"]["skipped"] == 0
    assert data["cases"][0]["status"] == "FAIL"
    assert data["cases"][0]["skip_reason"] is None


def test_dvztest_scheduler_process_child_preserves_successful_skip(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-process-child-skip.json"
    env = os.environ.copy()
    env["DVZTEST_SCHEDULER_CHILD_SKIP"] = "1"

    completed = _run_scheduler(
        ["--case", "process-isolated-child", "--json", str(json_path)], env=env
    )
    data = _load_json(json_path)

    assert "SKIP  scheduler/policy/process-isolated-child" in completed.stdout
    assert data["summary"]["selected"] == 1
    assert data["summary"]["failed"] == 0
    assert data["summary"]["skipped"] == 1
    assert data["cases"][0]["status"] == "SKIP"
    assert data["cases"][0]["skip_reason"] == "synthetic child skip"


def test_dvztest_scheduler_case_list_filters_by_case_id(tmp_path: Path) -> None:
    case_list = tmp_path / "cases.txt"
    json_path = tmp_path / "scheduler-case-list.json"
    case_list.write_text(
        "\n".join(
            [
                "# comments and blanks are ignored",
                "",
                "scheduler/policy/parallel-cpu-a",
                "scheduler/policy/serial-env",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    _run_scheduler(["--case-list", str(case_list), "--json", str(json_path)])
    data = _load_json(json_path)

    assert data["summary"]["selected"] == 2
    assert [case["case_id"] for case in data["cases"]] == [
        "scheduler/policy/parallel-cpu-a",
        "scheduler/policy/serial-env",
    ]


def test_dvztest_scheduler_case_list_filters_by_display_id(tmp_path: Path) -> None:
    case_list = tmp_path / "display-cases.txt"
    json_path = tmp_path / "scheduler-display-case-list.json"
    case_list.write_text("scheduler/policy/serial-log-capture\n", encoding="utf-8")

    _run_scheduler(["--case-list", str(case_list), "--json", str(json_path)])
    data = _load_json(json_path)

    assert data["summary"]["selected"] == 1
    assert data["cases"][0]["case_id"] == "scheduler/policy/serial-log-capture"


def test_dvztest_scheduler_adapter_token_reaches_context_fixture_and_json(
    tmp_path: Path,
) -> None:
    json_path = tmp_path / "scheduler-adapter.json"

    _run_scheduler(
        [
            "--case",
            "parallel-process-fixture",
            "--scheduler-token",
            "amber",
            "--json",
            str(json_path),
        ]
    )
    data = _load_json(json_path)

    assert data["schema_version"] == 3
    assert data["run"] == {"scheduler_token": "amber", "selected_count": 1}
    assert data["summary"]["selected"] == 1
    assert data["cases"][0]["name"] == "parallel-process-fixture"


def test_dvztest_scheduler_adapter_prepare_follows_case_filter(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-adapter-filter.json"

    _run_scheduler(
        [
            "--case",
            "parallel-cpu-a",
            "--scheduler-token",
            "filtered",
            "--json",
            str(json_path),
        ]
    )
    data = _load_json(json_path)

    assert data["run"] == {"scheduler_token": "filtered", "selected_count": 1}
    assert data["summary"]["selected"] == 1
    assert [case["name"] for case in data["cases"]] == ["parallel-cpu-a"]


def test_dvztest_scheduler_adapter_token_is_forwarded_to_shards(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-adapter-shards.json"

    completed = _run_scheduler(
        ["--jobs", "3", "--scheduler-token", "sharded", "--parent-json", str(json_path)]
    )
    data = _load_json(json_path)

    assert "8/8 tests passed" in completed.stdout
    assert data["run"] == {"scheduler_token": "sharded", "selected_count": 8}
    assert data["summary"]["selected"] == 8
    assert data["summary"]["passed"] == 8


def test_dvztest_scheduler_adapter_rejects_shard_metadata_mismatch(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-adapter-shard-mismatch.json"

    completed = _run_scheduler_failure(
        [
            "--jobs",
            "2",
            "--scheduler-token",
            "root",
            "--scheduler-child-metadata-mismatch",
            "--parent-json",
            str(json_path),
        ]
    )

    assert completed.returncode != 0
    assert "run metadata mismatch" in completed.stderr
    assert "scheduler_token" in completed.stderr
    assert json_path.exists()


def test_dvztest_scheduler_adapter_token_is_forwarded_to_process_child(tmp_path: Path) -> None:
    json_path = tmp_path / "scheduler-adapter-process.json"
    env = os.environ.copy()
    env["DVZTEST_SCHEDULER_REQUIRE_CHILD"] = "1"

    _run_scheduler(
        [
            "--case",
            "process-isolated-child",
            "--scheduler-token",
            "process",
            "--json",
            str(json_path),
        ],
        env=env,
    )
    data = _load_json(json_path)

    assert data["run"] == {"scheduler_token": "process", "selected_count": 1}
    assert data["summary"]["selected"] == 1
    assert data["cases"][0]["status"] == "PASS"


def test_dvztest_scheduler_adapter_rejects_process_child_metadata_mismatch(
    tmp_path: Path,
) -> None:
    json_path = tmp_path / "scheduler-adapter-process-mismatch.json"

    completed = _run_scheduler_failure(
        [
            "--case",
            "process-isolated-child",
            "--scheduler-token",
            "root",
            "--scheduler-child-metadata-mismatch",
            "--json",
            str(json_path),
        ]
    )

    assert completed.returncode != 0
    assert "process-isolated case 0 run metadata mismatch" in completed.stderr
    assert "scheduler_token" in completed.stderr
    assert json_path.exists()
