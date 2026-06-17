#!/usr/bin/env python3
"""Inspect staged Datoviz wheel artifacts."""

from __future__ import annotations

import argparse
import platform
import subprocess
import sys
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def _wheel(path: str | None) -> Path:
    if path:
        return Path(path)
    wheels = sorted((ROOT / "dist").glob("datoviz-*.whl"))
    if not wheels:
        raise FileNotFoundError("no dist/datoviz-*.whl found")
    if len(wheels) > 1:
        raise RuntimeError(f"multiple wheels found; pass --wheel explicitly: {wheels}")
    return wheels[0]


def _run_optional(cmd: list[str]) -> None:
    if not cmd:
        return
    try:
        subprocess.run(cmd, check=False)
    except FileNotFoundError:
        print(f"inspect_wheel: command not found: {cmd[0]}", file=sys.stderr)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel")
    parser.add_argument("--native-deps", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wheel = _wheel(args.wheel)
    print(wheel)
    with zipfile.ZipFile(wheel) as zf:
        for name in sorted(zf.namelist()):
            if name.startswith("datoviz/"):
                print(name)

    if args.native_deps:
        system = platform.system()
        if system == "Linux":
            _run_optional(["auditwheel", "show", str(wheel)])
        elif system == "Darwin":
            _run_optional(["delocate-listdeps", str(wheel)])
        elif system == "Windows":
            _run_optional(["python", "-m", "delvewheel", "show", str(wheel)])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
