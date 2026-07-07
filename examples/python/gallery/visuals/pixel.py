#!/usr/bin/env python3
"""Pixel visual with per-item square size."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    n = 36
    x, y = np.meshgrid(np.linspace(-0.85, 0.85, n), np.linspace(-0.55, 0.55, n))
    positions = np.column_stack([x.ravel(), y.ravel(), np.zeros(n * n)]).astype(np.float32)
    t = np.hypot(x.ravel(), y.ravel())
    colors = np.column_stack(
        [
            (40 + 180 * t).clip(0, 255).astype(np.uint8),
            (220 - 100 * t).clip(0, 255).astype(np.uint8),
            np.full(n * n, 230, dtype=np.uint8),
            np.full(n * n, 230, dtype=np.uint8),
        ]
    )
    sizes = np.full(n * n, 9.0, dtype=np.float32)

    pixel = dvz.dvz_pixel(scene, 0)
    if not pixel:
        raise RuntimeError("dvz_pixel() failed")
    dvz.dvz_visual_set_data_many(
        pixel,
        {
            "position": positions,
            "color": colors,
            "pixel_size_px": sizes,
        },
    )
    dvz.dvz_visual_set_depth_test(pixel, False)
    ex.add_visual(panel, pixel)

    ex.run(scene, figure, "Pixel")


if __name__ == "__main__":
    main()
