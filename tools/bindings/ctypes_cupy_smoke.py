#!/usr/bin/env python3
"""Advanced Linux/NVIDIA CuPy interop smoke scaffold.

This is intentionally not a public example. It records the first Python-side smoke target for the
Vulkan-owned buffer -> CUDA/CuPy import route. It validates the raw advanced interop ctypes
surface before gating on local CuPy/CUDA availability.
"""

from __future__ import annotations

import ctypes
import platform
import sys
from pathlib import Path
from typing import NoReturn


ROOT_DIR = Path(__file__).resolve().parents[2]


EXPORT_FIELDS = (
    'version',
    'memory_handle',
    'memory_handle_type',
    'allocation_size',
    'offset',
    'size',
    'usage',
    'vk_usage',
    'drp2_usage',
    'flags',
    'device_uuid_valid',
    'device_uuid',
    'semaphore_handle',
    'semaphore_handle_type',
    'semaphore_value',
)


CONFIG_FIELDS = (
    'offset',
    'size',
    'drp2_usage',
    'flags',
    'semaphore',
    'semaphore_handle_type',
    'semaphore_value',
)


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



def _field_names(record) -> tuple[str, ...]:
    return tuple(name for name, _ctype in record._fields_)


def _validate_raw_surface(dvz) -> None:
    if _field_names(dvz.DvzInteropBufferExport) != EXPORT_FIELDS:
        raise RuntimeError('DvzInteropBufferExport ctypes layout is stale')
    if _field_names(dvz.DvzInteropBufferExportConfig) != CONFIG_FIELDS:
        raise RuntimeError('DvzInteropBufferExportConfig ctypes layout is stale')

    expected_args = [
        ctypes.POINTER(dvz.DvzBuffer),
        ctypes.POINTER(dvz.DvzInteropBufferExportConfig),
        ctypes.POINTER(dvz.DvzInteropBufferExport),
    ]
    fn = dvz.dvz_interop_buffer_export_from_buffer
    if fn.argtypes != expected_args or fn.restype is not ctypes.c_int:
        raise RuntimeError('dvz_interop_buffer_export_from_buffer ctypes signature is stale')


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
    _validate_raw_surface(dvz)
    return dvz


def main() -> int:
    _require_linux()
    _require_raw_surface()
    cp = _require_cupy()

    # Next implementation step:
    # 1. create a Datoviz/Vulkan exportable vertex|storage DvzBuffer,
    # 2. export it with dvz_interop_buffer_export_from_buffer(), including a timeline semaphore,
    # 3. import the memory/semaphore FDs in a tiny CUDA bridge,
    # 4. wrap the mapped pointer with cupy.cuda.UnownedMemory and cupy.ndarray,
    # 5. write positions with a CuPy kernel, signal CUDA completion, then render through DRP2.
    print(f'ctypes CuPy interop smoke: READY (CuPy {cp.__version__})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
