#!/usr/bin/env python3
"""Portable wrapper for Datoviz native test and lane execution."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def runner() -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    path = ROOT / "build" / "testing" / f"dvztest{suffix}"
    if not path.exists():
        raise SystemExit(f"missing test runner: {path}; run `just build` first")
    return path


def run(cmd: list[str]) -> int:
    return subprocess.call(cmd, cwd=ROOT)


def inventory(lane: str) -> int:
    out_dir = ROOT / "build" / "testing"
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = [
        sys.executable,
        "tools/test_inventory.py",
        "--output",
        str(out_dir / (f"test_inventory_{lane}.json" if lane else "test_inventory.json")),
        "--markdown",
        str(out_dir / (f"test_inventory_{lane}.md" if lane else "test_inventory.md")),
    ]
    if lane:
        cmd[2:2] = ["--lane", lane]
    return run(cmd)


def lane(lane_name: str, extra: list[str]) -> int:
    out_dir = ROOT / "build" / "testing"
    out_dir.mkdir(parents=True, exist_ok=True)
    case_list = out_dir / f"test_lane_{lane_name}.txt"
    cmd = [
        sys.executable,
        "tools/test_inventory.py",
        "--lane",
        lane_name,
        "--output",
        str(out_dir / f"test_inventory_{lane_name}.json"),
        "--markdown",
        str(out_dir / f"test_inventory_{lane_name}.md"),
        "--case-list",
        str(case_list),
    ]
    rc = run(cmd)
    if rc != 0:
        return rc
    return run([str(runner()), "--case-list", str(case_list), *extra])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)

    run_p = sub.add_parser("run")
    run_p.add_argument("filter", nargs="?", default="")

    inv_p = sub.add_parser("inventory")
    inv_p.add_argument("lane", nargs="?", default="")

    lane_p = sub.add_parser("lane")
    lane_p.add_argument("lane")
    lane_p.add_argument("extra", nargs=argparse.REMAINDER)

    args = parser.parse_args(argv)
    if args.cmd == "run":
        cmd = [str(runner())]
        if args.filter:
            cmd.append(args.filter)
        return run(cmd)
    if args.cmd == "inventory":
        return inventory(args.lane)
    if args.cmd == "lane":
        extra = args.extra
        if extra and extra[0] == "--":
            extra = extra[1:]
        return lane(args.lane, extra)
    raise AssertionError(args.cmd)


if __name__ == "__main__":
    raise SystemExit(main())
