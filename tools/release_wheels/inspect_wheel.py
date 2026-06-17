#!/usr/bin/env python3
"""Inspect staged Datoviz wheel artifacts."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, os.fspath(ROOT))

from tools.datoviz_build_backend.validate import inspect_wheel, resolve_wheel, validate_wheel  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel")
    parser.add_argument("--native-deps", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wheel = resolve_wheel(args.wheel)
    validate_wheel(wheel)
    inspect_wheel(wheel, native_deps=args.native_deps)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
