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
