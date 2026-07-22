#!/usr/bin/env python3
"""Procedural RGBA texture mapped onto a retained UV-sphere mesh."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


TEXTURE_WIDTH = 1024
TEXTURE_HEIGHT = 512
TAU = 2.0 * np.pi


def _texture() -> np.ndarray:
    x = np.linspace(0.0, 1.0, TEXTURE_WIDTH, endpoint=False, dtype=np.float32)
    y = np.linspace(0.0, 1.0, TEXTURE_HEIGHT, dtype=np.float32)
    u, v = np.meshgrid(x, y)

    pole = np.clip(np.sin(np.pi * v), 0.0, 1.0) ** 1.75
    lon = pole * np.sin(TAU * 8.0 * u)
    wave = pole * np.sin(TAU * (3.0 * u + 1.5 * v))
    lat = 0.5 + 0.5 * np.cos(TAU * 4.0 * v)
    polar_tint = 1.0 - 0.32 * pole
    value = polar_tint * (0.48 + 0.28 * lat + 0.16 * lon + 0.08 * wave)

    rgba = np.empty((TEXTURE_HEIGHT, TEXTURE_WIDTH, 4), dtype=np.uint8)
    rgba[..., 0] = np.clip(18.0 + 58.0 * value, 0, 255).astype(np.uint8)
    rgba[..., 1] = np.clip(58.0 + 170.0 * value, 0, 255).astype(np.uint8)
    rgba[..., 2] = np.clip(96.0 + 144.0 * value, 0, 255).astype(np.uint8)
    rgba[..., 3] = 255
    return rgba


def _add_textured_mesh(scene, panel) -> None:
    field = dvz.dvz_sampled_field_from_array(scene, _texture())

    desc = dvz.dvz_geometry_sphere_desc()
    desc.radius = 0.82
    desc.sectors = 128
    desc.rings = 64
    desc.color = ex.WHITE
    geometry = dvz.dvz_geometry_sphere(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_sphere() failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_mesh() failed")

    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)

    if dvz.dvz_visual_set_field(mesh, b"texture", field) != 0:
        raise RuntimeError("dvz_visual_set_field(texture) failed")
    sampling = dvz.dvz_field_sampling_desc()
    sampling.min_filter = dvz.DVZ_FIELD_FILTER_LINEAR
    sampling.mag_filter = dvz.DVZ_FIELD_FILTER_LINEAR
    if dvz.dvz_visual_set_field_sampling(mesh, b"texture", ctypes.byref(sampling)) != 0:
        raise RuntimeError("dvz_visual_set_field_sampling(texture) failed")
    ex.add_visual(panel, mesh)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    _add_textured_mesh(scene, panel)

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        angles = (ctypes.c_float * 3)(0.0, 0.0, 0.0)
        if dvz.dvz_arcball_set(arcball, angles) != 0:
            raise RuntimeError("dvz_arcball_set() failed")

    ex.run_with_view(scene, figure, "Textured Mesh", configure)


if __name__ == "__main__":
    main()
