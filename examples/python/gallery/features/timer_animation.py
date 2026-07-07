#!/usr/bin/env python3
"""Point attributes updated every frame from a view callback."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 8
TAU = 2.0 * np.pi


def _timer_points(t: float):
    u = np.linspace(0.0, 1.0, POINT_COUNT, dtype=np.float32)
    phase = TAU * (u + 0.20 * t)
    positions = np.column_stack(
        (-0.78 + 1.56 * u, 0.22 * np.sin(phase), np.zeros(POINT_COUNT)),
    ).astype(np.float32)
    diameters = (28.0 + 18.0 * (0.5 + 0.5 * np.cos(phase))).astype(np.float32)

    palette = (ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT)
    color_index = int(2.0 * t)
    colors = ex.color_array(*(palette[(i + color_index) % len(palette)] for i in range(POINT_COUNT)))
    return positions, colors, diameters


def _upload(point, t: float) -> None:
    positions, colors, diameters = _timer_points(t)
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
    _upload(point, 0.0)
    ex.set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    ex.add_visual(panel, point)

    def on_frame(_view, frame_index: int, _elapsed: float) -> None:
        _upload(point, frame_index / 60.0)

    ex.run_with_frame_callback(scene, figure, "Timer Animation", on_frame)


if __name__ == "__main__":
    main()
