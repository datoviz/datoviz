#!/usr/bin/env python3
"""Contour isolines extracted from a synthetic surface grid."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


GRID_ROWS = 42
GRID_COLS = 50
LEVEL_COUNT = 9


def _surface_data():
    count = GRID_ROWS * GRID_COLS
    values = np.zeros(count, dtype=np.float64)
    colors = np.zeros((count, 4), dtype=np.uint8)

    for row in range(GRID_ROWS):
        y = -0.775 + row * (1.55 / (GRID_ROWS - 1))
        for col in range(GRID_COLS):
            x = -1.05 + col * (2.10 / (GRID_COLS - 1))
            idx = row * GRID_COLS + col
            r2 = x * x + y * y
            z = 0.24 * np.exp(-1.65 * r2) + 0.055 * np.sin(8.0 * x) * np.cos(6.0 * y)
            values[idx] = z

            t = min(max((z + 0.09) / 0.38, 0.0), 1.0)
            colors[idx, 0] = int((1.0 - t) * ex.GREEN.r + t * ex.CYAN.r)
            colors[idx, 1] = int((1.0 - t) * ex.GREEN.g + t * ex.CYAN.g)
            colors[idx, 2] = int((1.0 - t) * ex.GREEN.b + t * ex.CYAN.b)
            colors[idx, 3] = 230

    return values, colors


def _surface_geometry(values, colors):
    desc = dvz.dvz_geometry_surface_grid_desc()
    desc.rows = GRID_ROWS
    desc.cols = GRID_COLS
    desc.heights = values.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    desc.colors = colors.ctypes.data_as(ctypes.POINTER(dvz.DvzColor))
    desc.origin[:] = (-1.05, -0.775, 0.0)
    desc.col_basis[:] = (2.10 / (GRID_COLS - 1), 0.0, 0.0)
    desc.row_basis[:] = (0.0, 1.55 / (GRID_ROWS - 1), 0.0)
    geometry = dvz.dvz_geometry_surface_grid(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_surface_grid() failed")
    return geometry


def _add_surface(scene, panel, geometry) -> None:
    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
        raise RuntimeError("dvz_mesh_set_geometry(surface) failed")
    ex.add_visual(panel, mesh)


def _extract_contours(geometry, values):
    levels = np.array([-0.030 + 0.035 * i for i in range(LEVEL_COUNT)], dtype=np.float64)
    contours = dvz.dvz_geometry_contours(
        geometry,
        values.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        values.size,
        levels.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        levels.size,
    )
    if not contours:
        raise RuntimeError("dvz_geometry_contours() failed")
    return contours


def _add_contours(scene, panel, contours) -> None:
    data = contours.contents
    count = int(data.segment_count)
    if count == 0:
        raise RuntimeError("empty contour extraction")

    starts = np.zeros((count, 3), dtype=np.float32)
    ends = np.zeros((count, 3), dtype=np.float32)
    colors = np.zeros((count, 4), dtype=np.uint8)
    widths = np.zeros(count, dtype=np.float32)

    for i in range(count):
        segment = data.segments[i]
        starts[i] = (segment.p0[0], segment.p0[1], segment.p0[2] + 0.010)
        ends[i] = (segment.p1[0], segment.p1[1], segment.p1[2] + 0.010)
        major = segment.level_index == LEVEL_COUNT // 2
        color = ex.TEXT if major else ex.YELLOW
        colors[i] = (color.r, color.g, color.b, color.a)
        widths[i] = 4.0 if major else 2.0

    visual = dvz.dvz_segment(scene, 0)
    if not visual:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(contours) failed")
    if dvz.dvz_segment_set_caps(visual, dvz.DVZ_SEGMENT_CAP_BUTT, dvz.DVZ_SEGMENT_CAP_BUTT) != 0:
        raise RuntimeError("dvz_segment_set_caps() failed")
    ex.add_visual(panel, visual)


def _add_isolines(scene, panel) -> None:
    values, colors = _surface_data()
    geometry = _surface_geometry(values, colors)
    contours = None
    try:
        contours = _extract_contours(geometry, values)
        _add_surface(scene, panel, geometry)
        _add_contours(scene, panel, contours)
    finally:
        if contours:
            dvz.dvz_geometry_contours_destroy(contours)
        dvz.dvz_geometry_destroy(geometry)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    _add_isolines(scene, panel)

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        angles = (ctypes.c_float * 3)(+0.58, -0.12, +0.26)
        if dvz.dvz_arcball_set(arcball, angles) != 0:
            raise RuntimeError("dvz_arcball_set() failed")

    ex.run_with_view(scene, figure, "Isolines", configure)


if __name__ == "__main__":
    main()
