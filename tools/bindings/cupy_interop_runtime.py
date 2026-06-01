"""Internal CuPy/CUDA interop owner used by raw smoke tests.

This is not a public Datoviz API. It wraps the current Linux/NVIDIA proof path:
Datoviz/Vulkan-owned buffer -> CUDA import -> CuPy array -> explicit timeline sync -> DRP2
external-buffer registration.
"""

from __future__ import annotations

from contextlib import contextmanager
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
DVZ_DRP2_BUFFER_USAGE_VERTEX = 0x0010
DVZ_DRP2_BUFFER_USAGE_STORAGE = 0x0080
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
    'dvz_interop_buffer_wait_timeline',
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
    'DvzDrp2ExternalBufferDesc',
    'dvz_drp2_runtime_vklite_config',
    'dvz_drp2_runtime_vklite',
    'dvz_drp2_runtime_register_external_buffer',
    'dvz_drp2_runtime_execute',
    'dvz_drp2_runtime_download_buffer',
    'dvz_drp2_runtime_destroy',
    'dvz_drp2_stream',
    'dvz_drp2_stream_destroy',
    'dvz_drp2_stream_hello_renderer',
    'dvz_drp2_stream_renderer_hello_reply',
    'dvz_drp2_stream_create_shader_module_format',
    'dvz_drp2_stream_create_render_pipeline_ex2',
    'dvz_drp2_stream_create_texture_2d_usage',
    'dvz_drp2_stream_create_buffer',
    'dvz_drp2_stream_begin_command_encoder',
    'dvz_drp2_stream_begin_render_pass_clear',
    'dvz_drp2_stream_set_pipeline',
    'dvz_drp2_stream_set_vertex_buffer',
    'dvz_drp2_stream_draw',
    'dvz_drp2_stream_end_render_pass',
    'dvz_drp2_stream_copy_texture_to_buffer',
    'dvz_drp2_stream_finish_command_encoder',
    'dvz_drp2_stream_queue_submit',
    'dvz_scene_buffer_resource_key',
    'dvz_drp2_stream_label_id',
)


class InteropSkip(RuntimeError):
    """Environment skip for optional CUDA/CuPy smoke paths."""


class DvzCudaInteropBridge(ctypes.Structure):
    pass


DvzCudaInteropBridgePtr = ctypes.POINTER(DvzCudaInteropBridge)


def skip(reason: str) -> NoReturn:
    raise InteropSkip(reason)


def require_linux() -> None:
    if platform.system() != 'Linux':
        skip('Linux opaque-FD external memory/semaphore path required')


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


def build_bridge() -> Path:
    require_linux()
    paths = _cuda_toolkit_paths()
    if paths is None:
        skip('CUDA Toolkit headers/libcudart not found')
    include_dir, lib_dir = paths
    cc = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc')
    if cc is None:
        skip('C compiler not found for CUDA bridge')

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
        skip(f'CUDA bridge build failed{suffix}')
    return BRIDGE_LIBRARY


def load_bridge():
    path = build_bridge()
    try:
        bridge = ctypes.CDLL(str(path))
    except OSError as exc:
        skip(f'CUDA bridge load failed: {exc}')

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


def require_cupy():
    try:
        import cupy as cp  # noqa: PLC0415
    except Exception as exc:  # pragma: no cover - environment gate
        skip(f'CuPy unavailable: {exc}')
    try:
        if cp.cuda.runtime.getDeviceCount() <= 0:
            skip('no CUDA device visible to CuPy')
    except Exception as exc:  # pragma: no cover - environment gate
        skip(f'CUDA runtime unavailable through CuPy: {exc}')
    return cp


def field_names(record) -> tuple[str, ...]:
    return tuple(name for name, _ctype in record._fields_)


def validate_raw_surface(dvz) -> None:
    if field_names(dvz.DvzInteropBufferExport) != EXPORT_FIELDS:
        raise RuntimeError('DvzInteropBufferExport ctypes layout is stale')
    if field_names(dvz.DvzInteropBufferExportConfig) != CONFIG_FIELDS:
        raise RuntimeError('DvzInteropBufferExportConfig ctypes layout is stale')

    expected_args = [
        ctypes.POINTER(dvz.DvzBuffer),
        ctypes.POINTER(dvz.DvzInteropBufferExportConfig),
        ctypes.POINTER(dvz.DvzInteropBufferExport),
    ]
    fn = dvz.dvz_interop_buffer_export_from_buffer
    if fn.argtypes != expected_args or fn.restype is not ctypes.c_int:
        raise RuntimeError('dvz_interop_buffer_export_from_buffer ctypes signature is stale')

    wait_args = [
        ctypes.POINTER(dvz.DvzDevice),
        ctypes.POINTER(dvz.DvzBuffer),
        ctypes.c_uint64,
        ctypes.POINTER(dvz.DvzSemaphore),
        ctypes.c_uint64,
    ]
    wait_fn = dvz.dvz_interop_buffer_wait_timeline
    if wait_fn.argtypes != wait_args or wait_fn.restype is not ctypes.c_bool:
        raise RuntimeError('dvz_interop_buffer_wait_timeline ctypes signature is stale')


def require_raw_surface():
    sys.path.insert(0, str(ROOT_DIR))
    try:
        import datoviz.raw as dvz  # noqa: PLC0415
    except Exception as exc:  # pragma: no cover - environment gate
        skip(f'datoviz.raw unavailable: {exc}')

    missing = [name for name in REQUIRED_RAW_SYMBOLS if not hasattr(dvz, name)]
    if missing:
        skip('advanced interop ctypes symbols not generated yet: ' + ', '.join(missing[:4]))
    validate_raw_surface(dvz)
    return dvz


def probe_interop_context(dvz) -> None:
    ctx = dvz.dvz_interop_gpu_ctx(0, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
    if not ctx:
        skip('Datoviz interop GPU context unavailable')
    try:
        allocator = dvz.dvz_gpu_ctx_alloc(ctx)
        if not allocator:
            raise RuntimeError('interop GPU context has no allocator')
        external = dvz.dvz_allocator_external(allocator)
        if external != VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
            raise RuntimeError('interop GPU context allocator is not opaque-FD exportable')
    finally:
        dvz.dvz_gpu_ctx_destroy(ctx)


class ExportedDatovizBuffer:
    """Own a Vulkan-exportable Datoviz buffer plus exported handles."""

    def __init__(self, dvz, count: int, components: int = POSITION_COMPONENTS):
        self.dvz = dvz
        self.count = count
        self.components = components
        self.size = count * components * POSITION_DTYPE_SIZE
        self.ctx = None
        self.buffer = None
        self.semaphore = None
        self.desc = dvz.DvzInteropBufferExport()
        self.desc.memory_handle = -1
        self.desc.semaphore_handle = -1

    @property
    def device(self):
        return self.dvz.dvz_gpu_ctx_device(self.ctx) if self.ctx else None

    @property
    def allocator(self):
        return self.dvz.dvz_gpu_ctx_alloc(self.ctx) if self.ctx else None

    def __enter__(self):
        dvz = self.dvz
        self.ctx = dvz.dvz_interop_gpu_ctx(0, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
        if not self.ctx:
            skip('Datoviz interop GPU context unavailable')
        device = self.device
        allocator = self.allocator
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
        cfg.drp2_usage = DVZ_DRP2_BUFFER_USAGE_VERTEX | DVZ_DRP2_BUFFER_USAGE_STORAGE
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
    """Own CUDA external memory/semaphore imports and mapped pointer."""

    def __init__(self, bridge, handle):
        self.bridge = bridge
        self.handle = handle

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


def bridge_import_buffer(bridge, exported: ExportedDatovizBuffer) -> CudaMappedBufferOwner:
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
        skip(f'CUDA bridge import failed: {err}')
    exported.release_exported_handles_to_cuda()
    return CudaMappedBufferOwner(bridge, handle)


class SharedDatovizCudaArray:
    """Internal owner for one CuPy view over a Datoviz/Vulkan-owned buffer."""

    def __init__(self, dvz, cp, bridge, count: int, components: int = POSITION_COMPONENTS):
        self.dvz = dvz
        self.cp = cp
        self.bridge = bridge
        self.exported = ExportedDatovizBuffer(dvz, count=count, components=components)
        self.owner: CudaMappedBufferOwner | None = None
        self.array = None
        self.cuda_ready_value = 0
        self.vulkan_ready_value = 0
        self._next_value = 1

    @property
    def buffer(self):
        return self.exported.buffer

    @property
    def semaphore(self):
        return self.exported.semaphore

    @property
    def size(self) -> int:
        return self.exported.size

    @property
    def count(self) -> int:
        return self.exported.count

    @property
    def device(self):
        return self.exported.device

    @property
    def allocator(self):
        return self.exported.allocator

    def __enter__(self):
        self.exported.__enter__()
        self.owner = bridge_import_buffer(self.bridge, self.exported)
        ptr = self.bridge.dvz_cuda_bridge_ptr(self.owner.handle)
        size = self.bridge.dvz_cuda_bridge_size(self.owner.handle)
        if ptr == 0 or size != self.exported.size:
            self.owner.close()
            raise RuntimeError('CUDA bridge returned an invalid mapped pointer')
        device_id = self.cp.cuda.runtime.getDevice()
        unowned = self.cp.cuda.UnownedMemory(ptr, size, self.owner, device_id=device_id)
        memptr = self.cp.cuda.MemoryPointer(unowned, 0)
        self.array = self.cp.ndarray(
            (self.exported.count, self.exported.components), dtype=self.cp.float32, memptr=memptr
        )
        return self

    @contextmanager
    def cuda_write(self, stream=None):
        if self.owner is None or self.array is None:
            raise RuntimeError('shared CUDA array is not open')
        stream = stream or self.cp.cuda.get_current_stream()
        self.owner.wait(self.vulkan_ready_value, stream.ptr)
        signal_value = self._next_value
        self._next_value += 1
        try:
            yield self.array
        finally:
            self.owner.signal(signal_value, stream.ptr)
            self.cuda_ready_value = signal_value

    def wait_for_cuda_writes(self) -> None:
        if self.cuda_ready_value == 0:
            return
        if not self.dvz.dvz_interop_buffer_wait_timeline(
            self.device, self.buffer, self.size, self.semaphore, self.cuda_ready_value
        ):
            raise RuntimeError('Vulkan wait on CUDA timeline semaphore failed')

    def register_external_buffer(
        self, runtime, buffer_id: int, usage: int = DVZ_DRP2_BUFFER_USAGE_VERTEX
    ) -> None:
        desc = self.dvz.DvzDrp2ExternalBufferDesc()
        desc.buffer = self.buffer
        desc.size = self.size
        desc.usage = usage
        if not self.dvz.dvz_drp2_runtime_register_external_buffer(
            runtime, buffer_id, ctypes.byref(desc)
        ):
            raise RuntimeError('dvz_drp2_runtime_register_external_buffer() failed')

    def register_scene_buffer(
        self, runtime, stream, scene_buffer, usage: int = DVZ_DRP2_BUFFER_USAGE_VERTEX
    ) -> int:
        key = ctypes.create_string_buffer(128)
        if not self.dvz.dvz_scene_buffer_resource_key(scene_buffer, key, len(key)):
            raise RuntimeError('dvz_scene_buffer_resource_key() failed')
        buffer_id = int(self.dvz.dvz_drp2_stream_label_id(stream, key.value))
        if buffer_id == 0:
            raise RuntimeError(f'emitted DRP2 stream has no id for scene buffer {key.value!r}')
        self.register_external_buffer(runtime, buffer_id, usage=usage)
        return buffer_id

    def __exit__(self, exc_type, exc, tb):
        if self.owner is not None:
            self.owner.close()
            self.owner = None
        self.array = None
        return self.exported.__exit__(exc_type, exc, tb)
