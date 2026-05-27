#!/usr/bin/env python3
"""Smoke-test a tiny raw ctypes offscreen render."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))
    from examples.python.raw.offscreen_point import main as run_example  # noqa: PLC0415

    return run_example([])


if __name__ == '__main__':
    raise SystemExit(main())
