#!/usr/bin/env python3
"""Native Datoviz app window with a retained point scene."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 6


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [-0.70, -0.35, 0.0],
            [-0.42, +0.28, 0.0],
            [-0.14, -0.12, 0.0],
            [+0.14, +0.42, 0.0],
            [+0.42, -0.22, 0.0],
            [+0.70, +0.18, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.BLUE, ex.YELLOW, ex.TEXT, ex.GREEN, ex.WHITE)
    diameters = np.array([24.0, 32.0, 42.0, 54.0, 42.0, 32.0], dtype=np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.add_visual(panel, point)


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    _add_points(scene, panel)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "app_glfw", configure)


if __name__ == "__main__":
    main()
