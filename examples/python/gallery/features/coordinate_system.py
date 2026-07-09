#!/usr/bin/env python3
"""3D coordinate convention with colored axes, labels, and a reference grid."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


AXIS_LENGTH = 1.45

X_COLOR = dvz.DvzColor(230, 64, 64, 255)
Y_COLOR = dvz.DvzColor(52, 190, 88, 255)
Z_COLOR = dvz.DvzColor(66, 132, 255, 255)
ORIGIN_COLOR = dvz.DvzColor(230, 236, 244, 255)


def _identity_transform():
    transform = ((ctypes.c_double * 4) * 4)()
    for i in range(4):
        transform[i][i] = 1.0
    return transform


def _axis_transform(axis: int):
    transform = _identity_transform()
    if axis == 0:
        transform[0][0] = 0.0
        transform[0][1] = 0.0
        transform[0][2] = -1.0
        transform[1][0] = 0.0
        transform[1][1] = 1.0
        transform[1][2] = 0.0
        transform[2][0] = 1.0
        transform[2][1] = 0.0
        transform[2][2] = 0.0
    elif axis == 1:
        transform[0][0] = 1.0
        transform[0][1] = 0.0
        transform[0][2] = 0.0
        transform[1][0] = 0.0
        transform[1][1] = 0.0
        transform[1][2] = -1.0
        transform[2][0] = 0.0
        transform[2][1] = 1.0
        transform[2][2] = 0.0
    return transform


def _add_geometry(scene, panel, geometry) -> None:
    if not geometry:
        raise RuntimeError("geometry creation failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_mesh() failed")

    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
        ex.add_visual(panel, mesh)
    finally:
        dvz.dvz_geometry_destroy(geometry)


def _add_axis_arrow(scene, panel, axis: int, color) -> None:
    desc = dvz.dvz_geometry_arrow_desc()
    desc.center[:] = (0.0, 0.0, 0.5 * AXIS_LENGTH)
    desc.length = AXIS_LENGTH
    desc.shaft_radius = 0.035
    desc.head_radius = 0.105
    desc.head_length = 0.26
    desc.sectors = 64
    desc.color = color

    geometry = dvz.dvz_geometry_arrow(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_arrow() failed")
    if dvz.dvz_geometry_transform(geometry, _axis_transform(axis)) != 0:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_geometry_transform() failed")
    _add_geometry(scene, panel, geometry)


def _add_origin(scene, panel) -> None:
    desc = dvz.dvz_geometry_sphere_desc()
    desc.center[:] = (0.0, 0.0, 0.0)
    desc.radius = 0.075
    desc.rings = 24
    desc.sectors = 48
    desc.color = ORIGIN_COLOR
    _add_geometry(scene, panel, dvz.dvz_geometry_sphere(ctypes.byref(desc)))


def _add_reference_grid(panel) -> None:
    desc = dvz.dvz_reference_grid_desc()
    desc.plane = dvz.DVZ_REFERENCE_GRID_XZ
    desc.origin[:] = (0.0, 0.0, 0.0)
    desc.size[:] = (10.0, 10.0)
    desc.spacing = 0.25
    desc.major_every = 4
    desc.minor_color = dvz.DvzColor(74, 86, 98, 95)
    desc.major_color = dvz.DvzColor(116, 132, 148, 145)
    desc.axis_color = dvz.DvzColor(176, 190, 204, 185)
    desc.minor_width_px = 1.0
    desc.major_width_px = 1.5
    desc.axis_width_px = 2.0
    desc.depth_test = True
    grid = dvz.dvz_reference_grid(panel, ctypes.byref(desc))
    if not grid:
        raise RuntimeError("dvz_reference_grid() failed")


def _add_label(panel, string: bytes, position, color) -> None:
    text = dvz.dvz_text(panel, 0)
    if not text:
        raise RuntimeError("dvz_text() failed")

    style = dvz.dvz_text_style()
    style.size_px = 46.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (color.r, color.g, color.b, color.a)
    if dvz.dvz_text_set_style(text, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_WORLD
    placement.position[:] = tuple(float(v) for v in position)
    placement.offset[:] = (0.0, -8.0)
    placement.text_anchor[:] = (0.5, 0.5)
    placement.has_text_anchor = True
    placement.depth_test = False
    if dvz.dvz_text_set_placement(text, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")
    if dvz.dvz_text_set_string(text, string) != 0:
        raise RuntimeError("dvz_text_set_string() failed")


def _add_axis_labels(panel) -> None:
    pad = 0.18
    _add_label(panel, b"X", (AXIS_LENGTH + pad, 0.0, 0.0), X_COLOR)
    _add_label(panel, b"Y", (0.0, AXIS_LENGTH + pad, 0.0), Y_COLOR)
    _add_label(panel, b"Z", (0.0, 0.0, AXIS_LENGTH + pad), Z_COLOR)


def _camera_desc():
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (1.15, 1.75, 4.75)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.74
    camera.projection.near_clip = 0.005
    camera.projection.far_clip = 100.0
    return camera


def _add_scene(scene, panel) -> None:
    _add_reference_grid(panel)
    _add_axis_arrow(scene, panel, 0, X_COLOR)
    _add_axis_arrow(scene, panel, 1, Y_COLOR)
    _add_axis_arrow(scene, panel, 2, Z_COLOR)
    _add_origin(scene, panel)
    _add_axis_labels(panel)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    camera = _camera_desc()
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")
    _add_scene(scene, panel)

    def configure(view) -> None:
        desc = dvz.dvz_turntable_desc()
        desc.controller_flags = dvz.DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE
        desc.initial_view = camera.view
        desc.min_distance = 0.03
        desc.max_distance = 24.0
        turntable = dvz.dvz_view_turntable(view, panel, ctypes.byref(desc))
        if not turntable:
            raise RuntimeError("dvz_view_turntable() failed")

    ex.run_with_view(scene, figure, "Coordinate System", configure)


if __name__ == "__main__":
    main()
