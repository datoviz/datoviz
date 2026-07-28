"""Focused synchronization tests for the optional Taichi CUDA adapter."""

from __future__ import annotations

from contextlib import contextmanager
from types import SimpleNamespace

import pytest

from datoviz.experimental import cuda


class _FakeStream:
    def __init__(self):
        self.synchronized = 0

    def synchronize(self):
        self.synchronized += 1


class _FakeTorch:
    def __init__(self, stream):
        self.cuda = SimpleNamespace(current_stream=lambda: stream)


class _FakeTaichi:
    def __init__(self):
        self.cuda = object()
        self.cfg = SimpleNamespace(arch=self.cuda)
        self.synchronized = 0

    def sync(self):
        self.synchronized += 1


class _FakeBuffer:
    def __init__(self, stream):
        self._shared = object()
        self.stream = stream

    def _require_open(self):
        return None

    @contextmanager
    def torch_write(self, stream):
        assert stream is self.stream
        yield 'tensor'


def test_taichi_write_orders_both_sides_of_the_kernel(monkeypatch: pytest.MonkeyPatch):
    """The fallback waits before Taichi access and completes Taichi before Vulkan signaling."""
    stream = _FakeStream()
    taichi = _FakeTaichi()
    buffer = _FakeBuffer(stream)
    monkeypatch.setattr(cuda, '_require_torch', lambda: _FakeTorch(stream))
    monkeypatch.setattr(cuda, '_require_taichi', lambda: taichi)

    with cuda.CudaSceneBuffer.taichi_write(buffer) as tensor:
        assert tensor == 'tensor'
        assert stream.synchronized == 1
        assert taichi.synchronized == 0

    assert taichi.synchronized == 1


def test_taichi_requires_the_cuda_backend(monkeypatch: pytest.MonkeyPatch):
    """A non-CUDA Taichi runtime is rejected before sharing the tensor."""
    taichi = SimpleNamespace(cuda=object(), cfg=SimpleNamespace(arch=object()))
    monkeypatch.setattr(cuda.importlib, 'import_module', lambda _name: taichi)
    with pytest.raises(cuda.CudaInteropUnavailable, match='ti.init'):
        cuda._require_taichi()
