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
    raw.dvz_passthrough = _raw_function([ctypes.c_int, ctypes.c_int])
    raw.dvz_scene_buffer_set_data = _raw_function(
        [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64], ctypes.c_bool
    )
    raw.dvz_visual_set_data = _raw_function(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_uint32]
    )
    raw.dvz_visual_set_data_range = _raw_function(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
    )
    raw.dvz_visual_set_index_data = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
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


def test_top_level_datoviz_resolves_facade_lazily(fake_facade):
    raw, _facade = fake_facade
    import datoviz

    assert datoviz.dvz_visual_set_data(ctypes.c_void_p(6), 'color', np.zeros((1, 4), np.uint8)) == 17
    assert raw.dvz_visual_set_data.calls[-1][1] == b'color'
