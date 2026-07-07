#!/usr/bin/env python3
"""Single full-figure panel with one point visual."""

from __future__ import annotations

import numpy as np

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    positions = np.array(
        [[-0.50, -0.20, 0.0], [0.0, 0.32, 0.0], [0.50, -0.20, 0.0]],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.GREEN, ex.YELLOW)
    diameters = np.array([44.0, 58.0, 44.0], dtype=np.float32)
    ex.add_points(scene, panel, positions, colors, diameters)
    ex.run(scene, figure, "Single Panel")


if __name__ == "__main__":
    main()
