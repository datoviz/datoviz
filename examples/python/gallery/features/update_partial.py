#!/usr/bin/env python3
"""Move part of a point visual with a retained data-range update."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    positions = np.array(
        [
            [-0.72, -0.30, 0.0],
            [-0.44, -0.30, 0.0],
            [-0.16, -0.30, 0.0],
            [+0.16, -0.30, 0.0],
            [+0.44, -0.30, 0.0],
            [+0.72, -0.30, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.TEXT, ex.CYAN, ex.GREEN, ex.GREEN, ex.CYAN, ex.TEXT)
    diameters = np.array([32.0, 36.0, 42.0, 42.0, 36.0, 32.0], dtype=np.float32)
    point = ex.add_points(scene, panel, positions, colors, diameters)

    state = {"updated": False}

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        if state["updated"] or elapsed < 1.0:
            return
        moved_positions = np.array(
            [
                [-0.16, +0.34, 0.0],
                [+0.16, +0.34, 0.0],
            ],
            dtype=np.float32,
        )
        if dvz.dvz_visual_set_data_range(point, "position", 2, moved_positions) != 0:
            raise RuntimeError("dvz_visual_set_data_range() failed")
        state["updated"] = True

    ex.run_with_frame_callback(scene, figure, "Partial Data Update", on_frame)


if __name__ == "__main__":
    main()
