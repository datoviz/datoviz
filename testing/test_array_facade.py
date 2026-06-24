"""Tests for the generated top-level array-aware facade."""

from __future__ import annotations

import ctypes
import importlib
import sys
import types
from pathlib import Path

import numpy as np
import pytest


ROOT_DIR = Path(__file__).resolve().parents[1]

pytestmark = pytest.mark.skipif(
    not (ROOT_DIR / 'datoviz' / '_array_facade.py').exists(),
    reason='array facade has not been generated',
)


def _raw_function(argtypes, restype=ctypes.c_int):
    calls = []

    def func(*args):
        calls.append(args)
        return 17

    func.argtypes = argtypes
    func.restype = restype
    func.__doc__ = 'raw function'
    func.calls = calls
    return func


@pytest.fixture()
def fake_facade(monkeypatch):
    raw = types.ModuleType('datoviz.raw')
    raw.DvzVisual = type('DvzVisual', (), {})

    class DvzVisualDataUpdate(ctypes.Structure):
        _fields_ = [
            ('attr_name', ctypes.c_char_p),
            ('data', ctypes.c_void_p),
            ('item_count', ctypes.c_uint32),
        ]

    raw.DvzVisualDataUpdate = DvzVisualDataUpdate
    raw.dvz_passthrough = _raw_function([ctypes.c_int, ctypes.c_int])
    raw.dvz_scene_buffer_set_data = _raw_function(
        [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64], ctypes.c_bool
    )
    raw.dvz_visual_set_data = _raw_function(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_uint32]
    )
    raw.dvz_visual_set_data_many = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(DvzVisualDataUpdate), ctypes.c_uint32]
    )
    raw.dvz_visual_set_data_range = _raw_function(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
    )
    raw.dvz_visual_set_index_data = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
    )
    raw.dvz_view_canvas = _raw_function([ctypes.c_void_p], ctypes.c_void_p)
    raw.dvz_view_framebuffer_size = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)],
        None,
    )
    raw.dvz_canvas_capture_rgba_into = _raw_function(
        [
            ctypes.c_void_p,
            ctypes.c_uint32,
            ctypes.c_uint32,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
        ]
    )

    monkeypatch.setitem(sys.modules, 'datoviz.raw', raw)
    sys.modules.pop('datoviz._array_facade', None)
    import datoviz

    datoviz.raw = raw
    if '_array_facade' in datoviz.__dict__:
        delattr(datoviz, '_array_facade')
    facade = importlib.import_module('datoviz._array_facade')
    yield raw, facade
    sys.modules.pop('datoviz._array_facade', None)


def test_array_facade_exports_raw_names_and_passthrough(fake_facade):
    raw, facade = fake_facade

    assert 'dvz_visual_set_data' in facade.__all__
    assert 'dvz_passthrough' in facade.__all__
    assert facade.DvzVisual is raw.DvzVisual
    assert facade.dvz_passthrough(1, 2) == 17
    assert raw.dvz_passthrough.calls[-1] == (1, 2)


def test_visual_set_data_encodes_string_and_infers_shape0(fake_facade):
    raw, facade = fake_facade
    data = np.arange(6, dtype=np.float32).reshape(3, 2)

    assert facade.dvz_visual_set_data(ctypes.c_void_p(1), 'position', data) == 17

    visual, attr_name, pointer, item_count = raw.dvz_visual_set_data.calls[-1]
    assert visual.value == 1
    assert attr_name == b'position'
    assert pointer.value == data.ctypes.data
    assert item_count == 3


def test_visual_set_data_range_infers_count_after_first_item(fake_facade):
    raw, facade = fake_facade
    data = np.arange(4, dtype=np.float32).reshape(2, 2)

    facade.dvz_visual_set_data_range(ctypes.c_void_p(2), 'position', data, 5)

    _, attr_name, pointer, first_item, item_count = raw.dvz_visual_set_data_range.calls[-1]
    assert attr_name == b'position'
    assert pointer.value == data.ctypes.data
    assert first_item == 5
    assert item_count == 2


def test_visual_set_data_many_accepts_mapping(fake_facade):
    raw, facade = fake_facade
    positions = np.arange(9, dtype=np.float32).reshape(3, 3)
    colors = np.full((3, 4), 255, dtype=np.uint8)

    assert facade.dvz_visual_set_data_many(
        ctypes.c_void_p(7), {'position': positions, 'color': colors}
    ) == 17

    visual, updates, update_count = raw.dvz_visual_set_data_many.calls[-1]
    assert visual.value == 7
    assert update_count == 2
    assert updates[0].attr_name == b'position'
    assert updates[0].data == positions.ctypes.data
    assert updates[0].item_count == 3
    assert updates[1].attr_name == b'color'
    assert updates[1].data == colors.ctypes.data
    assert updates[1].item_count == 3


def test_visual_set_data_many_copies_non_contiguous_inputs(fake_facade):
    raw, facade = fake_facade
    data = np.arange(12, dtype=np.float32).reshape(3, 4)[:, ::2]
    observed = {}

    def inspect_many(_visual, updates, update_count):
        observed['update_count'] = update_count
        observed['pointer'] = updates[0].data
        copied = np.ctypeslib.as_array((ctypes.c_float * data.size).from_address(updates[0].data))
        observed['data'] = copied.reshape(data.shape).copy()
        return 17

    inspect_many.argtypes = raw.dvz_visual_set_data_many.argtypes
    inspect_many.restype = raw.dvz_visual_set_data_many.restype
    inspect_many.__doc__ = raw.dvz_visual_set_data_many.__doc__
    inspect_many.calls = []
    raw.dvz_visual_set_data_many = inspect_many

    facade.dvz_visual_set_data_many(ctypes.c_void_p(8), {'position': data})

    assert observed['update_count'] == 1
    assert observed['pointer'] != data.ctypes.data
    np.testing.assert_array_equal(observed['data'], np.ascontiguousarray(data))


def test_visual_set_data_many_rejects_inconsistent_counts_before_raw_call(fake_facade):
    raw, facade = fake_facade

    with pytest.raises(ValueError, match='item count 2 does not match expected 3'):
        facade.dvz_visual_set_data_many(
            ctypes.c_void_p(9),
            {
                'position': np.zeros((3, 3), dtype=np.float32),
                'color': np.zeros((2, 4), dtype=np.uint8),
            },
        )

    assert raw.dvz_visual_set_data_many.calls == []


def test_visual_set_data_many_raw_passthrough_requires_count(fake_facade):
    raw, facade = fake_facade
    updates = (raw.DvzVisualDataUpdate * 1)()

    with pytest.raises(TypeError, match='update_count must be provided'):
        facade.dvz_visual_set_data_many(ctypes.c_void_p(10), updates)

    facade.dvz_visual_set_data_many(ctypes.c_void_p(10), updates, 1)

    _, raw_updates, update_count = raw.dvz_visual_set_data_many.calls[-1]
    assert raw_updates is updates
    assert update_count == 1


def test_non_contiguous_visual_data_is_copied_during_call(fake_facade):
    raw, facade = fake_facade
    data = np.arange(12, dtype=np.float32).reshape(3, 4)[:, ::2]
    observed = {}

    def inspect_copy(_visual, _attr_name, pointer, item_count):
        observed['item_count'] = item_count
        copied = np.ctypeslib.as_array((ctypes.c_float * data.size).from_address(pointer.value))
        observed['data'] = copied.reshape(data.shape).copy()
        observed['pointer'] = pointer.value
        return 17

    inspect_copy.argtypes = raw.dvz_visual_set_data.argtypes
    inspect_copy.restype = raw.dvz_visual_set_data.restype
    inspect_copy.__doc__ = raw.dvz_visual_set_data.__doc__
    inspect_copy.calls = []
    raw.dvz_visual_set_data = inspect_copy

    facade.dvz_visual_set_data(ctypes.c_void_p(3), 'position', data)

    assert observed['item_count'] == 3
    assert observed['pointer'] != data.ctypes.data
    np.testing.assert_array_equal(observed['data'], np.ascontiguousarray(data))


def test_scene_buffer_set_data_infers_nbytes(fake_facade):
    raw, facade = fake_facade
    data = bytearray(b'abcd')

    facade.dvz_scene_buffer_set_data(ctypes.c_void_p(4), data)

    _, pointer, byte_size = raw.dvz_scene_buffer_set_data.calls[-1]
    assert pointer.value is not None
    assert byte_size == 4


def test_index_data_requires_declared_dtype(fake_facade):
    raw, facade = fake_facade
    indices = np.array([0, 2, 1], dtype=np.uint32)

    facade.dvz_visual_set_index_data(ctypes.c_void_p(5), indices)
    _, pointer, index_count = raw.dvz_visual_set_index_data.calls[-1]
    assert ctypes.cast(pointer, ctypes.c_void_p).value == indices.ctypes.data
    assert index_count == 3

    with pytest.raises(ValueError, match='dtype uint32'):
        facade.dvz_visual_set_index_data(ctypes.c_void_p(5), indices.astype(np.uint16))


def test_view_capture_rgba_returns_numpy_array(fake_facade):
    raw, facade = fake_facade
    canvas = ctypes.c_void_p(123)

    def view_canvas(_view):
        return canvas

    def framebuffer_size(_view, out_width, out_height):
        out_width._obj.value = 3
        out_height._obj.value = 2

    def capture_into(_canvas, width, height, out_rgba, out_size):
        assert _canvas == canvas
        assert width == 3
        assert height == 2
        assert out_size == 24
        data = np.ctypeslib.as_array(out_rgba, shape=(out_size,))
        data[:] = np.arange(out_size, dtype=np.uint8)
        return 0

    raw.dvz_view_canvas = view_canvas
    raw.dvz_view_framebuffer_size = framebuffer_size
    raw.dvz_canvas_capture_rgba_into = capture_into

    rgba = facade.dvz_view_capture_rgba(ctypes.c_void_p(11))

    assert rgba.shape == (2, 3, 4)
    assert rgba.dtype == np.uint8
    np.testing.assert_array_equal(rgba.ravel(), np.arange(24, dtype=np.uint8))


def test_view_capture_rgba_raises_on_missing_canvas(fake_facade):
    raw, facade = fake_facade
    raw.dvz_view_canvas = lambda _view: None

    with pytest.raises(RuntimeError, match='no canvas'):
        facade.dvz_view_capture_rgba(ctypes.c_void_p(12))


def test_view_capture_rgba_raises_on_capture_failure(fake_facade):
    raw, facade = fake_facade
    raw.dvz_view_canvas = lambda _view: ctypes.c_void_p(123)

    def framebuffer_size(_view, out_width, out_height):
        out_width._obj.value = 1
        out_height._obj.value = 1

    raw.dvz_view_framebuffer_size = framebuffer_size
    raw.dvz_canvas_capture_rgba_into = lambda *_args: -7

    with pytest.raises(RuntimeError, match='code -7'):
        facade.dvz_view_capture_rgba(ctypes.c_void_p(13))


def test_top_level_datoviz_resolves_facade_lazily(fake_facade):
    raw, _facade = fake_facade
    import datoviz

    assert datoviz.dvz_visual_set_data(ctypes.c_void_p(6), 'color', np.zeros((1, 4), np.uint8)) == 17
    assert raw.dvz_visual_set_data.calls[-1][1] == b'color'
