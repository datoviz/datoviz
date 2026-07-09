#!/usr/bin/env python3
"""User scale applied to screen-space visual sizes and axes."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


PATH_COUNT = 192
MARKER_COUNT = 9
TAU = 2.0 * np.pi


def _path_data():
    t = np.linspace(0.0, 1.0, PATH_COUNT, dtype=np.float32)
    positions = np.column_stack(
        (
            -3.0 + 6.0 * t,
            0.55 * np.sin(2.0 * TAU * t) + 0.18 * np.sin(7.0 * TAU * t),
            np.zeros(PATH_COUNT, dtype=np.float32),
        )
    ).astype(np.float32)
    colors = np.empty((PATH_COUNT, 4), dtype=np.uint8)
    half = PATH_COUNT // 2
    colors[:half] = (ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, ex.CYAN.a)
    colors[half:] = (ex.GREEN.r, ex.GREEN.g, ex.GREEN.b, ex.GREEN.a)
    widths = np.full(PATH_COUNT, 5.0, dtype=np.float32)
    return positions, colors, widths


def _marker_data():
    t = np.linspace(0.0, 1.0, MARKER_COUNT, dtype=np.float32)
    positions = np.column_stack(
        (-2.75 + 5.5 * t, np.full(MARKER_COUNT, -0.95), np.zeros(MARKER_COUNT))
    ).astype(np.float32)

    palette = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW)
    colors = np.vstack([palette[i % 3] for i in range(MARKER_COUNT)]).astype(np.uint8)
    diameters = np.array([34.0 + 8.0 * (i % 3) for i in range(MARKER_COUNT)], dtype=np.float32)
    angles = np.array([0.16 * i for i in range(MARKER_COUNT)], dtype=np.float32)
    symbols = np.array(
        [
            dvz.DVZ_SYMBOL_DISC if i % 3 == 0 else
            dvz.DVZ_SYMBOL_TRIANGLE if i % 3 == 1 else
            dvz.DVZ_SYMBOL_DIAMOND
            for i in range(MARKER_COUNT)
        ],
        dtype=np.uint32,
    )
    return positions, colors, diameters, angles, symbols


def _add_path(scene, panel) -> None:
    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")

    positions, colors, widths = _path_data()
    if dvz.dvz_visual_set_data_many(
        path,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(path) failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")

    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    if dvz.dvz_panel_add_visual(panel, path, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual(path) failed")


def _add_markers(scene, panel) -> None:
    markers = dvz.dvz_marker(scene, 0)
    if not markers:
        raise RuntimeError("dvz_marker() failed")

    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE
    style.edge_color = ex.TEXT
    style.stroke_width_px = 2.75
    if dvz.dvz_marker_set_style(markers, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")

    positions, colors, diameters, angles, symbols = _marker_data()
    if dvz.dvz_visual_set_data_many(
        markers,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
            "angle": angles,
            "symbol": symbols,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(markers) failed")
    if dvz.dvz_visual_set_alpha_mode(markers, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")
    ex.add_visual(panel, markers)


def _add_axes(panel) -> None:
    axes = dvz.dvz_panel_axes_2d_desc()
    axes.x_label = b"x"
    axes.y_label = b"amplitude"
    if dvz.dvz_panel_set_axes_2d(panel, ctypes.byref(axes)) != 0:
        raise RuntimeError("dvz_panel_set_axes_2d() failed")

    for dim in (dvz.DVZ_DIM_X, dvz.DVZ_DIM_Y):
        axis = dvz.dvz_panel_axis(panel, dim)
        if not axis:
            raise RuntimeError("dvz_panel_axis() failed")
        if dvz.dvz_axis_set_grid(axis, True) != 0:
            raise RuntimeError("dvz_axis_set_grid() failed")
        style = dvz.dvz_axis_style()
        style.tick_size_px = 13.0
        style.label_size_px = 19.0
        style.tick_gap_px = 9.0
        style.grid_color[:] = (116, 132, 148, 105)
        if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
            raise RuntimeError("dvz_axis_set_style() failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -3.25, 3.25) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.45, 1.20) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    _add_path(scene, panel)
    _add_markers(scene, panel)
    _add_axes(panel)
    return scene, figure, panel


def _configure_view(view, scene, panel, *, user_scale: float = 1.4, gui: bool = True) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)
    if dvz.dvz_view_set_user_scale(view, float(user_scale)) != 0:
        raise RuntimeError("dvz_view_set_user_scale() failed")

    if not gui:
        return
    overlay = dvz.dvz_view_gui(view, None)
    if not overlay:
        return

    scale = ctypes.c_float(float(user_scale))

    def gui_callback(gui_ptr, view_ptr, _user_data) -> None:
        if dvz.dvz_gui_begin(gui_ptr, b"User scale", None, 0):
            if dvz.dvz_gui_slider_float(gui_ptr, b"Scale", ctypes.byref(scale), 0.5, 2.5):
                dvz.dvz_view_set_user_scale(view_ptr, scale.value)
        dvz.dvz_gui_end(gui_ptr)

    if dvz.dvz_view_set_gui_callback(view, gui_callback, None) != 0:
        raise RuntimeError("dvz_view_set_gui_callback() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "User Scale", configure)


if __name__ == "__main__":
    main()
