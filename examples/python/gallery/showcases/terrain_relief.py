#!/usr/bin/env python3
"""Prepared USGS elevation with aligned NAIP imagery."""

from __future__ import annotations

import ctypes
import struct
from pathlib import Path

import numpy as np
from PIL import Image

import datoviz as dvz
from examples.python.gallery import common as ex


BUNDLE_DIRS = (Path("data/examples/terrain_relief/prepared"), Path(".cache/datoviz/examples/terrain_relief/prepared"))
HEADER = struct.Struct("<8sIIIffff")
DISPLAY_WIDTH = 5.0
EXAGGERATION = 1.35
INITIAL_ANGLES = (ctypes.c_float * 3)(+0.050954, -0.163512, -0.032719)


def _load():
    directory = next((path for path in BUNDLE_DIRS if (path / "terrain.bin").exists() and (path / "terrain.jpg").exists()), None)
    if directory is None:
        raise FileNotFoundError("missing prepared USGS terrain; run `uv run tools/data/prepare_terrain_relief.py`")
    payload = (directory / "terrain.bin").read_bytes()
    magic, version, rows, cols, width_m, depth_m, elevation_min, elevation_max = HEADER.unpack_from(payload)
    if magic[:7] != b"DVZTRN1" or version != 1 or rows < 2 or cols < 2:
        raise ValueError("invalid prepared terrain header")
    if HEADER.size + rows * cols * np.dtype("<f4").itemsize != len(payload):
        raise ValueError("unexpected prepared terrain size")
    heights = np.frombuffer(payload, "<f4", rows * cols, HEADER.size).astype(np.float64)
    heights -= elevation_min
    texture = np.asarray(Image.open(directory / "terrain.jpg").convert("RGBA"), dtype=np.uint8)
    return rows, cols, width_m, depth_m, heights, texture


def _build_scene():
    rows, cols, width_m, depth_m, heights, texture = _load()
    horizontal_scale = DISPLAY_WIDTH / width_m
    display_depth = depth_m * horizontal_scale
    desc = dvz.dvz_geometry_surface_grid_desc()
    desc.rows, desc.cols = rows, cols
    desc.heights = heights.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    desc.color = dvz.DvzColor(255, 255, 255, 255)
    desc.origin[:] = (-0.5 * DISPLAY_WIDTH, 0.0, +0.5 * display_depth)
    desc.col_basis[:] = (DISPLAY_WIDTH / (cols - 1), 0.0, 0.0)
    desc.row_basis[:] = (0.0, 0.0, -display_depth / (rows - 1))
    desc.height_axis[:] = (0.0, 1.0, 0.0)
    desc.height_scale = horizontal_scale * EXAGGERATION
    geometry = dvz.dvz_geometry_surface_grid(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_surface_grid() failed")

    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(6, 9, 10, 255))
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (+4.95, +3.95, +5.70)
    camera.view.target[:] = (0.0, +0.38, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.56
    camera.projection.near_clip = 0.03
    camera.projection.far_clip = 100.0
    dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera))

    mesh = dvz.dvz_mesh(scene, 0)
    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)
    material = dvz.dvz_phong_material_desc()
    material.light_direction[:] = (-0.38, +0.76, +0.52)
    material.phong.ambient = 0.734
    material.phong.diffuse = 0.454
    material.phong.specular = 0.049
    material.phong.shininess = 7.594
    dvz.dvz_visual_set_material(mesh, ctypes.byref(material))
    field = dvz.dvz_sampled_field_from_array(scene, texture)
    if not field or dvz.dvz_visual_set_field(mesh, b"texture", field) != 0:
        raise RuntimeError("terrain texture setup failed")
    ex.add_visual(panel, mesh)
    return scene, figure, panel, rows, cols


def main() -> None:
    scene, figure, panel, rows, cols = _build_scene()
    print(f"terrain_relief: {cols} x {rows} prepared elevation grid")

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball or dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("arcball setup failed")
        dvz.dvz_arcball_zoom(arcball, 0.606531)
        pan = (ctypes.c_float * 2)(-0.191645, +0.730556)
        dvz.dvz_arcball_pan(arcball, pan)

    ex.run_with_view(scene, figure, "McHenrys Peak Terrain Relief", configure)


if __name__ == "__main__":
    main()
