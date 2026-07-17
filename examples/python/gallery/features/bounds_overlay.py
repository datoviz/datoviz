#!/usr/bin/env python3
"""Diagnostic bounds overlays for 2D points and 3D spheres."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 256
SPHERE_COUNT = 32
SPHERE_RADIUS_SCALE = 1.5


def _point_data():
    t = np.linspace(0.0, 1.0, POINT_COUNT, dtype=np.float32)
    angle = np.float32(28.274333882308138) * t
    radius = 0.08 + 0.82 * t
    positions = np.column_stack(
        (radius * np.cos(angle), radius * np.sin(angle), np.zeros(POINT_COUNT, dtype=np.float32))
    ).astype(np.float32)

    colors = np.empty((POINT_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = (42.0 + 180.0 * t).astype(np.uint8)
    colors[:, 1] = (210.0 - 72.0 * t).astype(np.uint8)
    colors[:, 2] = (255.0 - 165.0 * t).astype(np.uint8)
    colors[:, 3] = 235
    diameters = (8.0 + 22.0 * t).astype(np.float32)
    return positions, colors, diameters


def _sphere_data():
    positions = np.zeros((SPHERE_COUNT, 3), dtype=np.float32)
    colors = np.zeros((SPHERE_COUNT, 4), dtype=np.uint8)
    radii = np.zeros(SPHERE_COUNT, dtype=np.float32)
    radius_classes = np.array([0.070, 0.105, 0.165], dtype=np.float32)

    for i in range(SPHERE_COUNT):
        ix = i % 4
        iy = (i // 4) % 4
        iz = i // 16
        jx = 0.035 * np.sin(1.7 * i)
        jy = 0.035 * np.cos(2.1 * i)
        jz = 0.055 * np.sin(0.9 * i)
        positions[i] = (-0.54 + 0.36 * ix + jx, -0.54 + 0.36 * iy + jy, -0.36 + 0.72 * iz + jz)

        t = i / (SPHERE_COUNT - 1)
        colors[i] = (
            int(230.0 - 120.0 * t),
            int(80.0 + 120.0 * t),
            int(120.0 + 100.0 * t),
            255,
        )
        radius_class = (i * 7 + iz) % 3
        radii[i] = SPHERE_RADIUS_SCALE * (
            radius_classes[radius_class] + 0.006 * np.sin(0.8 * i)
        )
    return positions, colors, radii


def _add_points(scene, panel) -> None:
    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    positions, colors, diameters = _point_data()
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


def _add_spheres(scene, panel) -> None:
    sphere = dvz.dvz_sphere(scene, 0)
    if not sphere:
        raise RuntimeError("dvz_sphere() failed")
    if dvz.dvz_sphere_set_mode(sphere, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")
    positions, colors, radii = _sphere_data()
    if dvz.dvz_visual_set_data_many(
        sphere,
        {
            "position": positions,
            "color": colors,
            "radius": radii,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(sphere) failed")
    ex.add_visual(panel, sphere)


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    panel_2d = ex.panel_rect(figure, 0.04, 0.08, 0.44, 0.84)
    panel_3d = ex.panel_rect(figure, 0.52, 0.08, 0.44, 0.84)
    ex.manual_camera(panel_3d)

    _add_points(scene, panel_2d)
    _add_spheres(scene, panel_3d)
    if dvz.dvz_panel_set_bounds_visible(panel_2d, True) != 0:
        raise RuntimeError("dvz_panel_set_bounds_visible(2d) failed")
    if dvz.dvz_panel_set_bounds_visible(panel_3d, True) != 0:
        raise RuntimeError("dvz_panel_set_bounds_visible(3d) failed")
    return scene, figure, panel_2d, panel_3d


def main() -> None:
    scene, figure, panel_2d, panel_3d = _build_scene()

    def configure(view) -> None:
        ex.bind_panzoom(view, scene, panel_2d, dvz.DVZ_DIM_MASK_XY)
        arcball = dvz.dvz_view_arcball(view, panel_3d, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")

    ex.run_with_view(scene, figure, "Bounds Overlay", configure)


if __name__ == "__main__":
    main()
