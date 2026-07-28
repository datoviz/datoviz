"""Experimental CUDA/CuPy interop helpers for Datoviz scene buffers."""

from __future__ import annotations

import ctypes
import importlib
from contextlib import contextmanager
from typing import Iterator

from datoviz import raw as dvz

from . import _cuda_runtime as _runtime

_SCENE_USAGE_BITS = {
    'vertex': _runtime.DVZ_SCENE_BUFFER_USAGE_VERTEX,
    'storage': _runtime.DVZ_SCENE_BUFFER_USAGE_STORAGE,
    'copy_src': _runtime.DVZ_SCENE_BUFFER_USAGE_COPY_SRC,
}

_RUNTIME_USAGE_BITS = {
    'copy_src': _runtime.DVZ_DRP2_BUFFER_USAGE_COPY_SRC,
    'vertex': _runtime.DVZ_DRP2_BUFFER_USAGE_VERTEX,
    'storage': _runtime.DVZ_DRP2_BUFFER_USAGE_STORAGE,
}


class CudaInteropUnavailable(RuntimeError):
    """Raised when the optional CUDA/CuPy interop path is unavailable."""


def _normalize_dtype(dtype) -> str:
    if dtype in ('float32', 'f4'):
        return 'float32'
    name = getattr(dtype, 'name', None)
    if name == 'float32':
        return 'float32'
    raise ValueError('only float32 CUDA scene buffers are supported by this experimental API')


def _normalize_shape(shape) -> tuple[int, int]:
    try:
        dims = tuple(int(v) for v in shape)
    except TypeError as exc:
        raise ValueError('shape must be a two-item iterable') from exc
    if len(dims) != 2:
        raise ValueError('shape must be a two-item iterable')
    if dims[0] <= 0 or dims[1] <= 0:
        raise ValueError('shape dimensions must be positive')
    return dims


def _normalize_image_shape(shape) -> tuple[int, int, int]:
    try:
        dims = tuple(int(v) for v in shape)
    except TypeError as exc:
        raise ValueError('image shape must be a three-item iterable `(height, width, 4)`') from exc
    if len(dims) != 3 or dims[2] != 4 or dims[0] <= 0 or dims[1] <= 0:
        raise ValueError('image shape must be `(height, width, 4)` with positive dimensions')
    if dims[0] > 0xFFFFFFFF or dims[1] > 0xFFFFFFFF // 4:
        raise ValueError('image dimensions exceed the supported tightly packed RGBA8 range')
    return dims


def _normalize_image_dtype(dtype) -> str:
    if dtype in ('uint8', 'u1') or getattr(dtype, 'name', None) == 'uint8':
        return 'uint8'
    raise ValueError('only uint8 RGBA8 CUDA image buffers are supported by this experimental API')


def _normalize_image_format(format) -> str:
    if str(format).lower() == 'rgba8_unorm':
        return 'rgba8_unorm'
    raise ValueError('only rgba8_unorm CUDA image buffers are supported by this experimental API')


def _usage_bits(usage, table: dict[str, int], kind: str) -> int:
    if isinstance(usage, str):
        names = (usage,)
    else:
        names = tuple(usage)
    if not names:
        raise ValueError('usage must contain at least one usage name')

    bits = 0
    unknown = []
    for name in names:
        try:
            bits |= table[str(name)]
        except KeyError:
            unknown.append(str(name))
    if unknown:
        raise ValueError(f'unsupported {kind} usage: {", ".join(unknown)}')
    return bits


def _usage_names(usage) -> tuple[str, ...]:
    if isinstance(usage, str):
        return (usage,)
    return tuple(usage)


def _require_runtime():
    try:
        _runtime.require_linux()
        raw_surface = _runtime.require_raw_surface()
        cp = _runtime.require_cupy()
        bridge = _runtime.load_bridge()
    except _runtime.InteropSkip as exc:
        raise CudaInteropUnavailable(str(exc)) from exc
    return raw_surface, cp, bridge


def _require_torch():
    try:
        torch = importlib.import_module('torch')
    except Exception as exc:  # pragma: no cover - environment gate
        raise CudaInteropUnavailable(f'PyTorch unavailable: {exc}') from exc
    if not torch.cuda.is_available():
        raise CudaInteropUnavailable('PyTorch CUDA runtime is unavailable')
    return torch


def _require_taichi():
    try:
        taichi = importlib.import_module('taichi')
    except Exception as exc:  # pragma: no cover - environment gate
        raise CudaInteropUnavailable(f'Taichi unavailable: {exc}') from exc
    config = getattr(taichi, 'cfg', None)
    if config is None or getattr(config, 'arch', None) != getattr(taichi, 'cuda', None):
        raise CudaInteropUnavailable('Taichi must be initialized with ti.init(arch=ti.cuda)')
    return taichi


def _torch_stream_device_index(torch, stream) -> int:
    device = getattr(stream, 'device', None)
    index = getattr(device, 'index', None)
    if index is None:
        index = torch.cuda.current_device()
    return int(index)


class CudaSceneBuffer:
    """Context-managed Datoviz scene buffer exposed to CuPy.

    Datoviz owns the GPU allocation and scene buffer. CUDA imports that allocation and exposes a
    CuPy view while the context manager is open. Mutating the buffer must happen inside
    `cupy_write()`, which brackets writes with the current CUDA/Vulkan synchronization contract.
    """

    def __init__(
        self,
        scene,
        *,
        shape,
        dtype='float32',
        usage=('vertex', 'storage'),
        runtime_usage=None,
        present: bool = False,
    ):
        self.scene = scene
        self.shape = _normalize_shape(shape)
        self.dtype = _normalize_dtype(dtype)
        self.usage = _usage_names(usage)
        self.runtime_usage = self.usage if runtime_usage is None else _usage_names(runtime_usage)
        self.present = bool(present)
        self._raw_surface = None
        self._cp = None
        self._bridge = None
        self._shared = None
        self._owns_shared = True
        self._clear_runtime_on_close = True

    @property
    def cupy(self):
        """Return the imported CuPy module after the buffer is opened."""

        if self._cp is None:
            raise RuntimeError('CUDA scene buffer is not open')
        return self._cp

    @property
    def array(self):
        """Return the live CuPy array view."""

        self._require_open()
        return self._shared.array

    @property
    def count(self) -> int:
        return self.shape[0]

    @property
    def components(self) -> int:
        return self.shape[1]

    @property
    def size(self) -> int:
        self._require_open()
        return self._shared.size

    @property
    def device(self):
        self._require_open()
        return self._shared.device

    @property
    def allocator(self):
        self._require_open()
        return self._shared.allocator

    def _require_open(self) -> None:
        if self._shared is None:
            raise RuntimeError('CUDA scene buffer is not open')

    def __enter__(self):
        raw_surface, cp, bridge = _require_runtime()
        self._open_with(raw_surface, cp, bridge, clear_runtime_on_close=True)
        return self

    def _open_with(
        self, raw_surface, cp, bridge, clear_runtime_on_close: bool, session_runtime=None
    ) -> None:
        if self._shared is not None:
            raise RuntimeError('CUDA scene buffer is already open')
        self._raw_surface = raw_surface
        self._cp = cp
        self._bridge = bridge
        self._clear_runtime_on_close = clear_runtime_on_close
        if session_runtime is None:
            self._owns_shared = True
            self._shared = _runtime.CudaSceneBufferRuntime(
                self._raw_surface,
                self._cp,
                self._bridge,
                self.scene,
                count=self.count,
                components=self.components,
                scene_usage=_usage_bits(self.usage, _SCENE_USAGE_BITS, 'scene'),
                runtime_usage=_usage_bits(self.runtime_usage, _RUNTIME_USAGE_BITS, 'runtime'),
                present=self.present,
            )
            self._shared.__enter__()
        else:
            self._owns_shared = False
            self._shared = session_runtime.buffer(
                count=self.count,
                components=self.components,
                scene_usage=_usage_bits(self.usage, _SCENE_USAGE_BITS, 'scene'),
                runtime_usage=_usage_bits(self.runtime_usage, _RUNTIME_USAGE_BITS, 'runtime'),
            )

    def __exit__(self, exc_type, exc, tb):
        return self._close(exc_type, exc, tb)

    def _close(self, exc_type=None, exc=None, tb=None):
        if self._shared is not None:
            try:
                if self._owns_shared:
                    return self._shared.__exit__(exc_type, exc, tb)
                return False
            finally:
                self._shared = None
                self._owns_shared = True
                if self._clear_runtime_on_close:
                    self._raw_surface = None
                    self._cp = None
                    self._bridge = None
        return False

    def bind_attr(
        self, visual, attr: bytes | str, first: int = 0, count: int | None = None
    ) -> None:
        """Bind this buffer to one visual attribute."""

        self._require_open()
        self._shared.bind_attr(visual, attr, first=first, count=count)

    @contextmanager
    def cupy_write(self, stream=None) -> Iterator:
        """Yield a writable CuPy view, then synchronize Datoviz reads.

        The returned view is borrowed. Callers should keep all writes inside this scope so the
        helper can wait for previous Datoviz use before yielding and make CUDA writes visible to
        Datoviz before the next render.
        """

        self._require_open()
        try:
            with self._shared.cupy_write(stream) as array:
                yield array
        finally:
            self._shared.wait_for_cuda_writes()

    def wait_for_writes(self) -> None:
        """Wait until the latest CUDA writes are visible to Datoviz."""

        self._require_open()
        self._shared.wait_for_cuda_writes()

    @contextmanager
    def torch_write(self, stream=None) -> Iterator:
        """Yield a borrowed zero-copy PyTorch tensor on one CUDA stream.

        The selected stream, or PyTorch's current CUDA stream when omitted, is also wrapped as a
        CuPy external stream. Datoviz queues its external-semaphore wait and signal on that same
        stream, so PyTorch writes inside this scope complete before the next Datoviz use.
        """

        self._require_open()
        torch = _require_torch()
        torch_stream = torch.cuda.current_stream() if stream is None else stream
        if not hasattr(torch_stream, 'cuda_stream'):
            raise CudaInteropUnavailable('torch_write() requires a torch.cuda.Stream')
        torch_device = _torch_stream_device_index(torch, torch_stream)
        cupy_device = int(self._cp.cuda.runtime.getDevice())
        if torch_device != cupy_device:
            raise CudaInteropUnavailable(
                f'PyTorch stream device {torch_device} does not match CuPy device {cupy_device}'
            )

        cupy_stream = self._cp.cuda.ExternalStream(torch_stream.cuda_stream, device_id=cupy_device)
        try:
            with self._shared.cupy_write(cupy_stream) as array:
                with torch.cuda.stream(torch_stream):
                    tensor = torch.from_dlpack(array)
                    if int(tensor.data_ptr()) != int(array.data.ptr):
                        raise RuntimeError(
                            'PyTorch DLPack import did not preserve the CUDA pointer'
                        )
                    yield tensor
        finally:
            self._shared.wait_for_cuda_writes()

    @contextmanager
    def taichi_write(self, stream=None) -> Iterator:
        """Yield a borrowed PyTorch tensor for a synchronized Taichi CUDA kernel.

        Taichi does not expose its internal CUDA stream through this contract. This adapter
        therefore synchronizes the incoming PyTorch stream before yielding and calls `ti.sync()`
        before signaling Vulkan. The memory remains zero-copy, but the handoff is intentionally
        CPU-blocking until a native stream-integration proof exists.
        """

        self._require_open()
        taichi = _require_taichi()
        torch = _require_torch()
        torch_stream = torch.cuda.current_stream() if stream is None else stream
        with self.torch_write(torch_stream) as tensor:
            torch_stream.synchronize()
            try:
                yield tensor
            finally:
                taichi.sync()


class CudaSceneImageBuffer(CudaSceneBuffer):
    """A CUDA-written RGBA8 image buffer copied into one retained sampled field."""

    def __init__(
        self, scene, *, shape, dtype='uint8', format='rgba8_unorm', present: bool = False
    ):
        self.image_shape = _normalize_image_shape(shape)
        self.image_dtype = _normalize_image_dtype(dtype)
        self.format = _normalize_image_format(format)
        super().__init__(
            scene,
            shape=(self.image_shape[0] * self.image_shape[1], 4),
            dtype='float32',
            usage=('copy_src',),
            runtime_usage=('copy_src',),
            present=present,
        )
        self.dtype = self.image_dtype
        self.shape = self.image_shape

    @property
    def count(self) -> int:
        return self.image_shape[0] * self.image_shape[1]

    @property
    def components(self) -> int:
        return self.image_shape[2]

    @property
    def height(self) -> int:
        return self.image_shape[0]

    @property
    def width(self) -> int:
        return self.image_shape[1]

    @property
    def channels(self) -> int:
        return self.image_shape[2]

    @property
    def field(self):
        self._require_open()
        return self._shared.field

    @property
    def array(self):
        self._require_open()
        return self._shared.array

    def _open_with(
        self, raw_surface, cp, bridge, clear_runtime_on_close: bool, session_runtime=None
    ) -> None:
        if self._shared is not None:
            raise RuntimeError('CUDA image buffer is already open')
        self._raw_surface = raw_surface
        self._cp = cp
        self._bridge = bridge
        self._clear_runtime_on_close = clear_runtime_on_close
        if session_runtime is None:
            self._owns_shared = True
            self._shared = _runtime.CudaImageBufferRuntime(
                raw_surface, cp, bridge, self.scene, shape=self.image_shape, present=self.present
            )
            self._shared.__enter__()
        else:
            self._owns_shared = False
            self._shared = session_runtime.image_buffer(shape=self.image_shape)

    @contextmanager
    def cupy_write(self, stream=None) -> Iterator:
        self._require_open()
        with self._shared.cupy_write(stream) as array:
            yield array

    @contextmanager
    def torch_write(self, stream=None) -> Iterator:
        self._require_open()
        torch = _require_torch()
        torch_stream = torch.cuda.current_stream() if stream is None else stream
        if not hasattr(torch_stream, 'cuda_stream'):
            raise CudaInteropUnavailable('torch_write() requires a torch.cuda.Stream')
        torch_device = _torch_stream_device_index(torch, torch_stream)
        cupy_device = int(self._cp.cuda.runtime.getDevice())
        if torch_device != cupy_device:
            raise CudaInteropUnavailable(
                f'PyTorch stream device {torch_device} does not match CuPy device {cupy_device}'
            )
        cupy_stream = self._cp.cuda.ExternalStream(torch_stream.cuda_stream, device_id=cupy_device)
        with self._shared.cupy_write(cupy_stream) as array:
            with torch.cuda.stream(torch_stream):
                tensor = torch.from_dlpack(array)
                if int(tensor.data_ptr()) != int(array.data.ptr):
                    raise RuntimeError('PyTorch DLPack import did not preserve the CUDA pointer')
                yield tensor

    @contextmanager
    def taichi_write(self, stream=None) -> Iterator:
        self._require_open()
        taichi = _require_taichi()
        torch = _require_torch()
        torch_stream = torch.cuda.current_stream() if stream is None else stream
        with self.torch_write(torch_stream) as tensor:
            torch_stream.synchronize()
            try:
                yield tensor
            finally:
                taichi.sync()

    def bind_field(self, visual, slot: bytes | str = 'field') -> None:
        """Bind this image's retained sampled field to one visual slot."""

        self._require_open()
        slot_bytes = slot if isinstance(slot, bytes) else slot.encode()
        if self._raw_surface.dvz_visual_set_field(visual, slot_bytes, self.field) != 0:
            raise RuntimeError(f'dvz_visual_set_field({slot!r}) failed')

    def wait_for_writes(self) -> None:
        """Reject the ordinary-buffer wait operation, which is not an image transfer."""

        self._require_open()
        raise RuntimeError('CUDA image writes are consumed by the next Datoviz render')


def scene_buffer(
    scene,
    *,
    shape,
    dtype='float32',
    usage=('vertex', 'storage'),
    runtime_usage=None,
    present: bool = False,
) -> CudaSceneBuffer:
    """Create an experimental Datoviz-owned scene buffer with a CuPy write view."""

    return CudaSceneBuffer(
        scene,
        shape=shape,
        dtype=dtype,
        usage=usage,
        runtime_usage=runtime_usage,
        present=present,
    )


def image_buffer(
    scene, *, shape, dtype='uint8', format='rgba8_unorm', present: bool = False
) -> CudaSceneImageBuffer:
    """Create an experimental CUDA-backed tightly packed RGBA8 sampled image."""

    return CudaSceneImageBuffer(scene, shape=shape, dtype=dtype, format=format, present=present)


class CudaSceneSession:
    """Context-managed CUDA/CuPy scene interop session.

    The session owns the optional CUDA runtime surface for one Datoviz scene and creates
    CudaSceneBuffer resources inside that lifetime. It also centralizes app resource creation so
    examples do not need to route app plumbing through an individual buffer.
    """

    def __init__(self, scene, *, present: bool = False):
        self.scene = scene
        self.present = bool(present)
        self._raw_surface = None
        self._cp = None
        self._bridge = None
        self._session = None
        self._buffers: list[CudaSceneBuffer] = []

    @property
    def cupy(self):
        """Return the imported CuPy module after the session is opened."""

        self._require_open()
        return self._cp

    def _require_open(self) -> None:
        if self._session is None:
            raise RuntimeError('CUDA scene session is not open')

    def __enter__(self):
        self._raw_surface, self._cp, self._bridge = _require_runtime()
        self._session = _runtime.CudaSceneSessionRuntime(
            self._raw_surface, self._cp, self._bridge, self.scene, present=self.present
        )
        self._session.__enter__()
        return self

    def __exit__(self, exc_type, exc, tb):
        ok = False
        for buffer in reversed(self._buffers):
            ok = buffer._close(exc_type, exc, tb) or ok
            buffer._raw_surface = None
            buffer._cp = None
            buffer._bridge = None
        self._buffers.clear()
        if self._session is not None:
            ok = self._session.__exit__(exc_type, exc, tb) or ok
            self._session = None
        self._raw_surface = None
        self._cp = None
        self._bridge = None
        return ok

    def buffer(
        self,
        *,
        shape,
        dtype='float32',
        usage=('vertex', 'storage'),
        runtime_usage=None,
    ) -> CudaSceneBuffer:
        """Create and open one CUDA-backed Datoviz scene buffer in this session."""

        self._require_open()
        buffer = CudaSceneBuffer(
            self.scene,
            shape=shape,
            dtype=dtype,
            usage=usage,
            runtime_usage=runtime_usage,
            present=self.present,
        )
        buffer._open_with(
            self._raw_surface,
            self._cp,
            self._bridge,
            clear_runtime_on_close=False,
            session_runtime=self._session,
        )
        self._buffers.append(buffer)
        return buffer

    def image_buffer(self, *, shape, dtype='uint8', format='rgba8_unorm') -> CudaSceneImageBuffer:
        """Create and open one CUDA-backed RGBA8 image in this session."""

        self._require_open()
        buffer = CudaSceneImageBuffer(
            self.scene,
            shape=shape,
            dtype=dtype,
            format=format,
            present=self.present,
        )
        buffer._open_with(
            self._raw_surface,
            self._cp,
            self._bridge,
            clear_runtime_on_close=False,
            session_runtime=self._session,
        )
        self._buffers.append(buffer)
        return buffer

    def app_resources(self, figure):
        """Return app resources using all CUDA-backed scene buffers in this session."""

        self._require_open()
        if not self._buffers:
            raise RuntimeError('create a CUDA scene buffer before requesting app resources')
        return self._session.create_app_resources(figure)

    def live_app(
        self,
        figure,
        width: int,
        height: int,
        title: bytes | str = b'cuda_scene',
        after_resources=None,
    ):
        """Create a live app/view pair using this session's interop runtime resources."""

        resources = self.app_resources(figure)
        if after_resources is not None:
            after_resources()
        config = dvz.dvz_app_config()
        config.instance_extension_count = 0
        config.instance_extensions = None
        config.enable_canvas_extensions = False
        config.enable_glfw_extensions = False
        config.schedule_mode = dvz.DvzAppScheduleMode.DVZ_APP_SCHEDULE_CONTINUOUS
        app = dvz.dvz_app_with_resources(self.scene, ctypes.byref(config), ctypes.byref(resources))
        if not app:
            raise CudaInteropUnavailable('dvz_app_with_resources() failed')
        title_bytes = title if isinstance(title, bytes) else title.encode()
        view = dvz.dvz_view_glfw(app, figure, width, height, title_bytes)
        if not view:
            dvz.dvz_app_destroy(app)
            raise CudaInteropUnavailable('dvz_view_glfw() failed')
        return app, view

    def offscreen_app(self, figure, width: int, height: int, after_resources=None):
        """Create an offscreen app/view pair using this session's interop runtime resources."""

        resources = self.app_resources(figure)
        if after_resources is not None:
            after_resources()
        config = dvz.dvz_app_config()
        config.instance_extension_count = 0
        config.instance_extensions = None
        config.enable_canvas_extensions = False
        config.enable_glfw_extensions = False
        app = dvz.dvz_app_with_resources(self.scene, ctypes.byref(config), ctypes.byref(resources))
        if not app:
            raise CudaInteropUnavailable('dvz_app_with_resources() failed')
        view = dvz.dvz_view_offscreen(app, figure, width, height)
        if not view:
            dvz.dvz_app_destroy(app)
            raise CudaInteropUnavailable('dvz_view_offscreen() failed')
        return app, view


def scene_session(scene, *, present: bool = False) -> CudaSceneSession:
    """Create an experimental CUDA/CuPy scene interop session."""

    return CudaSceneSession(scene, present=present)


InteropSkip = _runtime.InteropSkip
require_linux = _runtime.require_linux
require_cupy = _runtime.require_cupy
load_bridge = _runtime.load_bridge
