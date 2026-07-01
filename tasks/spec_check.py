#!/usr/bin/env python3
"""Run the v0.4 spec and contract checks with stable grouped reporting."""

from __future__ import annotations

import os
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def python_cmd() -> str:
    configured = os.environ.get("DATOVIZ_SPEC_PYTHON")
    if configured:
        return configured
    venv_python = ROOT / ".venv" / "bin" / "python"
    if venv_python.exists():
        return str(venv_python)
    return sys.executable


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--list", action="store_true", help="list checks without running them")
    args = parser.parse_args(argv)

    py = python_cmd()
    checks = [
        ("api status manifest", [py, "tools/check_api_status.py"]),
        ("drp2 fixture runner", [py, "tools/drp2_fixture_runner.py"]),
        ("webgpu fixture preflight", [py, "tools/webgpu_fixture_preflight.py"]),
        ("webgpu runner smoke", ["node", "tools/webgpu_runner_smoke.mjs"]),
        ("drp2 fixture tests", [py, "-m", "pytest", "-q", "testing/test_drp2_fixture_runner.py"]),
        ("webgpu preflight tests", [py, "-m", "pytest", "-q", "testing/test_webgpu_fixture_preflight.py"]),
        ("scheduler tests", [py, "-m", "pytest", "-q", "testing/test_dvztest_scheduler.py"]),
        (
            "scene query source guard",
            [py, "-m", "pytest", "-q", "testing/test_scene_query_source_guard.py"],
        ),
        (
            "scene architecture source guard",
            [py, "-m", "pytest", "-q", "testing/test_scene_architecture_source_guard.py"],
        ),
        ("scene visual boundary guard", [py, "tools/check_scene_visual_boundaries.py"]),
    ]
    if args.list:
        for name, cmd in checks:
            print(f"{name}: {' '.join(cmd)}")
        return 0

    with tempfile.TemporaryDirectory(prefix="datoviz-spec-check-") as tmp:
        procs = []
        for index, (name, cmd) in enumerate(checks):
            log_path = Path(tmp) / f"{index}.log"
            log = log_path.open("w", encoding="utf-8")
            proc = subprocess.Popen(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
            procs.append((name, proc, log, log_path))

        status = 0
        for name, proc, log, log_path in procs:
            rc = proc.wait()
            log.close()
            print(("PASS" if rc == 0 else "FAIL") + f" {name}")
            sys.stdout.write(log_path.read_text(encoding="utf-8"))
            if rc != 0:
                status = 1

    return status


if __name__ == "__main__":
    raise SystemExit(main())
