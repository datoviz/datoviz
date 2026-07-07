#!/usr/bin/env python3
"""2D panzoom controller bound to data-space points."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


TAU = 2.0 * np.pi
POINT_COUNT = 64


def _points():
    idx = np.arange(POINT_COUNT, dtype=np.float32)
    theta = TAU * idx / float(POINT_COUNT)
    radius = 0.42 + 0.18 * np.sin(5.0 * theta)

    positions = np.column_stack(
        (radius * np.cos(theta), radius * np.sin(theta), np.zeros(POINT_COUNT)),
    ).astype(np.float32)
    colors = np.empty((POINT_COUNT, 4), dtype=np.uint8)
    colors[0::2] = np.array([34, 211, 238, 230], dtype=np.uint8)
    colors[1::2] = np.array([250, 204, 21, 230], dtype=np.uint8)
    diameters = (8.0 + 5.0 * (0.5 + 0.5 * np.sin(3.0 * theta))).astype(np.float32)
    return positions, colors, diameters


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    positions, colors, diameters = _points()
    ex.add_points(scene, panel, positions, colors, diameters)

    def configure(view) -> None:
        _, panzoom = ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)
        if dvz.dvz_panzoom_zoom(panzoom, (ctypes.c_float * 2)(1.16, 1.16)) != 0:
            raise RuntimeError("dvz_panzoom_zoom() failed")
        if dvz.dvz_panzoom_pan(panzoom, (ctypes.c_float * 2)(-0.08, 0.06)) != 0:
            raise RuntimeError("dvz_panzoom_pan() failed")

    ex.run_with_view(scene, figure, "Panzoom", configure)


if __name__ == "__main__":
    main()
