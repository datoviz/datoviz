#!/usr/bin/env python3
"""Toggle one visual while neighboring visuals remain visible."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def _add_single_point(scene, panel, x: float, color, visible: bool):
    positions = np.array([[x, 0.0, 0.0]], dtype=np.float32)
    colors = ex.color_array(color)
    diameters = np.array([72.0], dtype=np.float32)
    point = ex.add_points(scene, panel, positions, colors, diameters)
    if dvz.dvz_visual_set_visible(point, visible) != 0:
        raise RuntimeError("dvz_visual_set_visible() failed")
    return point


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    _add_single_point(scene, panel, -0.42, ex.CYAN, True)
    hidden_point = _add_single_point(scene, panel, 0.0, ex.RED, False)
    _add_single_point(scene, panel, +0.42, ex.GREEN, True)

    state = {"visible": False, "tick": -1}

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        tick = int(elapsed / 0.25)
        if tick == state["tick"]:
            return
        visible = (tick % 2) == 0
        if visible != state["visible"]:
            if dvz.dvz_visual_set_visible(hidden_point, visible) != 0:
                raise RuntimeError("dvz_visual_set_visible() failed")
            state["visible"] = visible
        state["tick"] = tick

    ex.run_with_frame_callback(scene, figure, "Visual Visibility", on_frame)


if __name__ == "__main__":
    main()
