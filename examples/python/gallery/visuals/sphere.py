#!/usr/bin/env python3
"""Sphere impostor visual with per-sphere radius and color."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    positions = np.array(
        [
            [-0.45, -0.10, 0.0],
            [0.05, 0.20, 0.0],
            [0.50, -0.18, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW)
    radii = np.array([0.18, 0.26, 0.20], dtype=np.float32)

    sphere = dvz.dvz_sphere(scene, 0)
    if not sphere:
        raise RuntimeError("dvz_sphere() failed")
    dvz.dvz_visual_set_data_many(
        sphere,
        {
            "position": positions,
            "color": colors,
            "radius": radii,
        },
    )
    ex.add_visual(panel, sphere)

    ex.run(scene, figure, "Sphere")


if __name__ == "__main__":
    main()
