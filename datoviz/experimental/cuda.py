"""Experimental CUDA/CuPy interop helpers for Datoviz scene buffers."""

from __future__ import annotations

from contextlib import contextmanager
import sys
from pathlib import Path
from typing import Iterator

from datoviz import raw as dvz


ROOT_DIR = Path(__file__).resolve().parents[2]
TOOLS_DIR = ROOT_DIR / 'tools' / 'bindings'
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cupy_interop_runtime as _runtime  # noqa: E402


_SCENE_USAGE_BITS = {
    'vertex': _runtime.DVZ_SCENE_BUFFER_USAGE_VERTEX,
    'storage': _runtime.DVZ_SCENE_BUFFER_USAGE_STORAGE,
}

_RUNTIME_USAGE_BITS = {
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
        self._raw_surface, self._cp, self._bridge = _require_runtime()
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
        return self

    def __exit__(self, exc_type, exc, tb):
        if self._shared is not None:
            try:
                return self._shared.__exit__(exc_type, exc, tb)
            finally:
                self._shared = None
                self._raw_surface = None
                self._cp = None
                self._bridge = None
        return False

    def bind_attr(self, visual, attr: bytes | str, first: int = 0, count: int | None = None) -> None:
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

    def app_resources(self, figure):
        """Return app resources using this buffer's interop GPU context and runtime."""

        self._require_open()
        return self._shared.create_app_resources(figure)

    def offscreen_app(
        self, scene, figure, width: int, height: int, refresh_after_resource_resolution=None
    ):
        """Create an offscreen app using this buffer's interop GPU context and runtime."""

        self._require_open()
        return self._shared.create_offscreen_app(
            scene,
            figure,
            width,
            height,
            refresh_after_resource_resolution=refresh_after_resource_resolution,
        )


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


InteropSkip = _runtime.InteropSkip
require_linux = _runtime.require_linux
require_cupy = _runtime.require_cupy
load_bridge = _runtime.load_bridge
