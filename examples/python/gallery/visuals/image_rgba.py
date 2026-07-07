#!/usr/bin/env python3
"""Generated RGBA image displayed as a color sampled field."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    width, height = 384, 256
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    radius = np.hypot(u - 0.52, v - 0.50)
    ring = 0.5 + 0.5 * np.sin(2.0 * np.pi * (9.0 * radius - 0.18 * u))
    weave = 0.5 + 0.5 * np.cos(2.0 * np.pi * (2.4 * u + 1.7 * v))
    diagonal = np.abs((u + 0.23 * np.sin(2.0 * np.pi * v)) - (0.30 + 0.55 * v))

    rgba = np.empty((height, width, 4), dtype=np.uint8)
    rgba[..., 0] = np.clip(255.0 * (0.16 + 0.72 * u + 0.12 * ring), 0, 255).astype(np.uint8)
    rgba[..., 1] = np.clip(255.0 * (0.18 + 0.55 * (1.0 - v) + 0.22 * weave), 0, 255).astype(
        np.uint8
    )
    rgba[..., 2] = np.clip(255.0 * (0.34 + 0.40 * ring), 0, 255).astype(np.uint8)
    alpha = 0.98 - 0.46 * np.exp(-(diagonal**2) / (2.0 * 0.036**2))
    alpha -= 0.28 * np.exp(-(radius**2) / (2.0 * 0.18**2))
    rgba[..., 3] = np.clip(255.0 * alpha, 0, 255).astype(np.uint8)

    field = dvz.dvz_sampled_field_from_array(scene, rgba)
    ex.add_image(scene, panel, field, alpha=True)

    ex.run(scene, figure, "RGBA Image")


if __name__ == "__main__":
    main()
