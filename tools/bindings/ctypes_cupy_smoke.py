#!/usr/bin/env python3
"""Advanced Linux/NVIDIA CuPy interop smoke scaffold.

This is intentionally not a public example. It records the first Python-side smoke target for the
Vulkan-owned buffer -> CUDA/CuPy import route and skips cleanly until the raw advanced interop
ctypes surface is generated.
"""

from __future__ import annotations

import platform
import sys
from pathlib import Path
from typing import NoReturn


ROOT_DIR = Path(__file__).resolve().parents[2]


REQUIRED_RAW_SYMBOLS = (
    'DvzInteropBufferExport',
    'DvzInteropBufferExportConfig',
    'dvz_interop_buffer_export_from_buffer',
    'dvz_buffer_create_wrapper',
    'dvz_buffer_free',
    'dvz_buffer',
    'dvz_buffer_size',
    'dvz_buffer_usage',
    'dvz_buffer_flags',
    'dvz_buffer_create',
    'dvz_buffer_destroy',
    'dvz_semaphore_create_wrapper',
    'dvz_semaphore_free',
    'dvz_semaphore_timeline',
    'dvz_semaphore_destroy',
)


def _skip(reason: str) -> NoReturn:
    print(f'ctypes CuPy interop smoke: SKIP ({reason})')
    raise SystemExit(0)


def _require_linux() -> None:
    if platform.system() != 'Linux':
        _skip('Linux opaque-FD external memory/semaphore path required')


def _require_cupy():
    try:
        import cupy as cp  # noqa: PLC0415
    except Exception as exc:  # pragma: no cover - environment gate
        _skip(f'CuPy unavailable: {exc}')
    try:
        if cp.cuda.runtime.getDeviceCount() <= 0:
            _skip('no CUDA device visible to CuPy')
    except Exception as exc:  # pragma: no cover - environment gate
        _skip(f'CUDA runtime unavailable through CuPy: {exc}')
    return cp


def _require_raw_surface():
    sys.path.insert(0, str(ROOT_DIR))
    try:
        import datoviz.raw as dvz  # noqa: PLC0415
    except Exception as exc:  # pragma: no cover - environment gate
        _skip(f'datoviz.raw unavailable: {exc}')

    missing = [name for name in REQUIRED_RAW_SYMBOLS if not hasattr(dvz, name)]
    if missing:
        _skip('advanced interop ctypes symbols not generated yet: ' + ', '.join(missing[:4]))
    return dvz


def main() -> int:
    _require_linux()
    cp = _require_cupy()
    _require_raw_surface()

    # Next implementation step once the raw surface exists:
    # 1. create a Datoviz/Vulkan exportable vertex|storage DvzBuffer,
    # 2. export it with dvz_interop_buffer_export_from_buffer(), including a timeline semaphore,
    # 3. import the memory/semaphore FDs in a tiny CUDA bridge,
    # 4. wrap the mapped pointer with cupy.cuda.UnownedMemory and cupy.ndarray,
    # 5. write positions with a CuPy kernel, signal CUDA completion, then render through DRP2.
    print(f'ctypes CuPy interop smoke: READY (CuPy {cp.__version__})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
