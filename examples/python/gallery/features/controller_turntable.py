#!/usr/bin/env python3
"""World-up turntable controller orbiting a 3D cube."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    camera = ex.manual_camera(panel)
    ex.add_cube_mesh(scene, panel, size=1.1)

    def configure(view) -> None:
        desc = dvz.dvz_turntable_desc()
        desc.controller_flags = dvz.DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE
        desc.initial_view = camera.view
        desc.min_pitch = -0.72
        desc.max_pitch = +0.72
        desc.min_distance = 2.40
        desc.max_distance = 6.20
        turntable = dvz.dvz_view_turntable(view, panel, ctypes.byref(desc))
        if not turntable:
            raise RuntimeError("dvz_view_turntable() failed")

    ex.run_with_view(scene, figure, "Turntable Controller", configure)


if __name__ == "__main__":
    main()
