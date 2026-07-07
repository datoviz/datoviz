#!/usr/bin/env python3
"""Stroked path visual with per-sample color and width."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    count = 384
    t = np.linspace(0.0, 1.0, count, dtype=np.float32)
    x = -0.88 + 1.76 * t
    y = 0.42 * np.sin(2.4 * np.pi * t) + 0.16 * np.sin(9.0 * np.pi * t + 0.2)
    positions = np.column_stack([x, y, np.zeros(count)]).astype(np.float32)
    colors = np.column_stack(
        [
            (40 + 140 * t).astype(np.uint8),
            (220 - 70 * t).astype(np.uint8),
            np.full(count, 238, dtype=np.uint8),
            np.full(count, 255, dtype=np.uint8),
        ]
    )
    widths = np.full(count, 4.0, dtype=np.float32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
    dvz.dvz_visual_set_data_many(
        path,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    )
    dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND)
    dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0)
    ex.add_visual(panel, path)

    ex.run(scene, figure, "Path")


if __name__ == "__main__":
    main()
