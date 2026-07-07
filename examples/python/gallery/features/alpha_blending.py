#!/usr/bin/env python3
"""Translucent triangles composited in draw order."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    positions = np.array(
        [
            [-0.76, -0.54, 0.00],
            [-0.06, +0.76, 0.00],
            [+0.34, -0.42, 0.00],
            [-0.34, +0.48, 0.01],
            [+0.82, +0.36, 0.01],
            [+0.08, -0.82, 0.01],
            [-0.86, +0.10, 0.02],
            [+0.24, +0.90, 0.02],
            [+0.78, -0.10, 0.02],
        ],
        dtype=np.float32,
    )
    normals = np.zeros_like(positions)
    normals[:, 2] = 1.0
    colors = np.array(
        [
            [34, 211, 238, 150],
            [34, 211, 238, 150],
            [34, 211, 238, 150],
            [74, 222, 128, 132],
            [74, 222, 128, 132],
            [74, 222, 128, 132],
            [250, 204, 21, 118],
            [250, 204, 21, 118],
            [250, 204, 21, 118],
        ],
        dtype=np.uint8,
    )

    visual = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)
    if not visual:
        raise RuntimeError("dvz_primitive() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "normal": normals,
            "color": colors,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many() failed")
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")
    ex.add_visual(panel, visual)

    ex.run(scene, figure, "Alpha Blending")


if __name__ == "__main__":
    main()
