#!/usr/bin/env python3
"""Explicit perspective camera setup for a 3D cube."""

from __future__ import annotations

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    ex.add_cube_mesh(scene, panel)

    def configure(view) -> None:
        if not dvz.dvz_view_arcball(view, panel, None):
            raise RuntimeError("dvz_view_arcball() failed")

    ex.run_with_view(scene, figure, "Manual Camera", configure)


if __name__ == "__main__":
    main()
