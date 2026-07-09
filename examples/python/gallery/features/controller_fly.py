#!/usr/bin/env python3
"""Fly-style camera navigation through a 3D cube scene."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def _controller_camera():
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.0, 3.0, 5.0)
    camera.view.target[:] = (0.0, 0.0, 0.3)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.66
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    return camera


def _add_reference_grid(panel) -> None:
    desc = dvz.dvz_reference_grid_desc()
    desc.plane = dvz.DVZ_REFERENCE_GRID_XZ
    desc.origin[1] = -0.55
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


def _add_scene(scene, panel):
    camera = _controller_camera()
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")
    _add_reference_grid(panel)
    ex.add_cube_mesh(scene, panel, size=1.10)
    return camera


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    camera = _add_scene(scene, panel)

    def configure(view) -> None:
        desc = dvz.dvz_fly_desc()
        desc.mode = dvz.DVZ_FLY_MODE_PLANE
        desc.initial_view = camera.view
        desc.speed = 0.70
        desc.look_speed = 0.45
        fly = dvz.dvz_view_fly(view, panel, ctypes.byref(desc))
        if not fly:
            raise RuntimeError("dvz_view_fly() failed")

    ex.run_with_view(scene, figure, "Fly Controller", configure)


if __name__ == "__main__":
    main()
