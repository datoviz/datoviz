#!/usr/bin/env python3
"""Compare overlapping 3D points with depth testing on and off."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 8


def _set_depth_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.0, 0.0, 10.0)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.projection.fov_y = 0.20
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _add_depth_points(scene, panel, depth_test_enabled: bool) -> None:
    s = 0.15
    positions = np.array(
        [
            [-s, -s, +s],
            [+s, -s, +s],
            [-s, +s, +s],
            [+s, +s, +s],
            [-s, -s, -s],
            [+s, -s, -s],
            [-s, +s, -s],
            [+s, +s, -s],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.CYAN, ex.GREEN, ex.YELLOW, ex.RED, ex.YELLOW, ex.RED)
    colors[:, 3] = 248
    diameters = np.full(POINT_COUNT, 100.0, dtype=np.float32)

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
        raise RuntimeError("dvz_visual_set_data_many() failed")

    style = dvz.dvz_point_style_desc()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE
    style.stroke_width_px = 2.5
    style.edge_color = ex.TEXT
    if dvz.dvz_point_set_style(point, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_point_set_style() failed")
    if dvz.dvz_visual_set_depth_test(point, depth_test_enabled) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")
    ex.add_visual(panel, point)


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    depth_on = ex.panel_rect(figure, 0.06, 0.10, 0.41, 0.80)
    depth_off = ex.panel_rect(figure, 0.53, 0.10, 0.41, 0.80)
    for panel, enabled in ((depth_on, True), (depth_off, False)):
        _set_depth_camera(panel)
        _add_depth_points(scene, panel, enabled)

    ex.run(scene, figure, "Depth Test Toggle")


if __name__ == "__main__":
    main()
