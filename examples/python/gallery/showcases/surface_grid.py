#!/usr/bin/env python3
"""Generated height field rendered as a lit mesh with a wireframe overlay."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SURFACE_ROWS = 80
SURFACE_COLS = 96
INITIAL_ANGLES = (ctypes.c_float * 3)(0.58, -0.18, 0.22)

PANEL_BG = dvz.DvzColor(13, 18, 24, 255)
COLOR_LOW = dvz.DvzColor(42, 132, 148, 255)
COLOR_HIGH = dvz.DvzColor(88, 214, 238, 255)
WIREFRAME = dvz.DvzColor(14, 34, 38, 92)


def _surface_data():
    rows = np.arange(SURFACE_ROWS, dtype=np.float64)
    cols = np.arange(SURFACE_COLS, dtype=np.float64)
    y = -1.0 + 2.0 * rows[:, None] / (SURFACE_ROWS - 1)
    x = -1.0 + 2.0 * cols[None, :] / (SURFACE_COLS - 1)

    r2 = x * x + y * y
    ridge = 0.34 * np.cos(15.0 * np.sqrt(r2)) * np.exp(-1.25 * r2)
    saddle = 0.13 * (x * x - 0.70 * y * y) * np.exp(-0.85 * r2)
    ripple = 0.050 * np.sin(15.5 * x + 2.0 * y) * np.cos(10.0 * y - 2.5 * x)
    heights = (ridge + saddle + ripple).astype(np.float64)

    t = np.clip((heights + 0.26) / 0.58, 0.0, 1.0)
    colors = np.zeros((SURFACE_ROWS, SURFACE_COLS, 4), dtype=np.uint8)
    colors[..., 0] = ((1.0 - t) * COLOR_LOW.r + t * COLOR_HIGH.r).astype(np.uint8)
    colors[..., 1] = ((1.0 - t) * COLOR_LOW.g + t * COLOR_HIGH.g).astype(np.uint8)
    colors[..., 2] = ((1.0 - t) * COLOR_LOW.b + t * COLOR_HIGH.b).astype(np.uint8)
    colors[..., 3] = 235
    return np.ascontiguousarray(heights.ravel()), np.ascontiguousarray(colors.reshape(-1, 4))


def _grid_positions(heights):
    rows = np.arange(SURFACE_ROWS, dtype=np.float64)
    cols = np.arange(SURFACE_COLS, dtype=np.float64)
    row, col = np.meshgrid(rows, cols, indexing="ij")

    positions = np.zeros((SURFACE_ROWS * SURFACE_COLS, 3), dtype=np.float64)
    positions[:, 0] = -2.70 + col.ravel() * (2.0 * 2.70 / (SURFACE_COLS - 1))
    positions[:, 1] = 1.18 * heights
    positions[:, 2] = +2.00 - row.ravel() * (2.0 * 2.00 / (SURFACE_ROWS - 1))
    return positions


def _surface_geometry(heights, colors):
    desc = dvz.dvz_geometry_surface_grid_desc()
    desc.rows = SURFACE_ROWS
    desc.cols = SURFACE_COLS
    desc.heights = heights.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    desc.colors = colors.ctypes.data_as(ctypes.POINTER(dvz.DvzColor))
    desc.origin[:] = (-2.70, 0.0, +2.00)
    desc.col_basis[:] = (2.0 * 2.70 / (SURFACE_COLS - 1), 0.0, 0.0)
    desc.row_basis[:] = (0.0, 0.0, -2.0 * 2.00 / (SURFACE_ROWS - 1))
    desc.height_axis[:] = (0.0, 1.0, 0.0)
    desc.height_scale = 1.18
    geometry = dvz.dvz_geometry_surface_grid(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_surface_grid() failed")
    return geometry


def _surface_material():
    material = dvz.dvz_phong_material_desc()
    material.phong.ambient = 0.34
    material.phong.diffuse = 0.72
    material.phong.specular = 0.22
    material.phong.shininess = 40.0
    material.light_direction[:] = (-0.32, -0.58, -0.74)
    return material


def _add_surface(scene, panel, geometry) -> None:
    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    material = _surface_material()
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
        raise RuntimeError("dvz_mesh_set_geometry() failed")
    ex.add_visual(panel, mesh)


def _add_wireframe(scene, panel, geometry, positions) -> None:
    edges = dvz.dvz_geometry_edges(geometry)
    if not edges:
        raise RuntimeError("dvz_geometry_edges() failed")

    try:
        edge_data = edges.contents
        count = int(edge_data.edge_count)
        if count == 0:
            raise RuntimeError("empty geometry edge list")

        starts = np.zeros((count, 3), dtype=np.float32)
        ends = np.zeros((count, 3), dtype=np.float32)
        colors = np.empty((count, 4), dtype=np.uint8)
        widths = np.full(count, 1.10, dtype=np.float32)

        for i in range(count):
            edge = edge_data.edges[i]
            starts[i] = positions[int(edge.v0)]
            ends[i] = positions[int(edge.v1)]
            starts[i, 1] += 0.003
            ends[i, 1] += 0.003

        colors[:] = (WIREFRAME.r, WIREFRAME.g, WIREFRAME.b, WIREFRAME.a)

        wire = dvz.dvz_segment(scene, 0)
        if not wire:
            raise RuntimeError("dvz_segment() failed")
        if dvz.dvz_visual_set_data_many(
            wire,
            {
                "position_start": starts,
                "position_end": ends,
                "color": colors,
                "stroke_width_px": widths,
            },
        ) != 0:
            raise RuntimeError("dvz_visual_set_data_many(wireframe) failed")
        if dvz.dvz_segment_set_caps(wire, dvz.DVZ_SEGMENT_CAP_BUTT, dvz.DVZ_SEGMENT_CAP_BUTT) != 0:
            raise RuntimeError("dvz_segment_set_caps() failed")
        if dvz.dvz_visual_set_alpha_mode(wire, dvz.DVZ_ALPHA_BLENDED) != 0:
            raise RuntimeError("dvz_visual_set_alpha_mode(wireframe) failed")
        ex.add_visual(panel, wire)
    finally:
        dvz.dvz_geometry_edges_destroy(edges)


def _setup_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (3.25, 2.25, 3.20)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.58
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _build_scene():
    heights, colors = _surface_data()
    positions = _grid_positions(heights)
    geometry = _surface_geometry(heights, colors)

    try:
        scene, figure, panel = ex.scene_panel()
        dvz.dvz_panel_set_background_color(panel, PANEL_BG)
        _setup_camera(panel)
        _add_surface(scene, panel, geometry)
        _add_wireframe(scene, panel, geometry, positions)
        return scene, figure, panel
    finally:
        dvz.dvz_geometry_destroy(geometry)


def _configure_view(view, scene, panel) -> None:
    arcball = dvz.dvz_view_arcball(view, panel, None)
    if not arcball:
        raise RuntimeError("dvz_view_arcball() failed")
    if dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
        raise RuntimeError("dvz_arcball_set() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Surface Grid", configure)


if __name__ == "__main__":
    main()
