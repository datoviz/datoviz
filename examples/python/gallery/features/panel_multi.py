#!/usr/bin/env python3
"""Multiple uneven panels in one figure."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)

    specs = [
        (0.06, 0.08, 0.55, 0.84, ex.CYAN),
        (0.66, 0.54, 0.28, 0.38, ex.GREEN),
        (0.66, 0.08, 0.28, 0.38, ex.YELLOW),
    ]
    base_positions = np.array(
        [[-0.48, -0.24, 0.0], [0.00, 0.30, 0.0], [0.48, -0.24, 0.0]],
        dtype=np.float32,
    )
    for x, y, w, h, color in specs:
        panel = ex.panel_rect(figure, x, y, w, h)
        ex.add_points(
            scene,
            panel,
            base_positions,
            ex.color_array(color, ex.TEXT, color),
            np.array([38.0, 54.0, 38.0], dtype=np.float32),
        )

    ex.run(scene, figure, "Multiple Panels")


if __name__ == "__main__":
    main()
