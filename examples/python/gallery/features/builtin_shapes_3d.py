#!/usr/bin/env python3
"""Built-in 3D geometry builders rendered as retained meshes."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


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


def _y_up_transform(center):
    transform = ((ctypes.c_double * 4) * 4)()
    for i in range(4):
        transform[i][i] = 1.0
    transform[1][1] = 0.0
    transform[1][2] = -1.0
    transform[2][1] = +1.0
    transform[2][2] = 0.0
    transform[3][0] = center[0]
    transform[3][1] = center[1]
    transform[3][2] = center[2]
    return transform


def _add_y_up_geometry(scene, panel, geometry, center) -> None:
    if not geometry:
        raise RuntimeError("geometry creation failed")
    if dvz.dvz_geometry_transform(geometry, _y_up_transform(center)) != 0:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_geometry_transform() failed")
    _add_geometry(scene, panel, geometry)


def _cube(scene, panel) -> None:
    desc = dvz.dvz_geometry_cube_desc()
    desc.center[:] = (-0.92, 0.00, +0.42)
    desc.size = 0.58
    desc.color = ex.CYAN
    _add_geometry(scene, panel, dvz.dvz_geometry_cube(ctypes.byref(desc)))


def _sphere(scene, panel) -> None:
    desc = dvz.dvz_geometry_sphere_desc()
    desc.center[:] = (+0.00, 0.02, +0.42)
    desc.radius = 0.36
    desc.rings = 36
    desc.sectors = 72
    desc.color = ex.GREEN
    _add_geometry(scene, panel, dvz.dvz_geometry_sphere(ctypes.byref(desc)))


def _cylinder(scene, panel) -> None:
    desc = dvz.dvz_geometry_cylinder_desc()
    desc.radius = 0.24
    desc.height = 0.82
    desc.sectors = 128
    desc.color = ex.YELLOW
    _add_y_up_geometry(
        scene, panel, dvz.dvz_geometry_cylinder(ctypes.byref(desc)), (+0.92, 0.00, +0.42)
    )


def _cone(scene, panel) -> None:
    desc = dvz.dvz_geometry_cone_desc()
    desc.radius = 0.34
    desc.height = 0.86
    desc.sectors = 128
    desc.color = ex.TEXT
    _add_y_up_geometry(
        scene, panel, dvz.dvz_geometry_cone(ctypes.byref(desc)), (-0.92, 0.00, -0.42)
    )


def _torus(scene, panel) -> None:
    desc = dvz.dvz_geometry_torus_desc()
    desc.major_radius = 0.36
    desc.minor_radius = 0.105
    desc.rings = 72
    desc.sectors = 32
    desc.color = ex.BLUE
    _add_y_up_geometry(
        scene, panel, dvz.dvz_geometry_torus(ctypes.byref(desc)), (+0.00, 0.05, -0.42)
    )


def _arrow(scene, panel) -> None:
    desc = dvz.dvz_geometry_arrow_desc()
    desc.length = 1.00
    desc.shaft_radius = 0.075
    desc.head_radius = 0.20
    desc.head_length = 0.32
    desc.sectors = 128
    desc.color = ex.RED
    _add_y_up_geometry(
        scene, panel, dvz.dvz_geometry_arrow(ctypes.byref(desc)), (+0.92, 0.02, -0.42)
    )


def _add_shapes(scene, panel) -> None:
    _cube(scene, panel)
    _sphere(scene, panel)
    _cylinder(scene, panel)
    _cone(scene, panel)
    _torus(scene, panel)
    _arrow(scene, panel)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    _add_shapes(scene, panel)

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        angles = (ctypes.c_float * 3)(0.0, 0.0, 0.0)
        if dvz.dvz_arcball_set(arcball, angles) != 0:
            raise RuntimeError("dvz_arcball_set() failed")

    ex.run_with_view(scene, figure, "Builtin Shapes 3D", configure)


if __name__ == "__main__":
    main()
