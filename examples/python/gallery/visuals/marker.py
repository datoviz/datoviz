#!/usr/bin/env python3
"""Marker visual with built-in symbols, sizes, strokes, and rotations."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import _common as ex


SYMBOLS = np.array(
    [
        dvz.DVZ_SYMBOL_DISC,
        dvz.DVZ_SYMBOL_SQUARE,
        dvz.DVZ_SYMBOL_TRIANGLE,
        dvz.DVZ_SYMBOL_DIAMOND,
        dvz.DVZ_SYMBOL_CROSS,
        dvz.DVZ_SYMBOL_RING,
        dvz.DVZ_SYMBOL_TARGET,
    ],
    dtype=np.uint32,
)


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    cols = len(SYMBOLS)
    rows = 3
    positions = np.zeros((cols * rows, 3), dtype=np.float32)
    colors = np.zeros((cols * rows, 4), dtype=np.uint8)
    diameters = np.zeros(cols * rows, dtype=np.float32)
    angles = np.zeros(cols * rows, dtype=np.float32)
    symbols = np.tile(SYMBOLS, rows)

    palette = ex.color_array(ex.TEXT, ex.CYAN, ex.GREEN, ex.BLUE, ex.YELLOW, ex.RED, ex.CYAN)
    k = 0
    for row, y in enumerate([0.35, 0.0, -0.35]):
        for col, x in enumerate(np.linspace(-0.78, 0.78, cols)):
            positions[k] = (x, y, 0.0)
            colors[k] = palette[(col + row) % len(palette)]
            colors[k, 3] = 238
            diameters[k] = 42.0 + 8.0 * ((row + col) % 3)
            angles[k] = 0.18 * (row + col)
            k += 1

    marker = dvz.dvz_marker(scene, 0)
    if not marker:
        raise RuntimeError("dvz_marker() failed")
    ex.set_outline_marker_style(marker)
    dvz.dvz_visual_set_data_many(
        marker,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
            "angle": angles,
            "symbol": symbols,
        },
    )
    dvz.dvz_visual_set_alpha_mode(marker, dvz.DVZ_ALPHA_BLENDED)
    ex.add_visual(panel, marker)

    ex.run(scene, figure, "Marker")


if __name__ == "__main__":
    main()
