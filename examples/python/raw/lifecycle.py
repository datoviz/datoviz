#!/usr/bin/env python3
"""Minimal raw ctypes lifecycle example."""

from __future__ import annotations

import datoviz.raw as dvz


def main() -> int:
    t0 = dvz.dvz_time_monotonic_ns()
    t1 = dvz.dvz_time_monotonic_ns()
    assert t1 >= t0

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')
    dvz.dvz_scene_destroy(scene)

    print('raw lifecycle: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
