#!/usr/bin/env python3
"""Horizontal and vertical guide lines over a 2D signal."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


PATH_COUNT = 192


def _add_curve(scene, panel) -> None:
    t = np.linspace(0.0, 1.0, PATH_COUNT, dtype=np.float32)
    x = 10.0 * t
    y = 0.72 * np.sin(2.0 * np.pi * t) + 0.18 * np.sin(6.0 * np.pi * t)
    positions = np.column_stack((x, y, np.zeros(PATH_COUNT, dtype=np.float32))).astype(np.float32)
    colors = np.tile(np.array([[34, 211, 238, 255]], dtype=np.uint8), (PATH_COUNT, 1))
    widths = np.full(PATH_COUNT, 4.0, dtype=np.float32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
    if dvz.dvz_visual_set_data_many(
        path,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    ex.add_visual(panel, path)


def _add_axes(panel) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")
    if dvz.dvz_axis_set_grid(x_axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid(X) failed")
    if dvz.dvz_axis_set_grid(y_axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid(Y) failed")
    if dvz.dvz_axis_set_label(x_axis, b"x") != 0:
        raise RuntimeError("dvz_axis_set_label(X) failed")
    if dvz.dvz_axis_set_label(y_axis, b"signal") != 0:
        raise RuntimeError("dvz_axis_set_label(Y) failed")


def _add_guides(panel) -> None:
    hdesc = dvz.dvz_guide_line_desc()
    hdesc.orientation = dvz.DVZ_GUIDE_ORIENTATION_HORIZONTAL
    hdesc.value = 0.45
    hdesc.color = dvz.DvzColor(250, 204, 21, 255)
    hdesc.stroke_width_px = 3.0
    hdesc.label = b"threshold"
    hline = dvz.dvz_guide_line(panel, ctypes.byref(hdesc))
    if not hline:
        raise RuntimeError("dvz_guide_line(horizontal) failed")

    vdesc = dvz.dvz_guide_line_desc()
    vdesc.orientation = dvz.DVZ_GUIDE_ORIENTATION_VERTICAL
    vdesc.value = 3.6
    vdesc.color = dvz.DvzColor(74, 222, 128, 255)
    vdesc.stroke_width_px = 2.5
    vdesc.label = b"event"
    vline = dvz.dvz_guide_line(panel, ctypes.byref(vdesc))
    if not vline:
        raise RuntimeError("dvz_guide_line(vertical) failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 10.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.25, 1.25) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    _add_guides(panel)
    _add_curve(scene, panel)
    _add_axes(panel)

    def configure(view) -> None:
        ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)

    ex.run_with_view(scene, figure, "Guide Lines", configure)


if __name__ == "__main__":
    main()
