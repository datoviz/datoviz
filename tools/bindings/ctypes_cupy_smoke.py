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
VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT = 0x00000001
VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT = 0x00000001
VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001
VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020
VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080
DVZ_ALLOC_DEDICATED_MEMORY = 0x00000001
PARTICLE_COUNT = 1024
POSITION_COMPONENTS = 3
POSITION_DTYPE_SIZE = 4


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
    'dvz_interop_gpu_ctx',
    'dvz_gpu_ctx_alloc',
    'dvz_gpu_ctx_device',
    'dvz_gpu_ctx_destroy',
    'dvz_allocator_external',
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


class ExportedDatovizBuffer:
    def __init__(self, dvz, count: int = PARTICLE_COUNT):
        self.dvz = dvz
        self.count = count
        self.size = count * POSITION_COMPONENTS * POSITION_DTYPE_SIZE
        self.ctx = None
        self.buffer = None
        self.semaphore = None
        self.desc = dvz.DvzInteropBufferExport()
        self.desc.memory_handle = -1
        self.desc.semaphore_handle = -1

    def __enter__(self):
        dvz = self.dvz
        self.ctx = dvz.dvz_interop_gpu_ctx(0, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
        if not self.ctx:
            _skip('Datoviz interop GPU context unavailable')
        device = dvz.dvz_gpu_ctx_device(self.ctx)
        allocator = dvz.dvz_gpu_ctx_alloc(self.ctx)
        if not device or not allocator:
            raise RuntimeError('interop GPU context is missing device or allocator')

        self.buffer = dvz.dvz_buffer_create_wrapper()
        if not self.buffer:
            raise RuntimeError('dvz_buffer_create_wrapper() failed')
        usage = (
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        )
        dvz.dvz_buffer(device, allocator, self.buffer)
        dvz.dvz_buffer_size(self.buffer, self.size)
        dvz.dvz_buffer_usage(self.buffer, usage)
        dvz.dvz_buffer_flags(self.buffer, DVZ_ALLOC_DEDICATED_MEMORY)
        if dvz.dvz_buffer_create(self.buffer) != 0:
            raise RuntimeError('dvz_buffer_create() failed')

        self.semaphore = dvz.dvz_semaphore_create_wrapper()
        if not self.semaphore:
            raise RuntimeError('dvz_semaphore_create_wrapper() failed')
        dvz.dvz_semaphore_timeline(
            device, 0, self.semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
        )

        cfg = dvz.DvzInteropBufferExportConfig()
        cfg.offset = 0
        cfg.size = 0
        cfg.drp2_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        cfg.flags = 0
        cfg.semaphore = self.semaphore
        cfg.semaphore_handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
        cfg.semaphore_value = 0
        if dvz.dvz_interop_buffer_export_from_buffer(
            self.buffer, ctypes.byref(cfg), ctypes.byref(self.desc)
        ) != 0:
            raise RuntimeError('dvz_interop_buffer_export_from_buffer() failed')
        if self.desc.memory_handle < 0 or self.desc.semaphore_handle < 0:
            raise RuntimeError('interop export did not return memory and semaphore handles')
        if self.desc.size != self.size:
            raise RuntimeError('interop export returned an unexpected logical buffer size')
        return self

    def release_exported_handles_to_cuda(self) -> None:
        self.desc.memory_handle = -1
        self.desc.semaphore_handle = -1

    def __exit__(self, _exc_type, _exc, _tb):
        if self.desc.memory_handle >= 0:
            os.close(self.desc.memory_handle)
            self.desc.memory_handle = -1
        if self.desc.semaphore_handle >= 0:
            os.close(self.desc.semaphore_handle)
            self.desc.semaphore_handle = -1
        if self.buffer is not None:
            self.dvz.dvz_buffer_destroy(self.buffer)
            self.dvz.dvz_buffer_free(self.buffer)
            self.buffer = None
        if self.semaphore is not None:
            self.dvz.dvz_semaphore_destroy(self.semaphore)
            self.dvz.dvz_semaphore_free(self.semaphore)
            self.semaphore = None
        if self.ctx is not None:
            self.dvz.dvz_gpu_ctx_destroy(self.ctx)
            self.ctx = None
        return False


class CudaMappedBufferOwner:
    def __init__(self, bridge, handle, exported: ExportedDatovizBuffer):
        self.bridge = bridge
        self.handle = handle
        self.exported = exported

    def wait(self, value: int, stream_ptr: int) -> None:
        if self.bridge.dvz_cuda_bridge_wait(self.handle, value, stream_ptr) != 0:
            err = self.bridge.dvz_cuda_bridge_last_error().decode(errors='replace')
            raise RuntimeError(f'CUDA external semaphore wait failed: {err}')

    def signal(self, value: int, stream_ptr: int) -> None:
        if self.bridge.dvz_cuda_bridge_signal(self.handle, value, stream_ptr) != 0:
            err = self.bridge.dvz_cuda_bridge_last_error().decode(errors='replace')
            raise RuntimeError(f'CUDA external semaphore signal failed: {err}')

    def close(self) -> None:
        if self.handle:
            self.bridge.dvz_cuda_bridge_destroy(self.handle)
            self.handle = DvzCudaInteropBridgePtr()

    def __del__(self):
        self.close()


def _bridge_import_buffer(bridge, exported: ExportedDatovizBuffer) -> CudaMappedBufferOwner:
    handle = DvzCudaInteropBridgePtr()
    rc = bridge.dvz_cuda_bridge_import(
        exported.desc.memory_handle,
        exported.desc.allocation_size,
        exported.desc.offset,
        exported.desc.size,
        exported.desc.semaphore_handle,
        ctypes.byref(handle),
    )
    if rc != 0 or not handle:
        err = bridge.dvz_cuda_bridge_last_error().decode(errors='replace')
        _skip(f'CUDA bridge import failed: {err}')
    exported.release_exported_handles_to_cuda()
    return CudaMappedBufferOwner(bridge, handle, exported)


def _cupy_array_from_export(cp, bridge, exported: ExportedDatovizBuffer):
    owner = _bridge_import_buffer(bridge, exported)
    ptr = bridge.dvz_cuda_bridge_ptr(owner.handle)
    size = bridge.dvz_cuda_bridge_size(owner.handle)
    if ptr == 0 or size != exported.size:
        owner.close()
        raise RuntimeError('CUDA bridge returned an invalid mapped pointer')
    device_id = cp.cuda.runtime.getDevice()
    unowned = cp.cuda.UnownedMemory(ptr, size, owner, device_id=device_id)
    memptr = cp.cuda.MemoryPointer(unowned, 0)
    array = cp.ndarray((exported.count, POSITION_COMPONENTS), dtype=cp.float32, memptr=memptr)
    return array, owner


def _run_cupy_write_smoke(dvz, cp, bridge) -> None:
    with ExportedDatovizBuffer(dvz) as exported:
        array, owner = _cupy_array_from_export(cp, bridge, exported)
        stream = cp.cuda.get_current_stream()
        owner.wait(0, stream.ptr)
        t = cp.linspace(-1.0, 1.0, exported.count, dtype=cp.float32)
        array[:, 0] = t
        array[:, 1] = cp.sin(t * cp.float32(6.283185307179586))
        array[:, 2] = 0
        owner.signal(1, stream.ptr)
        stream.synchronize()
        owner.close()


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



def _probe_interop_context(dvz) -> None:
    ctx = dvz.dvz_interop_gpu_ctx(0, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
    if not ctx:
        _skip('Datoviz interop GPU context unavailable')
    try:
        allocator = dvz.dvz_gpu_ctx_alloc(ctx)
        if not allocator:
            raise RuntimeError('interop GPU context has no allocator')
        external = dvz.dvz_allocator_external(allocator)
        if external != VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
            raise RuntimeError('interop GPU context allocator is not opaque-FD exportable')
    finally:
        dvz.dvz_gpu_ctx_destroy(ctx)


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
    parser.add_argument(
        '--ctx-only',
        action='store_true',
        help='create and destroy an exportable Datoviz GPU context',
    )
    parser.add_argument(
        '--export-only', action='store_true', help='create and export a Datoviz buffer only'
    )
    args = parser.parse_args()

    _require_linux()
    bridge = _load_bridge() if args.bridge_only else None
    if args.bridge_only:
        assert bridge is not None
        print(f'ctypes CuPy interop bridge: OK ({BRIDGE_LIBRARY})')
        return 0

    dvz = _require_raw_surface()
    if args.ctx_only:
        _probe_interop_context(dvz)
        print('ctypes CuPy interop context: OK')
        return 0
    if args.export_only:
        with ExportedDatovizBuffer(dvz) as exported:
            print(
                'ctypes CuPy interop export: OK '
                f'(size={exported.desc.size}, memory_fd={exported.desc.memory_handle}, '
                f'semaphore_fd={exported.desc.semaphore_handle})'
            )
        return 0

    cp = _require_cupy()
    bridge = _load_bridge()
    assert bridge is not None
    _run_cupy_write_smoke(dvz, cp, bridge)
    print(f'ctypes CuPy interop smoke: READY (CuPy {cp.__version__}, zero-copy write path)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
