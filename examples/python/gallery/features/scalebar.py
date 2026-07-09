#!/usr/bin/env python3
"""Retained metric scale bar attached to a 2D panel."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 5


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [0.0, 0.0, 0.0],
            [2.0, 0.0, 0.0],
            [4.0, 0.0, 0.0],
            [6.0, 0.0, 0.0],
            [8.0, 0.0, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.TEXT, ex.TEXT, ex.TEXT, ex.CYAN)
    colors[1:4, 3] = 232
    diameters = np.array([22.0, 16.0, 16.0, 16.0, 22.0], dtype=np.float32)

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
    if dvz.dvz_visual_set_depth_test(point, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(point) failed")
    ex.add_visual(panel, point)


def _add_scalebar(scene, panel) -> None:
    units = dvz.dvz_units_builtin(scene, dvz.DVZ_UNIT_LADDER_METRIC_LENGTH, 0.001)
    if not units:
        raise RuntimeError("dvz_units_builtin() failed")

    desc = dvz.dvz_scale_bar_desc()
    desc.dimension = dvz.DVZ_DIM_X
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_BOTTOM_LEFT
    desc.label_position = dvz.DVZ_SCALEBAR_LABEL_ABOVE
    desc.target_length_px = 220.0
    desc.min_length_px = 160.0
    desc.max_length_px = 300.0
    desc.offset_px[:] = (72.0, 82.0)
    desc.tick_length_px = 18.0
    desc.line_width_px = 4.0
    desc.line_color[:] = (ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 255)

    scalebar = dvz.dvz_scale_bar(panel, ctypes.byref(desc))
    if not scalebar:
        raise RuntimeError("dvz_scale_bar() failed")

    style = dvz.dvz_text_style()
    style.size_px = 17.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 255)
    if dvz.dvz_scale_bar_set_label_style(scalebar, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_scale_bar_set_label_style() failed")
    if dvz.dvz_scale_bar_set_units(scalebar, units) != 0:
        raise RuntimeError("dvz_scale_bar_set_units() failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -1.0, 9.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(x) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(y) failed")
    _add_points(scene, panel)
    _add_scalebar(scene, panel)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Scale Bar", configure)


if __name__ == "__main__":
    main()
