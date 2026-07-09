#!/usr/bin/env python3
"""3D cube with an arcball controller and a passive orientation gizmo."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def _add_reference_grid(panel) -> None:
    desc = dvz.dvz_reference_grid_desc()
    desc.plane = dvz.DVZ_REFERENCE_GRID_XZ
    desc.origin[1] = -0.59
    desc.size[:] = (7.0, 7.0)
    desc.spacing = 0.25
    desc.major_every = 4
    desc.minor_color = dvz.DvzColor(74, 86, 98, 95)
    desc.major_color = dvz.DvzColor(116, 132, 148, 145)
    desc.axis_color = dvz.DvzColor(176, 190, 204, 185)
    desc.depth_test = True
    grid = dvz.dvz_reference_grid(panel, ctypes.byref(desc))
    if not grid:
        raise RuntimeError("dvz_reference_grid() failed")


def _add_orientation_gizmo(panel) -> None:
    desc = dvz.dvz_orientation_gizmo_desc()
    desc.placement = dvz.dvz_placement_panel_corner(
        dvz.DVZ_HORIZONTAL_ANCHOR_RIGHT,
        dvz.DVZ_VERTICAL_ANCHOR_BOTTOM,
        150.0,
        150.0,
        -18.0,
        -18.0,
    )
    gizmo = dvz.dvz_orientation_gizmo(panel, ctypes.byref(desc))
    if not gizmo:
        raise RuntimeError("dvz_orientation_gizmo() failed")


def _add_scene(scene, panel) -> None:
    ex.manual_camera(panel)
    _add_reference_grid(panel)
    ex.add_cube_mesh(scene, panel, size=1.18)
    _add_orientation_gizmo(panel)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _add_scene(scene, panel)

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        angles = (ctypes.c_float * 3)(0.0, 0.0, 0.0)
        if dvz.dvz_arcball_set(arcball, angles) != 0:
            raise RuntimeError("dvz_arcball_set() failed")

    ex.run_with_view(scene, figure, "Orientation Gizmo", configure)


if __name__ == "__main__":
    main()
