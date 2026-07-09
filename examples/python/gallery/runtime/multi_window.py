#!/usr/bin/env python3
"""Two native Datoviz windows driven by one app."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


WIDTH = 720
HEIGHT = 520
POINT_COUNT = 8
FIRST_WINDOW_X = 64
FIRST_WINDOW_Y = 96
WINDOW_GAP_X = 32


def _add_points(scene, panel, positions, colors, diameters) -> None:
    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.add_visual(panel, point)


def _overview_data():
    positions = np.array(
        [
            [-0.78, -0.40, 0.0],
            [-0.55, +0.20, 0.0],
            [-0.32, -0.08, 0.0],
            [-0.10, +0.48, 0.0],
            [+0.16, -0.26, 0.0],
            [+0.38, +0.36, 0.0],
            [+0.62, -0.02, 0.0],
            [+0.82, +0.30, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(
        ex.CYAN, ex.BLUE, ex.YELLOW, ex.TEXT, ex.GREEN, ex.WHITE, ex.CYAN, ex.BLUE
    )
    diameters = np.array(
        [26.0, 34.0, 42.0, 54.0, 46.0, 38.0, 30.0, 24.0], dtype=np.float32
    )
    return positions, colors, diameters


def _detail_data():
    positions = np.array(
        [
            [-0.42, -0.35, 0.0],
            [-0.34, +0.10, 0.0],
            [-0.20, +0.38, 0.0],
            [-0.02, -0.08, 0.0],
            [+0.14, +0.52, 0.0],
            [+0.30, -0.28, 0.0],
            [+0.48, +0.16, 0.0],
            [+0.62, +0.42, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(
        ex.YELLOW, ex.CYAN, ex.TEXT, ex.BLUE, ex.GREEN, ex.WHITE, ex.YELLOW, ex.CYAN
    )
    diameters = np.array(
        [36.0, 44.0, 58.0, 50.0, 42.0, 34.0, 28.0, 24.0], dtype=np.float32
    )
    return positions, colors, diameters


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")

    overview = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    detail = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    if not overview or not detail:
        raise RuntimeError("dvz_figure() failed")

    overview_panel = dvz.dvz_panel_full(overview)
    detail_panel = dvz.dvz_panel_full(detail)
    if not overview_panel or not detail_panel:
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(overview_panel, ex.BG)
    dvz.dvz_panel_set_background_color(detail_panel, ex.BG)

    _add_points(scene, overview_panel, *_overview_data())
    _add_points(scene, detail_panel, *_detail_data())
    return scene, overview, detail, overview_panel, detail_panel


def _positioned_view(app, figure, title: bytes, x: int, y: int):
    desc = dvz.dvz_view_desc(dvz.DVZ_VIEW_WINDOW)
    desc.size_policy = dvz.DVZ_VIEW_SIZE_HOST_LOGICAL_PX
    desc.size_width = float(WIDTH)
    desc.size_height = float(HEIGHT)
    desc.title = title
    desc.has_position = True
    desc.x = x
    desc.y = y
    view = dvz.dvz_view(app, figure, ctypes.byref(desc))
    if not view:
        raise RuntimeError("dvz_view() failed")
    return view


def _configure_view(view, scene, panel) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, overview, detail, overview_panel, detail_panel = _build_scene()
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_app() failed")

    try:
        overview_view = _positioned_view(
            app, overview, b"multi_window overview", FIRST_WINDOW_X, FIRST_WINDOW_Y
        )
        detail_view = _positioned_view(
            app,
            detail,
            b"multi_window detail",
            FIRST_WINDOW_X + WIDTH + WINDOW_GAP_X,
            FIRST_WINDOW_Y,
        )
        _configure_view(overview_view, scene, overview_panel)
        _configure_view(detail_view, scene, detail_panel)
        dvz.dvz_app_run(app, 0)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
