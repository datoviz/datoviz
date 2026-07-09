#!/usr/bin/env python3
"""Raw cimgui widgets updating a retained point visual."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz
import datoviz._ctypes as raw

from examples.python.gallery import common as ex


POINT_COUNT = 4
IMGUI_TABLE_FLAGS_ROW_BG = 1 << 6
IMGUI_TABLE_FLAGS_BORDERS_INNER = (1 << 7) | (1 << 9)


class ImVec2(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
    ]


class _RawImGui:
    def __init__(self) -> None:
        lib = raw.dvz
        self.begin = lib.igBegin
        self.begin.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_bool),
            ctypes.c_int,
        ]
        self.begin.restype = ctypes.c_bool

        self.end = lib.igEnd
        self.end.argtypes = []
        self.end.restype = None

        self.text_unformatted = lib.igTextUnformatted
        self.text_unformatted.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.text_unformatted.restype = None

        self.get_version = lib.igGetVersion
        self.get_version.argtypes = []
        self.get_version.restype = ctypes.c_char_p

        self.begin_table = lib.igBeginTable
        self.begin_table.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int,
            ImVec2,
            ctypes.c_float,
        ]
        self.begin_table.restype = ctypes.c_bool

        self.end_table = lib.igEndTable
        self.end_table.argtypes = []
        self.end_table.restype = None

        self.table_next_row = lib.igTableNextRow
        self.table_next_row.argtypes = [ctypes.c_int, ctypes.c_float]
        self.table_next_row.restype = None

        self.table_set_column_index = lib.igTableSetColumnIndex
        self.table_set_column_index.argtypes = [ctypes.c_int]
        self.table_set_column_index.restype = ctypes.c_bool

        self.slider_float = lib.igSliderFloat
        self.slider_float.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_float),
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_char_p,
            ctypes.c_int,
        ]
        self.slider_float.restype = ctypes.c_bool

        self.checkbox = lib.igCheckbox
        self.checkbox.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_bool)]
        self.checkbox.restype = ctypes.c_bool

        self.show_demo_window = lib.igShowDemoWindow
        self.show_demo_window.argtypes = [ctypes.POINTER(ctypes.c_bool)]
        self.show_demo_window.restype = None


IMGUI = _RawImGui()


class GuiCimguiState:
    def __init__(self, point) -> None:
        self.point = point
        self.diameter_px = ctypes.c_float(44.0)
        self.show_demo = ctypes.c_bool(False)


def _positions() -> np.ndarray:
    return np.array(
        [
            [-0.60, -0.30, 0.0],
            [-0.20, +0.30, 0.0],
            [+0.20, -0.30, 0.0],
            [+0.60, +0.30, 0.0],
        ],
        dtype=np.float32,
    )


def _colors() -> np.ndarray:
    return ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT)


def _upload(state: GuiCimguiState) -> None:
    diameters = np.full(POINT_COUNT, state.diameter_px.value, dtype=np.float32)
    if dvz.dvz_visual_set_data(state.point, "diameter_px", diameters) != 0:
        raise RuntimeError("dvz_visual_set_data(diameter_px) failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    state = GuiCimguiState(point)
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": _positions(),
            "color": _colors(),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    _upload(state)

    ex.set_filled_point_style(point)
    if dvz.dvz_visual_set_depth_test(point, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")
    ex.add_visual(panel, point)
    return scene, figure, panel, state


def _table_row(label: bytes, value: bytes) -> None:
    IMGUI.table_next_row(0, 0.0)
    IMGUI.table_set_column_index(0)
    IMGUI.text_unformatted(label, None)
    IMGUI.table_set_column_index(1)
    IMGUI.text_unformatted(value, None)


def _gui_callback_factory(state: GuiCimguiState):
    def gui_callback(_gui, _view, _user_data) -> None:
        open_window = ctypes.c_bool(True)
        if IMGUI.begin(b"Raw cimgui", ctypes.byref(open_window), 0):
            version = IMGUI.get_version() or b"unknown"
            IMGUI.text_unformatted(b"Dear ImGui " + version, None)
            table_flags = IMGUI_TABLE_FLAGS_ROW_BG | IMGUI_TABLE_FLAGS_BORDERS_INNER
            if IMGUI.begin_table(b"status", 2, table_flags, ImVec2(0.0, 0.0), 0.0):
                _table_row(b"binding", b"datoviz/imgui.h")
                _table_row(b"visual", b"retained point")
                IMGUI.end_table()
            if IMGUI.slider_float(
                b"Diameter", ctypes.byref(state.diameter_px), 8.0, 90.0, b"%.1f", 0
            ):
                _upload(state)
            IMGUI.checkbox(b"ImGui demo", ctypes.byref(state.show_demo))
        IMGUI.end()

        if state.show_demo.value:
            IMGUI.show_demo_window(ctypes.byref(state.show_demo))

    return gui_callback


def _configure_view(view, state: GuiCimguiState, *, gui: bool = True) -> None:
    if not gui:
        return
    overlay = dvz.dvz_view_gui(view, None)
    if not overlay:
        return
    if dvz.dvz_view_set_gui_callback(view, _gui_callback_factory(state), None) != 0:
        raise RuntimeError("dvz_view_set_gui_callback() failed")


def main() -> None:
    scene, figure, _panel, state = _build_scene()

    def configure(view) -> None:
        _configure_view(view, state)

    ex.run_with_view(scene, figure, "Raw cimgui GUI", configure)


if __name__ == "__main__":
    main()
