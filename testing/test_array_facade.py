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
    raw.DvzSizeSpace = type('DvzSizeSpace', (), {'DVZ_SIZE_LOGICAL': 0})
    raw.DVZ_SIZE_LOGICAL = raw.DvzSizeSpace.DVZ_SIZE_LOGICAL

    class DvzAxisTicks(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('count', ctypes.c_uint32),
            ('values', ctypes.POINTER(ctypes.c_double)),
            ('labels', ctypes.POINTER(ctypes.c_char_p)),
        ]

    class DvzColorbarTicks(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('count', ctypes.c_uint32),
            ('values', ctypes.POINTER(ctypes.c_double)),
            ('labels', ctypes.POINTER(ctypes.c_char_p)),
        ]

    class DvzVisualDataUpdate(ctypes.Structure):
        _fields_ = [
            ('attr_name', ctypes.c_char_p),
            ('data', ctypes.c_void_p),
            ('item_count', ctypes.c_uint32),
        ]

    class DvzSampledFieldDesc(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('dim', ctypes.c_int),
            ('format', ctypes.c_int),
            ('semantic', ctypes.c_int),
            ('color_role', ctypes.c_int),
            ('width', ctypes.c_uint32),
            ('height', ctypes.c_uint32),
            ('depth', ctypes.c_uint32),
        ]

    class DvzFieldDataView(ctypes.Structure):
        _fields_ = [
            ('struct_size', ctypes.c_uint32),
            ('flags', ctypes.c_uint32),
            ('data', ctypes.c_void_p),
            ('bytes_per_row', ctypes.c_uint64),
            ('rows_per_image', ctypes.c_uint64),
        ]

    class DvzFieldRegion(ctypes.Structure):
        _fields_ = [
            ('x', ctypes.c_uint32),
            ('y', ctypes.c_uint32),
            ('z', ctypes.c_uint32),
            ('width', ctypes.c_uint32),
            ('height', ctypes.c_uint32),
            ('depth', ctypes.c_uint32),
        ]

    raw.DvzAxisTicks = DvzAxisTicks
    raw.DvzColorbarTicks = DvzColorbarTicks
    raw.DvzVisualDataUpdate = DvzVisualDataUpdate
    raw.DvzSampledFieldDesc = DvzSampledFieldDesc
    raw.DvzFieldDataView = DvzFieldDataView
    raw.DvzFieldRegion = DvzFieldRegion
    raw.DVZ_FIELD_DIM_2D = 0
    raw.DVZ_FIELD_DIM_3D = 1
    raw.DVZ_FIELD_FORMAT_R8_UNORM = 0
    raw.DVZ_FIELD_FORMAT_R32_UINT = 9
    raw.DVZ_FIELD_FORMAT_R32_FLOAT = 11
    raw.DVZ_FIELD_FORMAT_RGBA8_UNORM = 22
    raw.DVZ_FIELD_SEMANTIC_SCALAR = 1
    raw.DVZ_FIELD_SEMANTIC_COLOR = 4
    raw.DVZ_FIELD_SEMANTIC_LABEL = 5
    raw.DVZ_COLOR_ROLE_NONE = 0
    raw.DVZ_COLOR_ROLE_SRGB_COLOR = 1
    raw.DVZ_COLOR_ROLE_DATA = 3
    raw.dvz_axis_set_ticks = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(DvzAxisTicks)], ctypes.c_bool
    )
    raw.dvz_colorbar_set_ticks = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(DvzColorbarTicks)], ctypes.c_bool
    )
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
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32]
    )
    raw.dvz_visual_set_index_data = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
    )
    raw.dvz_text_set_items = _raw_function(
        [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
    )
    raw.dvz_text_set_positions = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double), ctypes.c_uint32]
    )
    raw.dvz_text_set_offsets = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    )
    raw.dvz_text_set_anchors = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    )
    raw.dvz_text_set_sizes = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    )
    raw.dvz_text_set_colors = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32]
    )
    raw.dvz_text_set_angles = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    )
    raw.dvz_view_window = _raw_function(
        [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_char_p],
        ctypes.c_void_p,
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

    def sampled_field_desc():
        desc = DvzSampledFieldDesc()
        desc.struct_size = ctypes.sizeof(DvzSampledFieldDesc)
        return desc

    def field_data_view():
        view = DvzFieldDataView()
        view.struct_size = ctypes.sizeof(DvzFieldDataView)
        return view

    raw.dvz_sampled_field_desc = sampled_field_desc
    raw.dvz_field_data_view = field_data_view
    raw.dvz_sampled_field = _raw_function(
        [ctypes.c_void_p, ctypes.POINTER(DvzSampledFieldDesc)],
        ctypes.c_void_p,
    )
    sampled_field_set_data_calls = []

    def sampled_field_set_data(field, view):
        sampled_field_set_data_calls.append((field, view))
        return 0

    sampled_field_set_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(DvzFieldDataView)]
    sampled_field_set_data.restype = ctypes.c_int
    sampled_field_set_data.__doc__ = 'sampled field set data'
    sampled_field_set_data.calls = sampled_field_set_data_calls
    raw.dvz_sampled_field_set_data = sampled_field_set_data
    sampled_field_update_region_calls = []

    def sampled_field_update_region(field, region, view):
        sampled_field_update_region_calls.append((field, region, view))
        return 0

    sampled_field_update_region.argtypes = [
        ctypes.c_void_p,
        DvzFieldRegion,
        ctypes.POINTER(DvzFieldDataView),
    ]
    sampled_field_update_region.restype = ctypes.c_int
    sampled_field_update_region.__doc__ = 'sampled field update region'
    sampled_field_update_region.calls = sampled_field_update_region_calls
    raw.dvz_sampled_field_update_region = sampled_field_update_region
    raw.__all__ = [
        name for name in vars(raw) if name.startswith(('dvz_', 'Dvz', 'DVZ_'))
    ]

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
    assert 'dvz_colorbar_set_ticks' in facade.__all__
    assert 'dvz_passthrough' in facade.__all__
    assert 'DVZ_SIZE_LOGICAL' in facade.__all__
    assert facade.DvzVisual is raw.DvzVisual
    assert facade.DvzSizeSpace is raw.DvzSizeSpace
    assert facade.DVZ_SIZE_LOGICAL == raw.DVZ_SIZE_LOGICAL
    assert facade.dvz_passthrough(1, 2) == 17
    assert raw.dvz_passthrough.calls[-1] == (1, 2)


def test_top_level_package_reexports_raw_enum_constants(fake_facade):
    raw, facade = fake_facade
    import datoviz

    assert datoviz.DVZ_SIZE_LOGICAL == raw.DVZ_SIZE_LOGICAL
    assert datoviz.DvzSizeSpace is raw.DvzSizeSpace
    assert 'DVZ_SIZE_LOGICAL' in dir(datoviz)


def test_visual_set_data_encodes_string_and_infers_shape0(fake_facade):
    raw, facade = fake_facade
    data = np.arange(6, dtype=np.float32).reshape(3, 2)

    assert facade.dvz_visual_set_data(ctypes.c_void_p(1), 'position', data) == 17

    visual, attr_name, pointer, item_count = raw.dvz_visual_set_data.calls[-1]
    assert visual.value == 1
    assert attr_name == b'position'
    assert pointer.value == data.ctypes.data
    assert item_count == 3


def test_view_window_encodes_title(fake_facade):
    raw, facade = fake_facade

    assert facade.dvz_view_window(ctypes.c_void_p(1), ctypes.c_void_p(2), 320, 240, 'Datoviz') == 17

    app, figure, width, height, title = raw.dvz_view_window.calls[-1]
    assert app.value == 1
    assert figure.value == 2
    assert width == 320
    assert height == 240
    assert title == b'Datoviz'


def test_visual_set_data_range_infers_count_after_first_item(fake_facade):
    raw, facade = fake_facade
    data = np.arange(4, dtype=np.float32).reshape(2, 2)

    facade.dvz_visual_set_data_range(ctypes.c_void_p(2), 'position', 5, data)

    _, attr_name, first_item, pointer, item_count = raw.dvz_visual_set_data_range.calls[-1]
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


def test_axis_set_ticks_accepts_values_and_labels(fake_facade):
    raw, facade = fake_facade
    values = np.array([0.0, 5.0, 10.0], dtype=np.float64)

    assert facade.dvz_axis_set_ticks(ctypes.c_void_p(11), values, ['zero', 'five', 'ten']) == 17

    axis, ticks_ptr = raw.dvz_axis_set_ticks.calls[-1]
    ticks = ticks_ptr._obj
    assert axis.value == 11
    assert ticks.struct_size == ctypes.sizeof(raw.DvzAxisTicks)
    assert ticks.flags == 0
    assert ticks.count == 3
    copied_values = np.ctypeslib.as_array(ticks.values, shape=(3,))
    np.testing.assert_array_equal(copied_values, values)
    assert [ticks.labels[i] for i in range(3)] == [b'zero', b'five', b'ten']


def test_axis_set_ticks_accepts_empty_explicit_ticks(fake_facade):
    raw, facade = fake_facade

    facade.dvz_axis_set_ticks(ctypes.c_void_p(12), [])

    _, ticks_ptr = raw.dvz_axis_set_ticks.calls[-1]
    ticks = ticks_ptr._obj
    assert ticks.count == 0
    assert not bool(ticks.values)
    assert not bool(ticks.labels)


def test_axis_set_ticks_rejects_label_count_mismatch_before_raw_call(fake_facade):
    raw, facade = fake_facade

    with pytest.raises(ValueError, match='labels length 1 does not match tick count 2'):
        facade.dvz_axis_set_ticks(ctypes.c_void_p(13), [0.0, 1.0], ['zero'])

    assert raw.dvz_axis_set_ticks.calls == []


def test_axis_set_ticks_raw_descriptor_passthrough(fake_facade):
    raw, facade = fake_facade
    ticks = raw.DvzAxisTicks()
    ticks.struct_size = ctypes.sizeof(raw.DvzAxisTicks)

    facade.dvz_axis_set_ticks(ctypes.c_void_p(14), ctypes.byref(ticks))

    _, raw_ticks = raw.dvz_axis_set_ticks.calls[-1]
    assert raw_ticks._obj is ticks

    with pytest.raises(TypeError, match='labels must be omitted'):
        facade.dvz_axis_set_ticks(ctypes.c_void_p(14), ctypes.byref(ticks), ['unused'])


def test_colorbar_set_ticks_accepts_values_and_labels(fake_facade):
    raw, facade = fake_facade
    values = np.array([0.0, 0.5, 1.0], dtype=np.float64)

    assert facade.dvz_colorbar_set_ticks(
        ctypes.c_void_p(15), values, ['low', 'mid', 'high']
    ) == 17

    colorbar, ticks_ptr = raw.dvz_colorbar_set_ticks.calls[-1]
    ticks = ticks_ptr._obj
    assert colorbar.value == 15
    assert ticks.struct_size == ctypes.sizeof(raw.DvzColorbarTicks)
    assert ticks.flags == 0
    assert ticks.count == 3
    copied_values = np.ctypeslib.as_array(ticks.values, shape=(3,))
    np.testing.assert_array_equal(copied_values, values)
    assert [ticks.labels[i] for i in range(3)] == [b'low', b'mid', b'high']


def test_colorbar_set_ticks_accepts_empty_explicit_ticks(fake_facade):
    raw, facade = fake_facade

    facade.dvz_colorbar_set_ticks(ctypes.c_void_p(16), [])

    _, ticks_ptr = raw.dvz_colorbar_set_ticks.calls[-1]
    ticks = ticks_ptr._obj
    assert ticks.count == 0
    assert not bool(ticks.values)
    assert not bool(ticks.labels)


def test_colorbar_set_ticks_rejects_label_count_mismatch_before_raw_call(fake_facade):
    raw, facade = fake_facade

    with pytest.raises(ValueError, match='labels length 1 does not match tick count 2'):
        facade.dvz_colorbar_set_ticks(ctypes.c_void_p(17), [0.0, 1.0], ['low'])

    assert raw.dvz_colorbar_set_ticks.calls == []


def test_colorbar_set_ticks_raw_descriptor_passthrough(fake_facade):
    raw, facade = fake_facade
    ticks = raw.DvzColorbarTicks()
    ticks.struct_size = ctypes.sizeof(raw.DvzColorbarTicks)

    facade.dvz_colorbar_set_ticks(ctypes.c_void_p(18), ctypes.byref(ticks))

    _, raw_ticks = raw.dvz_colorbar_set_ticks.calls[-1]
    assert raw_ticks._obj is ticks

    with pytest.raises(TypeError, match='labels must be omitted'):
        facade.dvz_colorbar_set_ticks(ctypes.c_void_p(18), ctypes.byref(ticks), ['unused'])


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


def test_sampled_field_from_rgba_array_infers_desc_and_upload_view(fake_facade):
    raw, facade = fake_facade
    rgba = np.arange(3 * 4 * 4, dtype=np.uint8).reshape(3, 4, 4)

    assert facade.dvz_sampled_field_from_array(ctypes.c_void_p(21), rgba) == 17

    scene, desc_ptr = raw.dvz_sampled_field.calls[-1]
    desc = desc_ptr._obj
    assert scene.value == 21
    assert desc.dim == raw.DVZ_FIELD_DIM_2D
    assert desc.format == raw.DVZ_FIELD_FORMAT_RGBA8_UNORM
    assert desc.semantic == raw.DVZ_FIELD_SEMANTIC_COLOR
    assert desc.color_role == raw.DVZ_COLOR_ROLE_SRGB_COLOR
    assert (desc.width, desc.height, desc.depth) == (4, 3, 1)

    field, view_ptr = raw.dvz_sampled_field_set_data.calls[-1]
    view = view_ptr._obj
    assert field == 17
    assert view.data == rgba.ctypes.data
    assert view.bytes_per_row == rgba.strides[0]
    assert view.rows_per_image == 0


def test_sampled_field_from_scalar_3d_array_uses_volume_layout(fake_facade):
    raw, facade = fake_facade
    volume = np.zeros((5, 3, 4), dtype=np.float32)

    facade.dvz_sampled_field_from_array(ctypes.c_void_p(22), volume)

    _, desc_ptr = raw.dvz_sampled_field.calls[-1]
    desc = desc_ptr._obj
    assert desc.dim == raw.DVZ_FIELD_DIM_3D
    assert desc.format == raw.DVZ_FIELD_FORMAT_R32_FLOAT
    assert desc.semantic == raw.DVZ_FIELD_SEMANTIC_SCALAR
    assert desc.color_role == raw.DVZ_COLOR_ROLE_DATA
    assert (desc.width, desc.height, desc.depth) == (4, 3, 5)

    _, view_ptr = raw.dvz_sampled_field_set_data.calls[-1]
    view = view_ptr._obj
    assert view.bytes_per_row == volume.strides[1]
    assert view.rows_per_image == 3


def test_sampled_field_from_non_contiguous_array_copies_before_upload(fake_facade):
    raw, facade = fake_facade
    data = np.arange(24, dtype=np.float32).reshape(3, 8)[:, ::2]

    facade.dvz_sampled_field_from_array(ctypes.c_void_p(23), data)

    _, desc_ptr = raw.dvz_sampled_field.calls[-1]
    desc = desc_ptr._obj
    assert (desc.width, desc.height, desc.depth) == (4, 3, 1)

    _, view_ptr = raw.dvz_sampled_field_set_data.calls[-1]
    view = view_ptr._obj
    assert view.data != data.ctypes.data
    uploaded = np.ctypeslib.as_array((ctypes.c_float * data.size).from_address(view.data))
    np.testing.assert_array_equal(uploaded.reshape(data.shape), np.ascontiguousarray(data))


def test_sampled_field_rejects_unsupported_shape_without_raw_call(fake_facade):
    raw, facade = fake_facade

    with pytest.raises(ValueError, match='unsupported sampled-field dtype/shape'):
        facade.dvz_sampled_field_from_array(ctypes.c_void_p(24), np.zeros((3, 4), np.float64))

    assert raw.dvz_sampled_field.calls == []


def test_sampled_field_update_from_array_builds_region_and_view(fake_facade):
    raw, facade = fake_facade
    patch = np.arange(12 * 12, dtype=np.float32).reshape(12, 12)
    field = ctypes.c_void_p(31)

    assert facade.dvz_sampled_field_update_from_array(field, patch, offset=(4, 5)) is field

    field_arg, region, view_ptr = raw.dvz_sampled_field_update_region.calls[-1]
    view = view_ptr._obj
    assert field_arg is field
    assert (region.x, region.y, region.z) == (4, 5, 0)
    assert (region.width, region.height, region.depth) == (12, 12, 1)
    assert view.data == patch.ctypes.data
    assert view.bytes_per_row == patch.strides[0]
    assert view.rows_per_image == 0


def test_sampled_field_update_from_non_contiguous_array_copies_patch(fake_facade):
    raw, facade = fake_facade
    patch = np.arange(8 * 6, dtype=np.float32).reshape(6, 8)[:, ::2]

    facade.dvz_sampled_field_update_from_array(ctypes.c_void_p(32), patch, offset=(1, 2, 0))

    _, region, view_ptr = raw.dvz_sampled_field_update_region.calls[-1]
    view = view_ptr._obj
    assert (region.width, region.height, region.depth) == (4, 6, 1)
    assert view.data != patch.ctypes.data
    uploaded = np.ctypeslib.as_array((ctypes.c_float * patch.size).from_address(view.data))
    np.testing.assert_array_equal(uploaded.reshape(patch.shape), np.ascontiguousarray(patch))


def test_sampled_field_update_rejects_mismatched_extent_without_raw_call(fake_facade):
    raw, facade = fake_facade

    with pytest.raises(ValueError, match='does not match array-derived extent'):
        facade.dvz_sampled_field_update_from_array(
            ctypes.c_void_p(33),
            np.zeros((3, 4), dtype=np.float32),
            offset=(0, 0),
            extent=(5, 3),
        )

    assert raw.dvz_sampled_field_update_region.calls == []


def test_top_level_datoviz_resolves_facade_lazily(fake_facade):
    raw, _facade = fake_facade
    import datoviz

    assert datoviz.dvz_visual_set_data(ctypes.c_void_p(6), 'color', np.zeros((1, 4), np.uint8)) == 17
    assert raw.dvz_visual_set_data.calls[-1][1] == b'color'
