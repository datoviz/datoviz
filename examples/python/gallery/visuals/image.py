#!/usr/bin/env python3
"""Generated scalar field displayed as a sampled-field image."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    width, height = 320, 240
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    field_values = (
        0.18
        + 0.22 * u
        + 0.16 * v
        + 0.12 * np.sin(2.0 * np.pi * (2.4 * u + 0.35 * v))
        + 0.10 * np.cos(2.0 * np.pi * (0.70 * u - 3.2 * v))
    ).astype(np.float32)
    field_values = np.clip(field_values, 0.0, 1.0)

    scale = ex.continuous_scale(scene, b"python_visual_image")
    field = dvz.dvz_sampled_field_from_array(scene, field_values)
    ex.add_image(scene, panel, field, scale=scale)

    ex.run(scene, figure, "Image")


if __name__ == "__main__":
    main()
