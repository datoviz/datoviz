#!/usr/bin/env python3
"""Custom panel background behind a foreground primitive."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def _set_gradient_background(panel) -> None:
    background = dvz.dvz_panel_background_desc()
    background.type = dvz.DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT
    background.gradient.start[:] = (0.0, 0.0)
    background.gradient.end[:] = (1.0, 1.0)
    background.gradient.color0[:] = (0.010, 0.030, 0.065, 1.0)
    background.gradient.color1[:] = (0.025, 0.345, 0.380, 1.0)
    if dvz.dvz_panel_set_background(panel, ctypes.byref(background)) != 0:
        raise RuntimeError("dvz_panel_set_background() failed")


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    panel = ex.panel_rect(figure, 0.10, 0.12, 0.80, 0.76)
    _set_gradient_background(panel)

    positions = np.array(
        [
            [-0.70, -0.40, 0.0],
            [-0.10, -0.40, 0.0],
            [-0.40, +0.42, 0.0],
            [+0.10, -0.40, 0.0],
            [+0.70, -0.40, 0.0],
            [+0.40, +0.42, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW, ex.GREEN, ex.CYAN, ex.TEXT)
    normals = np.zeros_like(positions)
    normals[:, 2] = 1.0

    visual = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)
    if not visual:
        raise RuntimeError("dvz_primitive() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "color": colors,
            "normal": normals,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")

    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_VIEW
    if dvz.dvz_panel_add_visual(panel, visual, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual() failed")

    ex.run(scene, figure, "Panel Background")


if __name__ == "__main__":
    main()
