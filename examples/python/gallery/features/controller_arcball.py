#!/usr/bin/env python3
"""Arcball controller rotating a 3D cube view."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    ex.add_cube_mesh(scene, panel, size=1.1)

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        axis = (ctypes.c_float * 3)(0.0, 0.0, 1.0)
        if dvz.dvz_arcball_rotate_axis(arcball, 0.38, axis) != 0:
            raise RuntimeError("dvz_arcball_rotate_axis() failed")
        if dvz.dvz_arcball_zoom(arcball, 1.05) != 0:
            raise RuntimeError("dvz_arcball_zoom() failed")

    ex.run_with_view(scene, figure, "Arcball Controller", configure)


if __name__ == "__main__":
    main()
