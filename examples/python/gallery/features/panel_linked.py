#!/usr/bin/env python3
"""Linked X panzoom controllers on stacked signal panels."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


TAU = 2.0 * np.pi
PATH_COUNT = 128


def _set_domain(panel, ymin: float, ymax: float) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 10.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, ymin, ymax) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _signal(phase: float, green_base: int):
    t = np.linspace(0.0, 1.0, PATH_COUNT, dtype=np.float32)
    y = 0.62 * np.sin(TAU * (1.05 * t + phase))
    y += 0.22 * np.cos(TAU * (2.30 * t + 0.17 + phase))
    positions = np.column_stack((10.0 * t, y, np.zeros(PATH_COUNT))).astype(np.float32)

    colors = np.empty((PATH_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = 70
    colors[:, 1] = np.clip(green_base + 32.0 * t, 0, 255).astype(np.uint8)
    colors[:, 2] = 232
    colors[:, 3] = 242
    widths = np.full(PATH_COUNT, 3.0, dtype=np.float32)
    return positions, colors, widths


def _add_path(scene, panel, phase: float, green_base: int) -> None:
    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
    positions, colors, widths = _signal(phase, green_base)
    if dvz.dvz_visual_set_data_many(
        path,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    dvz.dvz_visual_set_depth_test(path, False)
    ex.add_visual(panel, path)


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    grid = dvz.dvz_figure_grid(figure, 2, 1)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")
    if dvz.dvz_grid_set_gutter(grid, 0.0, 34.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    top = dvz.dvz_grid_panel(grid, 0, 0)
    bottom = dvz.dvz_grid_panel(grid, 1, 0)
    if not top or not bottom:
        raise RuntimeError("dvz_grid_panel() failed")
    for panel in (top, bottom):
        dvz.dvz_panel_set_background_color(panel, ex.BG)

    _set_domain(top, -1.1, 1.1)
    _set_domain(bottom, -1.8, 1.8)
    _add_path(scene, top, 0.03, 188)
    _add_path(scene, bottom, 0.24, 164)

    def configure(view) -> None:
        top_x, top_x_pz = ex.bind_panzoom(view, scene, top, dvz.DVZ_DIM_MASK_X)
        bottom_x, _ = ex.bind_panzoom(view, scene, bottom, dvz.DVZ_DIM_MASK_X)
        _, top_y_pz = ex.bind_panzoom(view, scene, top, dvz.DVZ_DIM_MASK_Y)
        _, bottom_y_pz = ex.bind_panzoom(view, scene, bottom, dvz.DVZ_DIM_MASK_Y)

        if dvz.dvz_panzoom_zoom(top_x_pz, (ctypes.c_float * 2)(1.80, 1.0)) != 0:
            raise RuntimeError("dvz_panzoom_zoom(top_x) failed")
        if dvz.dvz_panzoom_pan(top_x_pz, (ctypes.c_float * 2)(0.22, 0.0)) != 0:
            raise RuntimeError("dvz_panzoom_pan(top_x) failed")
        if dvz.dvz_panzoom_zoom(top_y_pz, (ctypes.c_float * 2)(1.0, 1.15)) != 0:
            raise RuntimeError("dvz_panzoom_zoom(top_y) failed")
        if dvz.dvz_panzoom_zoom(bottom_y_pz, (ctypes.c_float * 2)(1.0, 1.45)) != 0:
            raise RuntimeError("dvz_panzoom_zoom(bottom_y) failed")

        link = dvz.dvz_controller_link(
            scene,
            top_x,
            bottom_x,
            dvz.DVZ_CONTROLLER_LINK_EXTENT_X,
            dvz.DVZ_CONTROLLER_LINK_TWO_WAY,
        )
        if not link:
            raise RuntimeError("dvz_controller_link() failed")

    ex.run_with_view(scene, figure, "Linked Panels", configure)


if __name__ == "__main__":
    main()
