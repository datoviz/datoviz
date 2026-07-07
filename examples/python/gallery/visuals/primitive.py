#!/usr/bin/env python3
"""Primitive triangle-list visual."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    positions = np.array(
        [
            [-0.65, -0.45, 0.0],
            [0.00, 0.58, 0.0],
            [0.65, -0.45, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW)

    primitive = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)
    if not primitive:
        raise RuntimeError("dvz_primitive() failed")
    dvz.dvz_visual_set_data_many(primitive, {"position": positions, "color": colors})
    ex.add_visual(panel, primitive)

    ex.run(scene, figure, "Primitive")


if __name__ == "__main__":
    main()
