#!/usr/bin/env python3
"""Smallest retained scene with one point visual."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")

    positions = np.array(
        [[-0.45, -0.25, 0.0], [0.00, 0.34, 0.0], [0.45, -0.25, 0.0]],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW)
    diameters = np.array([42.0, 58.0, 42.0], dtype=np.float32)

    dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    )
    ex.set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    ex.add_visual(panel, point)

    ex.run(scene, figure, "Basic Scene")


if __name__ == "__main__":
    main()
