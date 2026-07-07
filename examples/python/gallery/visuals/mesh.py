#!/usr/bin/env python3
"""Indexed colored cube mesh."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    positions = np.array(
        [
            [-0.55, -0.55, -0.55],
            [0.55, -0.55, -0.55],
            [0.55, 0.55, -0.55],
            [-0.55, 0.55, -0.55],
            [-0.55, -0.55, 0.55],
            [0.55, -0.55, 0.55],
            [0.55, 0.55, 0.55],
            [-0.55, 0.55, 0.55],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(
        ex.CYAN, ex.GREEN, ex.YELLOW, ex.RED, ex.BLUE, ex.CYAN, ex.GREEN, ex.WHITE
    )
    indices = np.array(
        [
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            0, 4, 5, 5, 1, 0,
            1, 5, 6, 6, 2, 1,
            2, 6, 7, 7, 3, 2,
            3, 7, 4, 4, 0, 3,
        ],
        dtype=np.uint32,
    )

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    dvz.dvz_visual_set_data_many(mesh, {"position": positions, "color": colors})
    dvz.dvz_visual_set_index_data(mesh, indices)
    ex.add_visual(panel, mesh)

    ex.run(scene, figure, "Mesh")


if __name__ == "__main__":
    main()
