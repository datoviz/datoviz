#!/usr/bin/env python3
"""Ground-plane reference grid in a 3D scene."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def _reference_grid_desc():
    desc = dvz.dvz_reference_grid_desc()
    desc.plane = dvz.DVZ_REFERENCE_GRID_XZ
    desc.origin[1] = -0.50
    desc.size[:] = (8.0, 8.0)
    desc.spacing = 0.25
    desc.major_every = 4
    return desc


def _add_reference_grid(panel) -> None:
    desc = _reference_grid_desc()
    grid = dvz.dvz_reference_grid(panel, ctypes.byref(desc))
    if not grid:
        raise RuntimeError("dvz_reference_grid() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    camera = ex.manual_camera(panel)
    _add_reference_grid(panel)

    def configure(view) -> None:
        desc = dvz.dvz_turntable_desc()
        desc.controller_flags = dvz.DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE
        desc.initial_view = camera.view
        desc.min_distance = 0.03
        desc.max_distance = 24.0
        turntable = dvz.dvz_view_turntable(view, panel, ctypes.byref(desc))
        if not turntable:
            raise RuntimeError("dvz_view_turntable() failed")

    ex.run_with_view(scene, figure, "Reference Grid", configure)


if __name__ == "__main__":
    main()
