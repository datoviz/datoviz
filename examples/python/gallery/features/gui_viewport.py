#!/usr/bin/env python3
"""Native GUI window hosting an embedded Datoviz render viewport."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


HOST_WIDTH = ex.WIDTH
HOST_HEIGHT = ex.HEIGHT
SOURCE_WIDTH = 640
SOURCE_HEIGHT = 480
POINT_COUNT = 5


class GuiViewportState:
    def __init__(self, point) -> None:
        self.viewport = None
        self.point = point
        self.diameter_px = ctypes.c_float(34.0)
        self.show_points = ctypes.c_bool(True)
        self.show_demo = ctypes.c_bool(False)


def _positions() -> np.ndarray:
    return np.array(
        [
            [-0.68, -0.45, 0.0],
            [-0.28, +0.28, 0.0],
            [+0.00, -0.05, 0.0],
            [+0.34, +0.48, 0.0],
            [+0.72, -0.25, 0.0],
        ],
        dtype=np.float32,
    )


def _colors() -> np.ndarray:
    return ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT, ex.BLUE)


def _upload(state: GuiViewportState) -> None:
    diameter = state.diameter_px.value if state.show_points.value else 0.0
    diameters = np.full(POINT_COUNT, diameter, dtype=np.float32)
    if dvz.dvz_visual_set_data(state.point, "diameter_px", diameters) != 0:
        raise RuntimeError("dvz_visual_set_data(diameter_px) failed")


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")

    source_figure = dvz.dvz_figure(scene, SOURCE_WIDTH, SOURCE_HEIGHT, 0)
    host_figure = dvz.dvz_figure(scene, HOST_WIDTH, HOST_HEIGHT, 0)
    if not source_figure or not host_figure:
        raise RuntimeError("dvz_figure() failed")

    source_panel = dvz.dvz_panel_full(source_figure)
    host_panel = dvz.dvz_panel_full(host_figure)
    if not source_panel or not host_panel:
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(source_panel, ex.BG)
    dvz.dvz_panel_set_background_color(host_panel, dvz.DvzColor(11, 13, 16, 255))

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    state = GuiViewportState(point)
    _upload(state)
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": _positions(),
            "color": _colors(),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.set_filled_point_style(point)
    ex.add_visual(source_panel, point)
    return scene, source_figure, host_figure, source_panel, state


def _gui_callback_factory(state: GuiViewportState):
    def gui_callback(gui, _view, _user_data) -> None:
        if state.viewport:
            dvz.dvz_gui_viewport_window(state.viewport, b"Datoviz viewport", None, 0)

        changed = False
        if dvz.dvz_gui_begin(gui, b"Viewport controls", None, 0):
            changed |= dvz.dvz_gui_slider_float(
                gui, b"Diameter", ctypes.byref(state.diameter_px), 4.0, 80.0
            )
            changed |= dvz.dvz_gui_checkbox(gui, b"Show points", ctypes.byref(state.show_points))
            dvz.dvz_gui_checkbox(gui, b"ImGui demo", ctypes.byref(state.show_demo))
        dvz.dvz_gui_end(gui)

        if state.show_demo.value:
            dvz.dvz_gui_demo(gui, ctypes.byref(state.show_demo))
        if changed:
            _upload(state)

    return gui_callback


def _configure_gui_viewport(view, scene, source_figure, source_panel, state: GuiViewportState) -> None:
    gui = dvz.dvz_view_gui(view, None)
    if not gui:
        raise RuntimeError("dvz_view_gui() failed")

    config = dvz.dvz_gui_viewport_config()
    config.viewport_flags = dvz.DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT
    viewport = dvz.dvz_gui_viewport(gui, source_figure, ctypes.byref(config))
    if not viewport:
        raise RuntimeError("dvz_gui_viewport() failed")
    state.viewport = viewport

    panzoom = dvz.dvz_panzoom(scene, None)
    if not panzoom:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_panel_bind_controller(source_panel, panzoom, dvz.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError("dvz_panel_bind_controller() failed")
    router = dvz.dvz_gui_viewport_input(viewport)
    if not router:
        raise RuntimeError("dvz_gui_viewport_input() failed")
    if dvz.dvz_panel_connect_input(source_panel, router) != 0:
        raise RuntimeError("dvz_panel_connect_input() failed")

    if dvz.dvz_view_set_gui_callback(view, _gui_callback_factory(state), None) != 0:
        raise RuntimeError("dvz_view_set_gui_callback() failed")


def main() -> None:
    scene, source_figure, host_figure, source_panel, state = _build_scene()
    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, host_figure, HOST_WIDTH, HOST_HEIGHT, b"GUI Viewport")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        _configure_gui_viewport(view, scene, source_figure, source_panel, state)
        dvz.dvz_app_run(app, 0)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
