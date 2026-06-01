#!/usr/bin/env python3
"""Advanced Linux/NVIDIA CuPy interop smoke scaffold.

This is intentionally not a public example. It records the first Python-side smoke target for the
Vulkan-owned buffer -> CUDA/CuPy import route. It validates the raw advanced interop ctypes
surface before gating on local CuPy/CUDA availability.
"""

from __future__ import annotations

import argparse
import ctypes
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import NoReturn


ROOT_DIR = Path(__file__).resolve().parents[2]
BRIDGE_SOURCE = ROOT_DIR / 'tools' / 'bindings' / 'cuda_interop_bridge.c'
BRIDGE_BUILD_DIR = ROOT_DIR / 'build' / 'bindings'
BRIDGE_LIBRARY = BRIDGE_BUILD_DIR / 'libdatoviz_cuda_interop_bridge.so'


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



class DvzCudaInteropBridge(ctypes.Structure):
    pass


DvzCudaInteropBridgePtr = ctypes.POINTER(DvzCudaInteropBridge)


def _cuda_toolkit_paths() -> tuple[Path, Path] | None:
    roots = []
    for name in ('CUDA_HOME', 'CUDA_PATH'):
        value = os.environ.get(name)
        if value:
            roots.append(Path(value))
    roots.extend([Path('/usr/local/cuda'), Path('/usr/local/cuda-12.8')])

    for root in roots:
        include_candidates = [root / 'include', root / 'targets' / 'x86_64-linux' / 'include']
        lib_candidates = [root / 'lib64', root / 'targets' / 'x86_64-linux' / 'lib']
        for include_dir in include_candidates:
            if not (include_dir / 'cuda_runtime_api.h').exists():
                continue
            for lib_dir in lib_candidates:
                if (lib_dir / 'libcudart.so').exists():
                    return include_dir, lib_dir
    return None


def _build_bridge() -> Path:
    if platform.system() != 'Linux':
        _skip('CUDA bridge build currently targets Linux')
    paths = _cuda_toolkit_paths()
    if paths is None:
        _skip('CUDA Toolkit headers/libcudart not found')
    include_dir, lib_dir = paths
    cc = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc')
    if cc is None:
        _skip('C compiler not found for CUDA bridge')

    BRIDGE_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    if BRIDGE_LIBRARY.exists() and BRIDGE_LIBRARY.stat().st_mtime >= BRIDGE_SOURCE.stat().st_mtime:
        return BRIDGE_LIBRARY

    cmd = [
        cc,
        '-Wall',
        '-Wextra',
        '-Werror',
        '-fPIC',
        '-shared',
        str(BRIDGE_SOURCE),
        '-o',
        str(BRIDGE_LIBRARY),
        f'-I{include_dir}',
        f'-L{lib_dir}',
        '-lcudart',
        f'-Wl,-rpath,{lib_dir}',
    ]
    try:
        subprocess.run(
            cmd, cwd=ROOT_DIR, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as exc:
        detail = exc.stderr.decode(errors='replace').splitlines()[:1]
        suffix = f': {detail[0]}' if detail else ''
        _skip(f'CUDA bridge build failed{suffix}')
    return BRIDGE_LIBRARY


def _load_bridge():
    path = _build_bridge()
    try:
        bridge = ctypes.CDLL(str(path))
    except OSError as exc:
        _skip(f'CUDA bridge load failed: {exc}')

    bridge.dvz_cuda_bridge_last_error.argtypes = []
    bridge.dvz_cuda_bridge_last_error.restype = ctypes.c_char_p
    bridge.dvz_cuda_bridge_import.argtypes = [
        ctypes.c_int,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_int,
        ctypes.POINTER(DvzCudaInteropBridgePtr),
    ]
    bridge.dvz_cuda_bridge_import.restype = ctypes.c_int
    bridge.dvz_cuda_bridge_ptr.argtypes = [DvzCudaInteropBridgePtr]
    bridge.dvz_cuda_bridge_ptr.restype = ctypes.c_uint64
    bridge.dvz_cuda_bridge_size.argtypes = [DvzCudaInteropBridgePtr]
    bridge.dvz_cuda_bridge_size.restype = ctypes.c_uint64
    bridge.dvz_cuda_bridge_wait.argtypes = [
        DvzCudaInteropBridgePtr, ctypes.c_uint64, ctypes.c_uint64
    ]
    bridge.dvz_cuda_bridge_wait.restype = ctypes.c_int
    bridge.dvz_cuda_bridge_signal.argtypes = [
        DvzCudaInteropBridgePtr, ctypes.c_uint64, ctypes.c_uint64
    ]
    bridge.dvz_cuda_bridge_signal.restype = ctypes.c_int
    bridge.dvz_cuda_bridge_destroy.argtypes = [DvzCudaInteropBridgePtr]
    bridge.dvz_cuda_bridge_destroy.restype = None
    return bridge


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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--bridge-only', action='store_true', help='build and load the optional CUDA bridge only'
    )
    args = parser.parse_args()

    _require_linux()
    bridge = _load_bridge() if args.bridge_only else None
    if args.bridge_only:
        assert bridge is not None
        print(f'ctypes CuPy interop bridge: OK ({BRIDGE_LIBRARY})')
        return 0

    _require_raw_surface()
    cp = _require_cupy()
    bridge = _load_bridge()

    # Next implementation step:
    # 1. create a Datoviz/Vulkan exportable vertex|storage DvzBuffer,
    # 2. export it with dvz_interop_buffer_export_from_buffer(), including a timeline semaphore,
    # 3. import the memory/semaphore FDs in a tiny CUDA bridge,
    # 4. wrap the mapped pointer with cupy.cuda.UnownedMemory and cupy.ndarray,
    # 5. write positions with a CuPy kernel, signal CUDA completion, then render through DRP2.
    assert bridge is not None
    print(f'ctypes CuPy interop smoke: READY (CuPy {cp.__version__}, bridge loaded)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
