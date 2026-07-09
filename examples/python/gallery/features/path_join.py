#!/usr/bin/env python3
"""Miter, round, and bevel joins on difficult stroked paths."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


MAX_SUBPATHS = 4
STAR_POINTS = 5
TAU = 2.0 * math.pi


def _color_rgba(color, alpha: int):
    return color.r, color.g, color.b, int(alpha)


def _append_point(positions, colors, widths, x: float, y: float, color, width: float) -> None:
    positions.append((x, y, 0.0))
    colors.append(color)
    widths.append(width)


def _append_v_path(positions, colors, widths, subpaths, cx: float, cy: float, color, width: float):
    first = len(positions)
    _append_point(positions, colors, widths, cx - 0.21, cy - 0.11, color, width)
    _append_point(positions, colors, widths, cx + 0.00, cy + 0.16, color, width)
    _append_point(positions, colors, widths, cx + 0.21, cy - 0.11, color, width)
    subpaths.append(len(positions) - first)


def _append_zigzag_path(
    positions, colors, widths, subpaths, cx: float, cy: float, color, width: float
):
    first = len(positions)
    for x, y in ((-0.25, -0.10), (-0.10, +0.14), (+0.03, -0.12), (+0.16, +0.13), (+0.27, -0.10)):
        _append_point(positions, colors, widths, cx + x, cy + y, color, width)
    subpaths.append(len(positions) - first)


def _append_open_star_path(
    positions, colors, widths, subpaths, cx: float, cy: float, color, width: float
):
    first = len(positions)
    for order in (0, 2, 4, 1, 3):
        angle = -0.25 * TAU + TAU * order / STAR_POINTS
        _append_point(
            positions,
            colors,
            widths,
            cx + 0.21 * math.cos(angle),
            cy + 0.21 * math.sin(angle),
            color,
            width,
        )
    subpaths.append(len(positions) - first)


def _append_closed_star_path(
    positions, colors, widths, subpaths, cx: float, cy: float, color, width: float
):
    first = len(positions)
    for i in range(2 * STAR_POINTS + 1):
        k = i % (2 * STAR_POINTS)
        radius = 0.23 if k % 2 == 0 else 0.075
        angle = -0.25 * TAU + TAU * k / (2 * STAR_POINTS)
        _append_point(
            positions,
            colors,
            widths,
            cx + radius * math.cos(angle),
            cy + radius * math.sin(angle),
            color,
            width,
        )
    subpaths.append(len(positions) - first)


def _join_column_data(cx: float, color):
    positions = []
    colors = []
    widths = []
    subpaths = []
    translucent = _color_rgba(color, 168)
    opaque = _color_rgba(color, 255)

    _append_v_path(positions, colors, widths, subpaths, cx, +0.61, translucent, 48.0)
    _append_zigzag_path(positions, colors, widths, subpaths, cx, +0.19, translucent, 36.0)
    _append_open_star_path(positions, colors, widths, subpaths, cx, -0.23, opaque, 28.0)
    _append_closed_star_path(positions, colors, widths, subpaths, cx, -0.67, opaque, 22.0)

    if len(subpaths) > MAX_SUBPATHS:
        raise RuntimeError("too many subpaths")
    return (
        np.array(positions, dtype=np.float32),
        np.array(colors, dtype=np.uint8),
        np.array(widths, dtype=np.float32),
        np.array(subpaths, dtype=np.uint32),
    )


def _add_join_column(scene, panel, join, cx: float, color) -> None:
    positions, colors, widths, subpaths = _join_column_data(cx, color)

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
        raise RuntimeError("dvz_visual_set_data_many(path) failed")
    lengths = (np.ctypeslib.as_ctypes(subpaths))
    if dvz.dvz_path_set_subpaths(path, len(subpaths), lengths) != 0:
        raise RuntimeError("dvz_path_set_subpaths() failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, join, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    if dvz.dvz_visual_set_alpha_mode(path, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")
    if dvz.dvz_visual_set_depth_test(path, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")

    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_VIEW
    if dvz.dvz_panel_add_visual(panel, path, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual(path) failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(1, 2, 3, 255))

    for join, cx, color in (
        (dvz.DVZ_PATH_JOIN_MITER, -0.63, ex.CYAN),
        (dvz.DVZ_PATH_JOIN_ROUND, +0.00, ex.GREEN),
        (dvz.DVZ_PATH_JOIN_BEVEL, +0.63, ex.YELLOW),
    ):
        _add_join_column(scene, panel, join, cx, color)
    return scene, figure, panel


def main() -> None:
    scene, figure, _panel = _build_scene()
    ex.run(scene, figure, "Path Join")


if __name__ == "__main__":
    main()
