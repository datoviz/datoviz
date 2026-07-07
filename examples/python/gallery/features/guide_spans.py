#!/usr/bin/env python3
"""Vertical and horizontal guide spans over 2D markers."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 7


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [0.5, 0.35, 0.0],
            [1.5, 0.82, 0.0],
            [2.5, 1.10, 0.0],
            [3.5, 0.62, 0.0],
            [4.5, 1.35, 0.0],
            [5.5, 1.58, 0.0],
            [6.5, 1.05, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.CYAN, ex.GREEN, ex.CYAN, ex.GREEN, ex.CYAN)
    diameters = np.array([25.0, 25.0, 25.0, 25.0, 38.0, 25.0, 25.0], dtype=np.float32)
    ex.add_points(scene, panel, positions, colors, diameters)


def _add_axes(panel) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")
    if dvz.dvz_axis_set_grid(x_axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid(X) failed")
    if dvz.dvz_axis_set_grid(y_axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid(Y) failed")
    if dvz.dvz_axis_set_label(x_axis, b"time") != 0:
        raise RuntimeError("dvz_axis_set_label(X) failed")
    if dvz.dvz_axis_set_label(y_axis, b"amplitude") != 0:
        raise RuntimeError("dvz_axis_set_label(Y) failed")


def _add_guides(panel) -> None:
    vdesc = dvz.dvz_guide_span_desc()
    vdesc.orientation = dvz.DVZ_GUIDE_ORIENTATION_VERTICAL
    vdesc.min_value = 1.2
    vdesc.max_value = 2.8
    vdesc.fill_color = dvz.DvzColor(76, 201, 240, 42)
    vdesc.outline_color = dvz.DvzColor(76, 201, 240, 170)
    vdesc.outline_width_px = 2.0
    vdesc.label = b"window"
    vspan = dvz.dvz_guide_span(panel, ctypes.byref(vdesc))
    if not vspan:
        raise RuntimeError("dvz_guide_span(vertical) failed")

    hdesc = dvz.dvz_guide_span_desc()
    hdesc.orientation = dvz.DVZ_GUIDE_ORIENTATION_HORIZONTAL
    hdesc.min_value = 0.72
    hdesc.max_value = 1.28
    hdesc.fill_color = dvz.DvzColor(255, 183, 3, 36)
    hdesc.outline_color = dvz.DvzColor(255, 183, 3, 170)
    hdesc.outline_width_px = 2.0
    hdesc.label = b"target band"
    hspan = dvz.dvz_guide_span(panel, ctypes.byref(hdesc))
    if not hspan:
        raise RuntimeError("dvz_guide_span(horizontal) failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 7.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 2.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    _add_guides(panel)
    _add_points(scene, panel)
    _add_axes(panel)

    def configure(view) -> None:
        ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)

    ex.run_with_view(scene, figure, "Guide Spans", configure)


if __name__ == "__main__":
    main()
