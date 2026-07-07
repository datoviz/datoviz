#!/usr/bin/env python3
"""Compare raw point data with a visual-local transform."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 5
Mat4 = (ctypes.c_float * 4) * 4


def _transform(x: float, y: float):
    mat = Mat4()
    values = (
        (1.24, 0.22, 0.0, 0.0),
        (-0.18, 0.82, 0.0, 0.0),
        (0.0, 0.0, 1.0, 0.0),
        (x, y, 0.0, 1.0),
    )
    for i, row in enumerate(values):
        for j, value in enumerate(row):
            mat[i][j] = value
    return mat


def _add_panel_points(scene, panel, color, transformed: bool) -> None:
    positions = np.array(
        [
            [-0.46, -0.24, 0.0],
            [-0.18, +0.24, 0.0],
            [+0.00, -0.08, 0.0],
            [+0.24, +0.30, 0.0],
            [+0.46, -0.18, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(*(color for _ in range(POINT_COUNT)))
    diameters = np.array([28.0, 40.0, 24.0, 44.0, 30.0], dtype=np.float32)

    point = ex.add_points(scene, panel, positions, colors, diameters)
    if transformed:
        if dvz.dvz_visual_set_transform(point, _transform(0.16, 0.18)) != 0:
            raise RuntimeError("dvz_visual_set_transform() failed")
        if not dvz.dvz_visual_has_transform(point):
            raise RuntimeError("dvz_visual_has_transform() failed")


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    base = ex.panel_rect(figure, 0.06, 0.10, 0.41, 0.80)
    transformed = ex.panel_rect(figure, 0.53, 0.10, 0.41, 0.80)
    muted = dvz.DvzColor(ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, 210)
    _add_panel_points(scene, base, muted, False)
    _add_panel_points(scene, transformed, ex.CYAN, True)

    def configure(view) -> None:
        ex.bind_panzoom(view, scene, base, dvz.DVZ_DIM_MASK_XY)
        ex.bind_panzoom(view, scene, transformed, dvz.DVZ_DIM_MASK_XY)

    ex.run_with_view(scene, figure, "Visual Transform", configure)


if __name__ == "__main__":
    main()
