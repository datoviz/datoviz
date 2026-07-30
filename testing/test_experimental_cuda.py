"""Focused package-level checks for the optional CUDA interop provider."""

from __future__ import annotations

import ctypes
from contextlib import contextmanager
from importlib import resources

import pytest

from datoviz.experimental import _cuda_runtime as runtime
from datoviz.experimental import cuda


def test_cuda_provider_imports_without_initializing_cuda() -> None:
    """Importing the experimental API does not require CUDA or CuPy."""

    assert callable(cuda.scene_buffer)
    assert callable(cuda.scene_session)
    assert callable(cuda.image_buffer)


def test_cuda_provider_packages_bridge_source() -> None:
    """The lazy bridge compiler can locate its installed C source."""

    source = resources.files('datoviz.experimental').joinpath('_cuda_interop_bridge.c')
    assert source.is_file()


def test_cuda_raw_surface_matches_generated_interop_layout() -> None:
    """The optional runtime accepts the generated ABI layout before CUDA is requested."""

    from datoviz import raw as dvz

    runtime.validate_raw_surface(dvz)
    assert runtime.field_names(dvz.DvzInteropBufferExportConfig) == runtime.CONFIG_FIELDS


def test_cuda_export_config_initializes_abi_prologue() -> None:
    """Export configuration includes its size and all current ABI-owned fields."""

    from datoviz import raw as dvz

    semaphore = ctypes.pointer(dvz.DvzSemaphore())
    cfg = runtime.interop_buffer_export_config(
        dvz, semaphore, runtime.DVZ_DRP2_BUFFER_USAGE_VERTEX
    )
    assert cfg.struct_size == ctypes.sizeof(dvz.DvzInteropBufferExportConfig)
    assert cfg.flags == 0
    assert cfg.export_flags == 0
    assert ctypes.addressof(cfg.semaphore.contents) == ctypes.addressof(semaphore.contents)
    assert cfg.drp2_usage == runtime.DVZ_DRP2_BUFFER_USAGE_VERTEX


def test_cuda_scene_setup_emit_config_initializes_abi_prologue() -> None:
    """Scene setup emission starts from the native default configuration."""

    from datoviz import raw as dvz

    cfg = runtime.scene_setup_emit_config(dvz)
    assert cfg.struct_size == ctypes.sizeof(dvz.DvzFramePlanEmitConfig)
    assert cfg.flags == 0
    assert cfg.shader_format == runtime.DVZ_SCENE_SHADER_FORMAT_GLSL
    assert cfg.color_target_id == runtime.DRP2_ID_COLOR_TARGET


def test_cuda_borrowed_app_resources_initializes_abi_prologue() -> None:
    """Borrowed app resources start from the native default descriptor."""

    from datoviz import raw as dvz

    resources = runtime.borrowed_app_resources(dvz, None, None)
    assert resources.struct_size == ctypes.sizeof(dvz.DvzAppResources)
    assert resources.flags == 0
    assert not resources.gpu_ctx
    assert not resources.runtime
    assert not resources.window_host


def test_cuda_external_buffer_registration_initializes_abi_prologue() -> None:
    """External buffer registration starts from the native default descriptor."""

    class _Desc(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('buffer', ctypes.c_void_p),
            ('size', ctypes.c_uint64),
            ('usage', ctypes.c_uint32),
        ]

    class _Dvz:
        @staticmethod
        def dvz_drp2_external_buffer_desc():
            return _Desc(struct_size=ctypes.sizeof(_Desc))

        @staticmethod
        def dvz_drp2_runtime_register_external_buffer(_runtime, buffer_id, desc):
            registered = ctypes.cast(desc, ctypes.POINTER(_Desc)).contents
            assert buffer_id == 7
            assert registered.struct_size == ctypes.sizeof(_Desc)
            assert registered.flags == 0
            assert registered.buffer == 123
            assert registered.size == 4096
            assert registered.usage == runtime.DVZ_DRP2_BUFFER_USAGE_VERTEX
            return True

    mapped = runtime.CudaMappedDatovizBuffer.__new__(runtime.CudaMappedDatovizBuffer)
    mapped.dvz = _Dvz()
    mapped.exported = type('_Exported', (), {'buffer': ctypes.c_void_p(123), 'size': 4096})()
    mapped.register_external_buffer('runtime', 7)


def test_cuda_scene_buffer_creation_initializes_abi_prologue() -> None:
    """Scene buffer creation starts from the native default descriptor."""

    class _Desc(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('usage', ctypes.c_uint32),
            ('stride', ctypes.c_uint32),
            ('byte_size', ctypes.c_uint64),
        ]

    class _Dvz:
        @staticmethod
        def dvz_scene_buffer_desc():
            return _Desc(struct_size=ctypes.sizeof(_Desc))

        @staticmethod
        def dvz_scene_buffer(_scene, desc):
            created = ctypes.cast(desc, ctypes.POINTER(_Desc)).contents
            assert created.struct_size == ctypes.sizeof(_Desc)
            assert created.flags == 0
            assert created.usage == runtime.DVZ_SCENE_BUFFER_USAGE_COPY_SRC
            assert created.stride == 4
            assert created.byte_size == 4096
            return 'scene-buffer'

    class _Shared:
        size = 4096

        @staticmethod
        def __enter__():
            return None

        @staticmethod
        def __exit__(_exc_type, _exc, _tb):
            return None

    mapped = runtime.CudaSceneBufferRuntime.__new__(runtime.CudaSceneBufferRuntime)
    mapped.dvz = _Dvz()
    mapped.shared = _Shared()
    mapped.scene = 'scene'
    mapped.scene_usage = runtime.DVZ_SCENE_BUFFER_USAGE_COPY_SRC
    mapped.stride = 4
    mapped.scene_buffer = None
    assert mapped.__enter__() is mapped
    assert mapped.scene_buffer == 'scene-buffer'


def test_cuda_session_translates_optional_runtime_skip(monkeypatch: pytest.MonkeyPatch) -> None:
    """Unavailable optional prerequisites are reported through the public exception."""

    monkeypatch.setattr(runtime, 'require_linux', lambda: runtime.skip('test CUDA unavailable'))
    with pytest.raises(cuda.CudaInteropUnavailable, match='test CUDA unavailable'):
        with cuda.scene_session(object()):
            pass


@pytest.mark.parametrize('shape', ((4, 5), (4, 5, 3), (0, 5, 4)))
def test_cuda_image_buffer_rejects_non_rgba8_shapes(shape) -> None:
    """The image entry point is deliberately narrower than ordinary float scene buffers."""

    with pytest.raises(ValueError, match='height, width, 4'):
        cuda.image_buffer(object(), shape=shape)


def test_cuda_image_buffer_rejects_non_uint8_dtype() -> None:
    """The first image path accepts only tightly packed RGBA8 pixels."""

    with pytest.raises(ValueError, match='uint8'):
        cuda.image_buffer(object(), shape=(4, 5, 4), dtype='float32')


def test_cuda_image_buffer_rejects_non_rgba8_format() -> None:
    """The first image path exposes one explicit texture-copy format."""

    with pytest.raises(ValueError, match='rgba8_unorm'):
        cuda.image_buffer(object(), shape=(4, 5, 4), format='rgba16_float')


def test_cuda_image_buffer_keeps_ordinary_buffer_contract_separate() -> None:
    """Image allocation does not overload the existing two-dimensional float buffer API."""

    ordinary = cuda.scene_buffer(object(), shape=(3, 2))
    image = cuda.image_buffer(object(), shape=(4, 5, 4))
    assert ordinary.shape == (3, 2)
    assert ordinary.dtype == 'float32'
    assert image.shape == (4, 5, 4)
    assert image.image_shape == (4, 5, 4)
    assert image.count == 20
    assert image.components == 4
    assert image.dtype == 'uint8'


def test_cuda_raw_surface_requires_image_handoff_symbols(monkeypatch: pytest.MonkeyPatch) -> None:
    """Image interop refuses generated bindings that omit its timeline handoff contract."""

    from datoviz import raw as dvz

    monkeypatch.delattr(dvz, 'dvz_drp2_runtime_arm_external_buffer_timeline')
    with pytest.raises(runtime.InteropSkip, match='advanced interop ctypes symbols'):
        runtime.require_raw_surface()


def test_cuda_image_timeline_arms_one_pending_copy_and_advances_vulkan_value() -> None:
    """The next CUDA write waits for the signal emitted by the exact image-copy submission."""

    class _Timeline(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('semaphore', ctypes.c_void_p),
            ('wait_value', ctypes.c_uint64),
            ('signal_value', ctypes.c_uint64),
        ]

    class _Dvz:
        DvzDrp2ExternalBufferTimelineDesc = _Timeline

        @staticmethod
        def dvz_drp2_external_buffer_timeline_desc():
            return _Timeline()

        @staticmethod
        def dvz_drp2_runtime_external_buffer_timeline_pending(_runtime, _buffer_id):
            return False

        @staticmethod
        def dvz_drp2_runtime_arm_external_buffer_timeline(_runtime, _buffer_id, desc):
            handoff = ctypes.cast(desc, ctypes.POINTER(_Timeline)).contents
            assert handoff.wait_value == 1
            assert handoff.signal_value == 2
            return True

    mapped = runtime.CudaMappedDatovizBuffer.__new__(runtime.CudaMappedDatovizBuffer)
    mapped.dvz = _Dvz()
    mapped.exported = type('_Exported', (), {'semaphore': ctypes.c_void_p(123)})()
    mapped.cuda_ready_value = 1
    mapped.vulkan_ready_value = 0
    mapped._next_value = 2
    assert mapped.arm_timeline(object(), 42)
    assert mapped.vulkan_ready_value == 2


def test_cuda_image_setup_requires_a_completed_cuda_write() -> None:
    """An image may not create app resources from uninitialized shared memory."""

    image = runtime.CudaImageBufferRuntime.__new__(runtime.CudaImageBufferRuntime)
    image.shared = type('_Shared', (), {'cuda_ready_value': 0})()

    with pytest.raises(RuntimeError, match='write the CUDA image'):
        image._validate_app_resources()


def test_cuda_image_registration_borrows_the_session_runtime() -> None:
    """Image handoff state must not turn a session-owned runtime into a second owned runtime."""

    class _Shared:
        @staticmethod
        def arm_timeline(runtime_, buffer_id):
            assert runtime_ == 'session-runtime'
            assert buffer_id == 7
            return True

    image = runtime.CudaImageBufferRuntime.__new__(runtime.CudaImageBufferRuntime)
    image.shared = _Shared()
    image.runtime = None
    image.handoff_runtime = None
    image.buffer_id = 0
    image._runtime_registered('session-runtime', 7)

    assert image.runtime is None
    assert image.handoff_runtime == 'session-runtime'
    assert image.buffer_id == 7


def test_cuda_image_write_invalidates_and_arms_after_success() -> None:
    """One completed image write dirties the field and arms its exact copy submission."""

    events = []

    class _Dvz:
        @staticmethod
        def dvz_drp2_runtime_external_buffer_timeline_pending(_runtime, _buffer_id):
            return False

        @staticmethod
        def dvz_sampled_field_invalidate(field):
            events.append(('invalidate', field))
            return 0

    class _Shared:
        cuda_ready_value = 0

        @contextmanager
        def cupy_write(self, _stream):
            try:
                yield 'pixels'
            finally:
                self.cuda_ready_value = 1
                events.append('cuda-signal')

        def arm_timeline(self, runtime_, buffer_id):
            events.append(('arm', runtime_, buffer_id, self.cuda_ready_value))
            return True

    image = runtime.CudaImageBufferRuntime.__new__(runtime.CudaImageBufferRuntime)
    image.dvz = _Dvz()
    image.shared = _Shared()
    image.field = 'field'
    image.handoff_runtime = 'runtime'
    image.buffer_id = 7

    with image.cupy_write() as pixels:
        assert pixels == 'pixels'

    assert events == ['cuda-signal', ('invalidate', 'field'), ('arm', 'runtime', 7, 1)]


def test_cuda_image_write_invalidates_and_arms_after_body_exception() -> None:
    """A signaled CUDA scope still schedules its field update when its body raises."""

    events = []

    class _Dvz:
        @staticmethod
        def dvz_drp2_runtime_external_buffer_timeline_pending(_runtime, _buffer_id):
            return False

        @staticmethod
        def dvz_sampled_field_invalidate(field):
            events.append(('invalidate', field))
            return 0

    class _Shared:
        cuda_ready_value = 0

        @contextmanager
        def cupy_write(self, _stream):
            try:
                yield 'pixels'
            finally:
                self.cuda_ready_value = 1
                events.append('cuda-signal')

        def arm_timeline(self, runtime_, buffer_id):
            events.append(('arm', runtime_, buffer_id, self.cuda_ready_value))
            return True

    image = runtime.CudaImageBufferRuntime.__new__(runtime.CudaImageBufferRuntime)
    image.dvz = _Dvz()
    image.shared = _Shared()
    image.field = 'field'
    image.handoff_runtime = 'runtime'
    image.buffer_id = 7

    with pytest.raises(ValueError, match='writer failed'):
        with image.cupy_write():
            raise ValueError('writer failed')

    assert events == ['cuda-signal', ('invalidate', 'field'), ('arm', 'runtime', 7, 1)]


def test_cuda_image_write_rejects_a_pending_prior_transfer() -> None:
    """A second CUDA write cannot overtake the one-shot BufferToTexture handoff."""

    class _Dvz:
        @staticmethod
        def dvz_drp2_runtime_external_buffer_timeline_pending(_runtime, _buffer_id):
            return True

    image = runtime.CudaImageBufferRuntime.__new__(runtime.CudaImageBufferRuntime)
    image.dvz = _Dvz()
    image.handoff_runtime = 'runtime'
    image.buffer_id = 7

    with pytest.raises(RuntimeError, match='previous CUDA image write'):
        image.cupy_write()


def test_cuda_image_timeline_does_not_advance_value_when_native_arm_fails() -> None:
    """A failed arm leaves the next timeline value available for a retry."""

    class _Timeline(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('semaphore', ctypes.c_void_p),
            ('wait_value', ctypes.c_uint64),
            ('signal_value', ctypes.c_uint64),
        ]

    class _Dvz:
        DvzDrp2ExternalBufferTimelineDesc = _Timeline

        @staticmethod
        def dvz_drp2_runtime_external_buffer_timeline_pending(_runtime, _buffer_id):
            return False

        @staticmethod
        def dvz_drp2_external_buffer_timeline_desc():
            return _Timeline()

        @staticmethod
        def dvz_drp2_runtime_arm_external_buffer_timeline(_runtime, _buffer_id, _desc):
            return False

    mapped = runtime.CudaMappedDatovizBuffer.__new__(runtime.CudaMappedDatovizBuffer)
    mapped.dvz = _Dvz()
    mapped.exported = type('_Exported', (), {'semaphore': ctypes.c_void_p(123)})()
    mapped.cuda_ready_value = 1
    mapped.vulkan_ready_value = 0
    mapped._next_value = 2

    with pytest.raises(RuntimeError, match='arm_external_buffer_timeline'):
        mapped.arm_timeline(object(), 42)
    assert mapped._next_value == 2
    assert mapped.vulkan_ready_value == 0
