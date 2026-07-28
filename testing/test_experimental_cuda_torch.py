"""Focused stream-ordering tests for the optional PyTorch CUDA adapter."""

from __future__ import annotations

from contextlib import contextmanager
from types import SimpleNamespace

import pytest

from datoviz.experimental import cuda


class _FakeArray:
    data = SimpleNamespace(ptr=1234)


class _FakeShared:
    def __init__(self):
        self.stream = None
        self.waited = 0

    @contextmanager
    def cupy_write(self, stream):
        self.stream = stream
        yield _FakeArray()

    def wait_for_cuda_writes(self):
        self.waited += 1


class _FakeTorchStream:
    cuda_stream = 4567
    device = SimpleNamespace(index=2)


class _FakeTorchCuda:
    def __init__(self, stream):
        self._stream = stream
        self.active_stream = None

    def is_available(self):
        return True

    def current_stream(self):
        return self._stream

    def current_device(self):
        return 2

    @contextmanager
    def stream(self, stream):
        self.active_stream = stream
        yield


class _FakeTorch:
    def __init__(self, stream):
        self.cuda = _FakeTorchCuda(stream)

    @staticmethod
    def from_dlpack(array):
        return SimpleNamespace(data_ptr=lambda: array.data.ptr)


def _buffer(shared):
    buffer = cuda.CudaSceneBuffer.__new__(cuda.CudaSceneBuffer)
    buffer._shared = shared
    buffer._cp = SimpleNamespace(
        cuda=SimpleNamespace(
            runtime=SimpleNamespace(getDevice=lambda: 2),
            ExternalStream=lambda pointer, device_id: SimpleNamespace(
                ptr=pointer, device_id=device_id
            ),
        )
    )
    return buffer


def test_torch_write_queues_interop_on_the_selected_torch_stream(monkeypatch: pytest.MonkeyPatch):
    """PyTorch writes share the pointer and use the same stream for semaphore ordering."""

    stream = _FakeTorchStream()
    torch = _FakeTorch(stream)
    shared = _FakeShared()
    monkeypatch.setattr(cuda, '_require_torch', lambda: torch)

    with _buffer(shared).torch_write() as tensor:
        assert tensor.data_ptr() == 1234

    assert shared.stream.ptr == stream.cuda_stream
    assert shared.stream.device_id == 2
    assert torch.cuda.active_stream is stream
    assert shared.waited == 1


def test_torch_write_rejects_a_different_cuda_device(monkeypatch: pytest.MonkeyPatch):
    """The adapter refuses streams that cannot address the imported CuPy allocation."""

    stream = _FakeTorchStream()
    stream.device = SimpleNamespace(index=1)
    monkeypatch.setattr(cuda, '_require_torch', lambda: _FakeTorch(stream))

    with pytest.raises(cuda.CudaInteropUnavailable, match='does not match'):
        with _buffer(_FakeShared()).torch_write():
            pass


def test_torch_write_reports_missing_pytorch(monkeypatch: pytest.MonkeyPatch):
    """A missing optional PyTorch dependency is a public availability error."""

    def missing(_name):
        raise ModuleNotFoundError('torch')

    monkeypatch.setattr(cuda.importlib, 'import_module', missing)
    with pytest.raises(cuda.CudaInteropUnavailable, match='PyTorch unavailable'):
        cuda._require_torch()
