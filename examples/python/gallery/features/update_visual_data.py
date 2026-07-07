#!/usr/bin/env python3
"""Replace all point arrays on an existing visual."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 7


def _point_arrays(updated: bool):
    initial_positions = np.array(
        [
            [-0.72, -0.32, 0.0],
            [-0.48, -0.32, 0.0],
            [-0.24, -0.32, 0.0],
            [+0.00, -0.32, 0.0],
            [+0.24, -0.32, 0.0],
            [+0.48, -0.32, 0.0],
            [+0.72, -0.32, 0.0],
        ],
        dtype=np.float32,
    )
    updated_positions = np.array(
        [
            [-0.72, +0.18, 0.0],
            [-0.48, -0.02, 0.0],
            [-0.24, +0.30, 0.0],
            [+0.00, +0.04, 0.0],
            [+0.24, +0.30, 0.0],
            [+0.48, -0.02, 0.0],
            [+0.72, +0.18, 0.0],
        ],
        dtype=np.float32,
    )
    initial_colors = ex.color_array(*(ex.TEXT for _ in range(POINT_COUNT)))
    updated_colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT, ex.YELLOW, ex.GREEN, ex.CYAN)
    initial_diameters = np.full(POINT_COUNT, 18.0, dtype=np.float32)
    updated_diameters = np.array([26.0, 34.0, 44.0, 58.0, 44.0, 34.0, 26.0], dtype=np.float32)

    if updated:
        return updated_positions, updated_colors, updated_diameters
    return initial_positions, initial_colors, initial_diameters


def _upload(point, updated: bool) -> None:
    positions, colors, diameters = _point_arrays(updated)
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    _upload(point, False)
    ex.set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    ex.add_visual(panel, point)

    state = {"updated": False}

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        if not state["updated"] and elapsed >= 1.0:
            _upload(point, True)
            state["updated"] = True

    ex.run_with_frame_callback(scene, figure, "Visual Data Update", on_frame)


if __name__ == "__main__":
    main()
