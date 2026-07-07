#!/usr/bin/env python3
"""2D point visual with scalar-derived color and per-point diameter."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 960
TAU = 2.0 * np.pi


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    i = np.arange(POINT_COUNT, dtype=np.float32)
    arm = np.mod(i, 6.0)
    local = np.floor(i / 6.0) / (POINT_COUNT / 6.0)
    theta = TAU * (2.10 * local + arm / 6.0)
    radius = 0.10 + 0.82 * np.sqrt(local)
    ripple = 0.040 * np.sin(TAU * (3.0 * local + 0.13 * arm))

    positions = np.zeros((POINT_COUNT, 3), dtype=np.float32)
    positions[:, 0] = (radius + ripple) * np.cos(theta)
    positions[:, 1] = 0.84 * (radius - 0.5 * ripple) * np.sin(theta)

    band = 0.5 + 0.5 * np.sin(TAU * (i / (POINT_COUNT - 1) + 0.08 * arm))
    values = np.clip(0.12 + 0.76 * (0.25 + 0.75 * np.sqrt(local)) + 0.12 * band, 0.0, 1.0)
    colors = np.column_stack(
        [
            (40 + 80 * values).astype(np.uint8),
            (100 + 130 * values).astype(np.uint8),
            (180 + 60 * (1.0 - values)).astype(np.uint8),
            np.full(POINT_COUNT, 230, dtype=np.uint8),
        ]
    )
    diameters = (10.0 + 11.0 * band + 5.0 * (1.0 - local)).astype(np.float32)

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

    ex.run(scene, figure, "Point")


if __name__ == "__main__":
    main()
