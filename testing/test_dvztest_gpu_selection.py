#!/usr/bin/env python3
"""Tests for Datoviz test-runner GPU selection."""

from __future__ import annotations

import json
import os
from pathlib import Path
import platform
import subprocess

import pytest


ROOT_DIR = Path(__file__).resolve().parents[1]


def _runner_path(name: str = "dvztest") -> Path:
    suffix = ".exe" if platform.system() == "Windows" else ""
    path = ROOT_DIR / "build" / "testing" / f"{name}{suffix}"
    if not path.exists():
        raise RuntimeError(f"missing GPU test runner: {path}; run `just build` first")
    return path


def _run(
    args: list[str],
    *,
    env: dict[str, str] | None = None,
    check: bool = False,
    runner: str = "dvztest",
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(_runner_path(runner)), *args],
        cwd=ROOT_DIR,
        env=env,
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def _load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


@pytest.mark.parametrize(
    "args",
    [
        ["--gpu"],
        ["--gpu", ""],
        ["--gpu", "-1"],
        ["--gpu", "+1"],
        ["--gpu", " 1"],
        ["--gpu", "1 "],
        ["--gpu", "1x"],
        ["--gpu", "4294967296"],
        ["--gpu=1"],
        ["--gpu", "--case", "test_gpu_props"],
    ],
)
def test_dvztest_gpu_rejects_invalid_cli_selectors(args: list[str]) -> None:
    completed = _run(args)

    assert completed.returncode != 0
    assert "gpu" in completed.stderr.lower()


@pytest.mark.parametrize("value", ["", "-1", "+1", " 1", "1 ", "1x", "4294967296"])
def test_dvztest_gpu_rejects_invalid_environment_selectors(value: str) -> None:
    env = os.environ.copy()
    env["DVZ_TEST_GPU"] = value

    completed = _run(["--module", "common", "--group", "alloc", "--case", "basic"], env=env)

    assert completed.returncode != 0
    assert "DVZ_TEST_GPU" in completed.stderr


def test_dvztest_gpu_listing_modes_do_not_consume_environment_selection() -> None:
    env = os.environ.copy()
    env["DVZ_TEST_GPU"] = "invalid"

    completed = _run(["--list", "--module", "common"], env=env)

    assert completed.returncode == 0
    assert "common/" in completed.stdout
    assert "GPU " not in completed.stdout


def test_dvztest_gpu_cpu_only_json_is_null(tmp_path: Path) -> None:
    json_path = tmp_path / "cpu-only.json"

    completed = _run(
        [
            "--module",
            "common",
            "--group",
            "alloc",
            "--case",
            "test_alloc_basic",
            "--json",
            str(json_path),
        ]
    )
    data = _load_json(json_path)

    assert completed.returncode == 0
    assert "GPU " not in completed.stdout
    assert data["schema_version"] == 3
    assert data["run"] == {"gpu": None}
    assert data["summary"]["selected"] == 1


def test_dvztest_list_gpus_is_exclusive() -> None:
    completed = _run(["--list-gpus", "--list"])

    assert completed.returncode != 0
    assert "exclusive" in completed.stderr


def test_dvztest_gpu_cli_is_rejected_in_listing_mode() -> None:
    completed = _run(["--gpu", "0", "--list"])

    assert completed.returncode != 0
    assert "cannot be combined" in completed.stderr


def test_dvztest_gpu_help_documents_cli_and_environment() -> None:
    completed = _run(["--help"], runner="dvztest_vk")

    assert completed.returncode == 0
    assert "--gpu index" in completed.stdout
    assert "--list-gpus" in completed.stdout
    assert "DVZ_TEST_GPU" in completed.stdout


def test_dvztest_gpu_discovery_and_selected_property_identity(tmp_path: Path) -> None:
    discovery = _run(["--list-gpus"])
    if discovery.returncode != 0:
        pytest.skip("Vulkan discovery is unavailable on this host")
    assert "[0]" in discovery.stdout
    assert "api_version_raw=" in discovery.stdout
    assert "driver_version_raw=" in discovery.stdout

    json_path = tmp_path / "gpu-selected.json"
    env = os.environ.copy()
    env["DVZ_TEST_GPU"] = "invalid"
    selected = _run(
        ["--gpu", "0", "--case", "test_gpu_props", "--json", str(json_path)],
        env=env,
        runner="dvztest_vk",
    )
    data = _load_json(json_path)

    assert selected.returncode == 0, selected.stderr
    assert data["summary"]["selected"] == 1
    assert data["summary"]["passed"] == 1
    assert data["run"]["gpu"]["requested_index"] == 0
    assert data["run"]["gpu"]["resolved_index"] == 0
    assert data["run"]["gpu"]["selection_source"] == "cli"
    assert data["run"]["gpu"]["name"] in selected.stdout

    default_path = tmp_path / "gpu-default.json"
    default = _run(
        ["--case", "test_gpu_props", "--json", str(default_path)], runner="dvztest_vk"
    )
    default_data = _load_json(default_path)

    assert default.returncode == 0, default.stderr
    assert default_data["run"]["gpu"]["requested_index"] == 0
    assert default_data["run"]["gpu"]["selection_source"] == "default"

    sharded_path = tmp_path / "gpu-env-sharded.json"
    sharded_env = os.environ.copy()
    sharded_env["DVZ_TEST_GPU"] = "0"
    sharded = _run(
        [
            "--case",
            "test_gpu_props",
            "--jobs",
            "2",
            "--parent-json",
            str(sharded_path),
        ],
        env=sharded_env,
        runner="dvztest_vk",
    )
    sharded_data = _load_json(sharded_path)

    assert sharded.returncode == 0, sharded.stderr
    assert sharded_data["summary"]["passed"] == 1
    assert sharded_data["run"]["gpu"]["selection_source"] == "env"


def test_dvztest_gpu_unavailable_index_prints_discovery_evidence() -> None:
    discovery = _run(["--list-gpus"])
    if discovery.returncode != 0:
        pytest.skip("Vulkan discovery is unavailable on this host")

    completed = _run(
        ["--gpu", "4294967295", "--case", "test_gpu_props"], runner="dvztest_vk"
    )

    assert completed.returncode != 0
    assert "unavailable" in completed.stderr
    assert "[0]" in completed.stdout


def test_dvztest_gpu_migrated_vk_case_joins_campaign() -> None:
    completed = _run(["--gpu", "0", "--case", "test_gpu_memprops"], runner="dvztest_vk")

    assert completed.returncode == 0, completed.stderr
    assert "1/1 tests passed" in completed.stdout


@pytest.mark.parametrize(
    "args",
    [
        ["--gpu"],
        ["--gpu", ""],
        ["--gpu", "-1"],
        ["--gpu", "+1"],
        ["--gpu", " 1"],
        ["--gpu", "1 "],
        ["--gpu", "1x"],
        ["--gpu", "4294967296"],
        ["--gpu=1"],
        ["--gpu", "0", "--gpu", "1"],
    ],
)
def test_dvz_live_canvas_rejects_invalid_gpu_selectors(args: list[str]) -> None:
    completed = _run(args, runner="dvz_live_canvas")

    assert completed.returncode != 0
    assert "gpu" in completed.stderr.lower()


def test_dvz_live_canvas_gpu_precedence_and_environment() -> None:
    discovery = _run(["--list-gpus"])
    if discovery.returncode != 0:
        pytest.skip("Vulkan discovery is unavailable on this host")

    invalid_env = os.environ.copy()
    invalid_env["DVZ_TEST_GPU"] = "invalid"
    cli = _run(
        ["--backend", "offscreen", "--frames", "1", "--gpu", "0"],
        env=invalid_env,
        runner="dvz_live_canvas",
    )
    assert cli.returncode == 0, cli.stderr
    assert "GPU 0:" in cli.stderr
    assert "source=cli" in cli.stderr

    selected_env = os.environ.copy()
    selected_env["DVZ_TEST_GPU"] = "0"
    env_run = _run(
        ["--backend", "offscreen", "--frames", "1"],
        env=selected_env,
        runner="dvz_live_canvas",
    )
    assert env_run.returncode == 0, env_run.stderr
    assert "GPU 0:" in env_run.stderr
    assert "source=env" in env_run.stderr
