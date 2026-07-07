#!/usr/bin/env python3
"""Independent stroked line segments with rounded caps."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    count = 32
    t = np.linspace(0.0, 1.0, count, dtype=np.float32)
    a = 2.0 * np.pi * t
    starts = np.column_stack([0.18 * np.cos(a), 0.18 * np.sin(a), np.zeros(count)]).astype(
        np.float32
    )
    ends = np.column_stack([0.78 * np.cos(a), 0.55 * np.sin(a), np.zeros(count)]).astype(np.float32)
    colors = np.column_stack(
        [
            np.full(count, 34, dtype=np.uint8),
            (150 + 80 * t).astype(np.uint8),
            np.full(count, 238, dtype=np.uint8),
            np.full(count, 235, dtype=np.uint8),
        ]
    )
    widths = np.linspace(2.0, 8.0, count, dtype=np.float32)

    segment = dvz.dvz_segment(scene, 0)
    if not segment:
        raise RuntimeError("dvz_segment() failed")
    dvz.dvz_visual_set_data_many(
        segment,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    )
    dvz.dvz_segment_set_caps(segment, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND)
    ex.add_visual(panel, segment)

    ex.run(scene, figure, "Segment")


if __name__ == "__main__":
    main()
