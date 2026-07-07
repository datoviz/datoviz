#!/usr/bin/env python3
"""Compare a free 2D panel with an equal-aspect 2D panel."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


CIRCLE_COUNT = 97


def _add_circle(scene, panel, color) -> None:
    t = np.linspace(0.0, 1.0, CIRCLE_COUNT, dtype=np.float32)
    angle = 2.0 * np.pi * t
    positions = np.column_stack(
        (np.cos(angle), np.sin(angle), np.zeros(CIRCLE_COUNT, dtype=np.float32)),
    ).astype(np.float32)
    colors = ex.color_array(*(color for _ in range(CIRCLE_COUNT)))
    widths = np.full(CIRCLE_COUNT, 4.0, dtype=np.float32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
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
    ex.add_visual(panel, path)


def _set_free_domain(panel) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _set_equal_view2d(panel) -> None:
    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.18
    desc.domain_x[:] = (-1.0, 1.0)
    desc.domain_y[:] = (-1.0, 1.0)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    free_panel = ex.panel_rect(figure, 0.06, 0.10, 0.41, 0.80)
    fit_panel = ex.panel_rect(figure, 0.53, 0.10, 0.41, 0.80)
    _set_free_domain(free_panel)
    _set_equal_view2d(fit_panel)
    _add_circle(scene, free_panel, ex.GREEN)
    _add_circle(scene, fit_panel, ex.CYAN)

    ex.run(scene, figure, "Panel View 2D")


if __name__ == "__main__":
    main()
