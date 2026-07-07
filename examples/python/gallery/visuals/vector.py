#!/usr/bin/env python3
"""Straight vector field visual with arrow caps."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    cols, rows = 19, 11
    gx, gy = np.meshgrid(np.linspace(-0.86, 0.86, cols), np.linspace(-0.66, 0.66, rows))
    positions = np.column_stack([gx.ravel(), gy.ravel(), np.zeros(cols * rows)]).astype(np.float32)
    swirl = np.arctan2(gy.ravel(), gx.ravel()) + 0.34 * np.sin(2.0 * np.pi * gx.ravel())
    length = 0.06 + 0.04 * (0.5 + 0.5 * np.sin(2.0 * np.pi * gy.ravel()))
    vectors = np.column_stack(
        [length * np.cos(swirl), length * np.sin(swirl), np.zeros(cols * rows)]
    ).astype(np.float32)
    colors = np.tile(ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW), (cols * rows // 3 + 1, 1))[
        : cols * rows
    ]
    colors[:, 3] = 230
    widths = np.full(cols * rows, 3.0, dtype=np.float32)

    vector = dvz.dvz_vector(scene, 0)
    if not vector:
        raise RuntimeError("dvz_vector() failed")
    style = dvz.dvz_vector_style()
    style.anchor = dvz.DVZ_VECTOR_ANCHOR_CENTER
    style.end_cap = dvz.DVZ_SEGMENT_CAP_TRIANGLE_OUT
    if dvz.dvz_vector_set_style(vector, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_vector_set_style() failed")
    dvz.dvz_visual_set_data_many(
        vector,
        {
            "position": positions,
            "vector": vectors,
            "color": colors,
            "stroke_width_px": widths,
        },
    )
    dvz.dvz_visual_set_depth_test(vector, False)
    ex.add_visual(panel, vector)

    ex.run(scene, figure, "Vector")


if __name__ == "__main__":
    main()
