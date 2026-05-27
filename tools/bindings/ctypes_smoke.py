#!/usr/bin/env python3
"""Smoke-test the generated raw ctypes binding."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))

    import datoviz as dvz  # noqa: PLC0415

    t0 = dvz.dvz_time_monotonic_ns()
    t1 = dvz.dvz_time_monotonic_ns()
    assert isinstance(t0, int)
    assert t1 >= t0

    scene = dvz.dvz_scene()
    assert bool(scene)
    dvz.dvz_scene_destroy(scene)

    print('raw ctypes smoke: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
