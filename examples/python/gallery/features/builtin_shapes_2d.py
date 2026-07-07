#!/usr/bin/env python3
"""Built-in 2D geometry builders rendered as retained meshes."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def _set_equal_view2d(panel) -> None:
    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.04
    desc.domain_x[:] = (-1.05, +1.05)
    desc.domain_y[:] = (-0.72, +0.72)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


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


def _plane(scene, panel) -> None:
    desc = dvz.dvz_geometry_plane_desc()
    desc.center[:] = (-0.66, +0.40, 0.0)
    desc.width = 0.46
    desc.height = 0.30
    desc.color = ex.CYAN
    _add_geometry(scene, panel, dvz.dvz_geometry_plane(ctypes.byref(desc)))


def _disc(scene, panel) -> None:
    desc = dvz.dvz_geometry_disc_desc()
    desc.center[:] = (0.0, +0.40, 0.01)
    desc.radius = 0.21
    desc.segments = 48
    desc.color = ex.GREEN
    _add_geometry(scene, panel, dvz.dvz_geometry_disc(ctypes.byref(desc)))


def _sector(scene, panel) -> None:
    desc = dvz.dvz_geometry_sector_desc()
    desc.center[:] = (+0.66, +0.40, 0.02)
    desc.radius = 0.27
    desc.start_angle = -0.35
    desc.sweep_angle = 4.4
    desc.segments = 36
    desc.color = ex.YELLOW
    _add_geometry(scene, panel, dvz.dvz_geometry_sector(ctypes.byref(desc)))


def _regular_polygon(scene, panel) -> None:
    desc = dvz.dvz_geometry_regular_polygon_desc()
    desc.center[:] = (-0.66, -0.30, 0.03)
    desc.radius = 0.25
    desc.sides = 7
    desc.color = ex.TEXT
    _add_geometry(scene, panel, dvz.dvz_geometry_regular_polygon(ctypes.byref(desc)))


def _star(scene, panel) -> None:
    desc = dvz.dvz_geometry_star_desc()
    desc.center[:] = (0.0, -0.30, 0.04)
    desc.outer_radius = 0.28
    desc.inner_radius = 0.12
    desc.points = 5
    desc.color = ex.RED
    _add_geometry(scene, panel, dvz.dvz_geometry_star(ctypes.byref(desc)))


def _hole_polygon(scene, panel) -> None:
    outer = np.array(
        [
            [+0.46, -0.56, 0.05],
            [+0.88, -0.50, 0.05],
            [+0.84, -0.12, 0.05],
            [+0.54, -0.02, 0.05],
            [+0.36, -0.28, 0.05],
        ],
        dtype=np.float32,
    )
    hole = np.array(
        [
            [+0.58, -0.38, 0.05],
            [+0.72, -0.36, 0.05],
            [+0.70, -0.22, 0.05],
            [+0.56, -0.22, 0.05],
        ],
        dtype=np.float32,
    )
    positions = np.vstack((outer, hole)).astype(np.float32)
    colors = ex.color_array(*(ex.BLUE for _ in range(len(positions))))
    indices = np.array(
        [
            0, 1, 6,
            0, 6, 5,
            1, 2, 7,
            1, 7, 6,
            2, 3, 8,
            2, 8, 7,
            3, 4, 8,
            4, 5, 8,
            4, 0, 5,
        ],
        dtype=np.uint32,
    )

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    if dvz.dvz_visual_set_data_many(mesh, {"position": positions, "color": colors}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(holed polygon) failed")
    if dvz.dvz_visual_set_index_data(mesh, indices) != 0:
        raise RuntimeError("dvz_visual_set_index_data(holed polygon) failed")
    ex.add_visual(panel, mesh)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _set_equal_view2d(panel)

    _plane(scene, panel)
    _disc(scene, panel)
    _sector(scene, panel)
    _regular_polygon(scene, panel)
    _star(scene, panel)
    _hole_polygon(scene, panel)

    def configure_view(view) -> None:
        desc = dvz.dvz_panzoom_desc()
        desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_KEEP_ASPECT
        controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
        if not controller:
            raise RuntimeError("dvz_panzoom() failed")
        if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
            raise RuntimeError("dvz_view_bind_controller() failed")

    ex.run_with_view(scene, figure, "Builtin Shapes 2D", configure_view)


if __name__ == "__main__":
    main()
