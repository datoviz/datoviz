#!/usr/bin/env python3
"""Quickstart scatter plot with random colored points."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 10000


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    rng = np.random.default_rng(12345)
    positions = np.zeros((POINT_COUNT, 3), dtype=np.float32)
    positions[:, :2] = rng.uniform(-1.0, 1.0, size=(POINT_COUNT, 2))
    colors = rng.integers(0, 255, size=(POINT_COUNT, 4), dtype=np.uint8)
    colors[:, 3] = 200
    diameters = rng.uniform(4.0, 12.0, size=POINT_COUNT).astype(np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
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
    dvz.dvz_visual_set_alpha_mode(point, dvz.DVZ_ALPHA_BLENDED)
    ex.add_visual(panel, point)

    ex.run(scene, figure, "Scatter Plot")


if __name__ == "__main__":
    main()
