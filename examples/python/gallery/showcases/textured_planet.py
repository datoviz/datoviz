#!/usr/bin/env python3
"""Real planetary texture mapped onto an indexed sphere mesh."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

import datoviz as dvz

from examples.python.gallery import common as ex


TEXTURE_WIDTH = 1024
TEXTURE_HEIGHT = 512
EARTH_TEXTURE_PATH = Path("data/assets/textures/world.200412.3x5400x2700.jpg")
SPHERE_RADIUS = 0.92
SPHERE_SECTORS = 96
SPHERE_RINGS = 48
ROTATION_SPEED_RAD_PER_SEC = 0.16
STAR_COUNT = 900
STAR_RADIUS = 48.0
SUN_DIR = (-0.80, +0.22, +0.55)
TAU = 2.0 * np.pi

PANEL_BG = dvz.DvzColor(2, 2, 4, 255)


@dataclass
class PlanetState:
    spin_rotation: object | None = None
    spin_animation: object | None = None

    def destroy_tracks(self) -> None:
        if self.spin_rotation:
            dvz.dvz_track_destroy(self.spin_rotation)
        self.spin_rotation = None
        self.spin_animation = None


def _earth_texture() -> np.ndarray:
    if EARTH_TEXTURE_PATH.exists():
        with Image.open(EARTH_TEXTURE_PATH) as image:
            image = image.convert("RGBA")
            width, height = image.size
            if width == 2 * height:
                return np.ascontiguousarray(np.asarray(image, dtype=np.uint8))

    y = np.linspace(0.0, 1.0, TEXTURE_HEIGHT, dtype=np.float64)
    x = np.linspace(0.0, 1.0, TEXTURE_WIDTH, dtype=np.float64)
    u, v = np.meshgrid(x, y)
    lat = (0.5 - v) * np.pi
    lon = (u - 0.5) * TAU
    bands = (
        np.sin(12.0 * lon + 1.7 * np.sin(5.0 * lat))
        + 0.72 * np.sin(9.0 * lat + 2.2 * np.cos(3.0 * lon))
        + 0.38 * np.sin(21.0 * (lon + lat))
    )
    ridge = np.sin(34.0 * lon) * np.sin(16.0 * lat)
    land = bands + 0.28 * ridge > 0.38
    ice = np.abs(lat) > 1.22
    grid_u = np.abs(u * 24.0 - np.floor(u * 24.0 + 0.5))
    grid_v = np.abs(v * 12.0 - np.floor(v * 12.0 + 0.5))
    grid = (grid_u < 0.015) | (grid_v < 0.015)

    r = np.full_like(u, 0.05)
    g = np.full_like(u, 0.13)
    b = np.full_like(u, 0.27)

    warm = np.clip(0.5 + 0.5 * np.sin(5.0 * lon - 7.0 * lat), 0.0, 1.0)
    latitude_weight = 1.0 - np.abs(lat) / (0.5 * np.pi)
    r = np.where(land, 0.20 + 0.38 * warm, r)
    g = np.where(land, 0.34 + 0.32 * latitude_weight, g)
    b = np.where(land, 0.16 + 0.12 * (1.0 - warm), b)

    water = 0.5 + 0.5 * np.sin(8.0 * lon + 5.0 * lat)
    r = np.where(~land, 0.03 + 0.05 * water, r)
    g = np.where(~land, 0.20 + 0.18 * water, g)
    b = np.where(~land, 0.44 + 0.22 * water, b)

    r = np.where(ice, 0.84, r)
    g = np.where(ice, 0.90, g)
    b = np.where(ice, 0.93, b)
    r = np.where(grid, 0.92, r)
    g = np.where(grid, 0.94, g)
    b = np.where(grid, 0.86, b)

    rgba = np.empty((TEXTURE_HEIGHT, TEXTURE_WIDTH, 4), dtype=np.uint8)
    rgba[..., 0] = np.clip(255.0 * r + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 1] = np.clip(255.0 * g + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 2] = np.clip(255.0 * b + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 3] = 255
    return rgba


def _planet_transform():
    transform = ((ctypes.c_double * 4) * 4)()
    transform[0][1] = -1.0
    transform[1][2] = +1.0
    transform[2][0] = -1.0
    transform[3][3] = 1.0
    return transform


def _add_star_shell(scene, panel) -> None:
    rng = np.random.default_rng(0xD42024)
    z = 2.0 * rng.random(STAR_COUNT, dtype=np.float32) - 1.0
    phi = TAU * rng.random(STAR_COUNT, dtype=np.float32)
    radius = np.sqrt(np.maximum(0.0, 1.0 - z * z))
    positions = np.column_stack(
        (
            STAR_RADIUS * radius * np.cos(phi),
            STAR_RADIUS * radius * np.sin(phi),
            STAR_RADIUS * z,
        )
    ).astype(np.float32)
    brightness = 0.45 + 0.55 * rng.random(STAR_COUNT, dtype=np.float32)
    colors = np.empty((STAR_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = np.clip(255.0 * 0.82 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 1] = np.clip(255.0 * 0.88 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 2] = np.clip(255.0 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 3] = 255
    sizes = (1.0 + 2.2 * rng.random(STAR_COUNT) * rng.random(STAR_COUNT)).astype(np.float32)

    positions[0] = (STAR_RADIUS * SUN_DIR[0], STAR_RADIUS * SUN_DIR[1], STAR_RADIUS * SUN_DIR[2])
    colors[0] = (255, 244, 214, 255)
    sizes[0] = 14.0

    stars = dvz.dvz_point(scene, 0)
    if not stars:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        stars,
        {
            "position": positions,
            "color": colors,
            "diameter_px": sizes,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(stars) failed")
    ex.set_filled_point_style(stars)
    if dvz.dvz_visual_set_depth_test(stars, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(stars) failed")
    ex.add_visual(panel, stars)


def _planet_material():
    material = dvz.dvz_phong_material_desc()
    material.light_direction[:] = SUN_DIR
    material.phong.ambient = 0.16
    material.phong.diffuse = 0.92
    material.phong.specular = 0.015
    material.phong.shininess = 18.0
    return material


def _add_planet(scene, panel):
    field = dvz.dvz_sampled_field_from_array(scene, _earth_texture())

    desc = dvz.dvz_geometry_sphere_desc()
    desc.radius = SPHERE_RADIUS
    desc.sectors = SPHERE_SECTORS
    desc.rings = SPHERE_RINGS
    desc.color = ex.WHITE
    geometry = dvz.dvz_geometry_sphere(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_sphere() failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_mesh() failed")

    try:
        if dvz.dvz_geometry_transform(geometry, _planet_transform()) != 0:
            raise RuntimeError("dvz_geometry_transform() failed")
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)

    material = _planet_material()
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    if dvz.dvz_visual_set_field(mesh, b"texture", field) != 0:
        raise RuntimeError("dvz_visual_set_field(texture) failed")
    ex.add_visual(panel, mesh)
    return mesh


def _setup_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.0, 0.0, 3.0)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.72
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _add_spin(scene, mesh, state: PlanetState) -> None:
    rotation_desc = dvz.dvz_track_rotation_desc()
    rotation_desc.axis[:] = (0.0, 1.0, 0.0)
    rotation_desc.speed_rad_per_sec = 1.0
    rotation = dvz.dvz_track_rotation(ctypes.byref(rotation_desc))
    if not rotation:
        raise RuntimeError("dvz_track_rotation() failed")
    state.spin_rotation = rotation

    transform_desc = dvz.dvz_transform_motion_desc()
    transform_desc.rotation = rotation
    animation = dvz.dvz_anim_visual_transform(scene, mesh, ctypes.byref(transform_desc))
    if not animation:
        raise RuntimeError("dvz_anim_visual_transform() failed")
    state.spin_animation = animation
    if dvz.dvz_anim_set_speed(animation, ROTATION_SPEED_RAD_PER_SEC) != 0:
        raise RuntimeError("dvz_anim_set_speed() failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start() failed")


def _build_scene(spin: bool = False):
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    _setup_camera(panel)
    _add_star_shell(scene, panel)
    mesh = _add_planet(scene, panel)
    state = PlanetState()
    if spin:
        _add_spin(scene, mesh, state)
    return scene, figure, panel, mesh, state


def _configure_view(view, panel) -> None:
    desc = dvz.dvz_turntable_desc()
    desc.min_distance = 1.45
    desc.max_distance = 7.50
    desc.zoom_speed = 0.018
    turntable = dvz.dvz_view_turntable(view, panel, ctypes.byref(desc))
    if not turntable:
        raise RuntimeError("dvz_view_turntable() failed")


def main() -> None:
    scene, figure, panel, _mesh, state = _build_scene(spin=True)

    def configure(view) -> None:
        _configure_view(view, panel)

    try:
        ex.run_with_view(scene, figure, "Textured Planets", configure)
    finally:
        state.destroy_tracks()


if __name__ == "__main__":
    main()
