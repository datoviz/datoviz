#!/usr/bin/env python3
"""Path visual in data coordinates with retained x/y axes."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 10.0)
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -2.0, 2.0)

    count = 384
    t = np.linspace(0.0, 1.0, count, dtype=np.float32)
    x = 10.0 * t
    envelope = np.exp(-0.12 * x)
    y = envelope * (
        1.65 * np.sin(1.25 * 2.0 * np.pi * t)
        + 0.35 * np.sin(4.0 * 2.0 * np.pi * t + 0.35)
    )
    positions = np.column_stack([x, y, np.zeros(count)]).astype(np.float32)
    colors = np.column_stack(
        [
            (30 + 90 * t).astype(np.uint8),
            (170 + 50 * t).astype(np.uint8),
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

    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    dvz.dvz_axis_set_grid(x_axis, True)
    dvz.dvz_axis_set_grid(y_axis, True)
    dvz.dvz_axis_set_label(x_axis, b"time (s)")
    dvz.dvz_axis_set_label(y_axis, b"signal")

    ex.run(scene, figure, "Path With 2D Axes")


if __name__ == "__main__":
    main()
