#!/usr/bin/env python3
"""Install a Datoviz wheel in a clean venv and run installed-package smokes."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, os.fspath(ROOT))

from tools.datoviz_build_backend.validate import (  # noqa: E402
    resolve_wheel,
    run_installed_checks,
    validate_wheel,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel")
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--release-build", action="store_true")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--window", action="store_true")
    parser.add_argument("--precompiled-shaders", action="store_true")
    parser.add_argument("--shaderc", action="store_true")
    parser.add_argument("--cmake-consumer", action="store_true")
    parser.add_argument("--examples", choices=("skip", "basic", "render"), default="skip")
    parser.add_argument("--qt-probe", choices=("skip", "optional", "required"), default="skip")
    parser.add_argument("--keep", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wheel = resolve_wheel(args.wheel).resolve()
    validate_wheel(wheel)
    run_installed_checks(
        wheel,
        work_dir=args.work_dir,
        release_build=args.release_build,
        render=args.render,
        window=args.window,
        precompiled_shaders=args.precompiled_shaders,
        shaderc=args.shaderc,
        cmake_consumer=args.cmake_consumer,
        examples=args.examples,
        qt_probe=args.qt_probe,
        keep=args.keep,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
