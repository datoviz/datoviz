#!/usr/bin/env python3
"""Two-by-two panel grid with one point cluster per panel."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)

    centers = [(-0.32, 0.22), (0.34, 0.18), (-0.24, -0.28), (0.28, -0.22)]
    colors = [ex.CYAN, ex.GREEN, ex.YELLOW, ex.RED]
    for i, (col, row) in enumerate([(0, 0), (1, 0), (0, 1), (1, 1)]):
        panel = ex.panel_rect(figure, 0.05 + 0.48 * col, 0.06 + 0.45 * row, 0.42, 0.38)
        cx, cy = centers[i]
        positions = np.array(
            [[cx - 0.18, cy - 0.12, 0.0], [cx, cy + 0.18, 0.0], [cx + 0.18, cy - 0.12, 0.0]],
            dtype=np.float32,
        )
        ex.add_points(
            scene,
            panel,
            positions,
            ex.color_array(colors[i], ex.TEXT, colors[i]),
            np.array([32.0, 44.0, 32.0], dtype=np.float32),
        )

    ex.run(scene, figure, "Panel Grid")


if __name__ == "__main__":
    main()
