"""Focused tests for the small v0.4 Python facade adapters."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz._array_facade as facade


def _spy(argtypes, result=17):
    calls = []

    def call(*args):
        calls.append(args)
        return result

    call.argtypes = argtypes
    call.restype = ctypes.c_int
    return call, calls


def test_path_text_link_and_colormap_adapters(monkeypatch):
    path, path_calls = _spy([ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)])
    text, text_calls = _spy([ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint32])
    link, link_calls = _spy(
        [ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.c_uint32]
    )
    target_link, target_link_calls = _spy(
        [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint64),
            ctypes.c_uint32,
        ]
    )
    cmap, cmap_calls = _spy(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32],
        result=ctypes.c_void_p(3),
    )
    monkeypatch.setattr(facade._raw, 'dvz_path_set_subpaths', path)
    monkeypatch.setattr(facade._raw, 'dvz_text_set_strings', text)
    monkeypatch.setattr(facade._raw, 'dvz_visual_set_link_keys', link)
    monkeypatch.setattr(facade._raw, 'dvz_visual_set_target_link_keys', target_link)
    monkeypatch.setattr(facade._raw, 'dvz_colormap_custom', cmap)

    assert facade.dvz_path_set_subpaths(ctypes.c_void_p(1), [2, 3]) == 17
    assert path_calls[-1][1] == 2
    assert np.ctypeslib.as_array(path_calls[-1][2], shape=(2,)).tolist() == [2, 3]

    assert facade.dvz_text_set_strings(ctypes.c_void_p(2), ['α', b'b']) == 17
    assert text_calls[-1][2] == 2
    assert [text_calls[-1][1][i] for i in range(2)] == ['α'.encode(), b'b']

    keys = np.array([11, 12], dtype=np.uint64)
    assert facade.dvz_visual_set_link_keys(ctypes.c_void_p(3), ctypes.c_void_p(4), keys) == 17
    assert link_calls[-1][3] == 2
    assert np.ctypeslib.as_array(link_calls[-1][2], shape=(2,)).tolist() == [11, 12]

    assert facade.dvz_visual_set_target_link_keys(
        ctypes.c_void_p(3), facade.DVZ_SCENE_TARGET_FACE, ctypes.c_void_p(4), keys
    ) == 17
    assert target_link_calls[-1][1] == facade.DVZ_SCENE_TARGET_FACE
    assert target_link_calls[-1][4] == 2
    assert np.ctypeslib.as_array(target_link_calls[-1][3], shape=(2,)).tolist() == [11, 12]

    colors = np.array([[0, 1, 2, 255], [255, 2, 1, 0]], dtype=np.uint8)
    assert facade.dvz_colormap_custom(ctypes.c_void_p(5), 'ramp', colors).value == 3
    assert cmap_calls[-1][1] == b'ramp'
    assert cmap_calls[-1][3] == 2
    assert np.ctypeslib.as_array(cmap_calls[-1][2], shape=(2, 4)).tolist() == colors.tolist()


def test_colormap_rejects_wrong_shape_and_lights_build_handle_array(monkeypatch):
    cmap, cmap_calls = _spy(
        [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32],
        result=ctypes.c_void_p(3),
    )
    light_type = ctypes.c_void_p
    lights, lights_calls = _spy(
        [ctypes.c_void_p, ctypes.POINTER(light_type), ctypes.c_uint32], result=0
    )
    monkeypatch.setattr(facade._raw, 'dvz_colormap_custom', cmap)
    monkeypatch.setattr(facade._raw, 'dvz_panel_set_lights', lights)

    with np.testing.assert_raises(ValueError):
        facade.dvz_colormap_custom(ctypes.c_void_p(1), b'bad', np.zeros((2, 3), dtype=np.uint8))
    with np.testing.assert_raises(ValueError):
        facade.dvz_colormap_custom(ctypes.c_void_p(1), b'bad', [[0.5, 0, 0, 1]])
    with np.testing.assert_raises(ValueError):
        facade.dvz_colormap_custom(
            ctypes.c_void_p(1), b'bad', np.zeros((2, 4), dtype=np.uint8), count=3
        )

    assert facade.dvz_panel_set_lights(
        ctypes.c_void_p(2), [ctypes.c_void_p(7), ctypes.c_void_p(8)]
    ) == 0
    assert lights_calls[-1][2] == 2
    assert [lights_calls[-1][1][i] for i in range(2)] == [7, 8]
