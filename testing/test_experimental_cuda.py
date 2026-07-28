"""Focused package-level checks for the optional CUDA interop provider."""

from __future__ import annotations

import ctypes
from importlib import resources

import pytest

from datoviz.experimental import _cuda_runtime as runtime
from datoviz.experimental import cuda


def test_cuda_provider_imports_without_initializing_cuda() -> None:
    """Importing the experimental API does not require CUDA or CuPy."""

    assert callable(cuda.scene_buffer)
    assert callable(cuda.scene_session)


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


def test_cuda_session_translates_optional_runtime_skip(monkeypatch: pytest.MonkeyPatch) -> None:
    """Unavailable optional prerequisites are reported through the public exception."""

    monkeypatch.setattr(runtime, 'require_linux', lambda: runtime.skip('test CUDA unavailable'))
    with pytest.raises(cuda.CudaInteropUnavailable, match='test CUDA unavailable'):
        with cuda.scene_session(object()):
            pass
