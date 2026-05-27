#!/usr/bin/env python3
"""Smoke-test the generated raw ctypes binding."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'


def _smoke_symbols() -> list[str]:
    symbols: list[str] = []
    in_smoke_list = False
    for line in POLICY_PATH.read_text().splitlines():
        stripped = line.strip()
        if stripped == 'smoke_symbols:':
            in_smoke_list = True
            continue
        if in_smoke_list and stripped.startswith('- '):
            symbols.append(stripped[2:].strip())
        elif in_smoke_list and stripped and not stripped.startswith('#'):
            break
    return symbols


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))

    import datoviz as dvz  # noqa: PLC0415
    import datoviz.raw as raw  # noqa: PLC0415

    for symbol in _smoke_symbols():
        assert hasattr(dvz, symbol), f'missing datoviz.{symbol}'
        assert hasattr(raw, symbol), f'missing datoviz.raw.{symbol}'

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
