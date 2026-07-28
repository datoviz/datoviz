"""Focused package-level checks for the optional CUDA interop provider."""

from __future__ import annotations

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


def test_cuda_session_translates_optional_runtime_skip(monkeypatch: pytest.MonkeyPatch) -> None:
    """Unavailable optional prerequisites are reported through the public exception."""

    monkeypatch.setattr(runtime, 'require_linux', lambda: runtime.skip('test CUDA unavailable'))
    with pytest.raises(cuda.CudaInteropUnavailable, match='test CUDA unavailable'):
        with cuda.scene_session(object()):
            pass
