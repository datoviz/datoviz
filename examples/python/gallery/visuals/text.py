#!/usr/bin/env python3
"""Retained semantic text items in panel data coordinates."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    text = dvz.dvz_text(panel, 0)
    if not text:
        raise RuntimeError("dvz_text() failed")

    style = dvz.dvz_text_style()
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    if dvz.dvz_text_set_style(text, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_DATA
    if dvz.dvz_text_set_placement(text, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")

    strings = [
        b"Retained text",
        b"semantic strings, data anchored",
        b"MSDF atlas renderer",
        b"panzoom follows data coordinates",
        b"rotated label",
    ]
    positions = [
        (-0.80, +0.50, 0.0),
        (-0.79, +0.16, 0.0),
        (-0.78, -0.12, 0.0),
        (-0.77, -0.38, 0.0),
        (+0.32, -0.58, 0.0),
    ]
    sizes = [60.0, 34.0, 28.0, 22.0, 26.0]
    angles = [0.0, 0.0, 0.0, 0.0, -0.34]
    colors = [ex.TEXT, ex.CYAN, ex.GREEN, ex.BLUE, ex.YELLOW]

    items = (dvz.DvzTextItem * len(strings))()
    for item, string, position, size, angle, color in zip(
        items, strings, positions, sizes, angles, colors, strict=True
    ):
        item.string = string
        item.position[:] = position
        item.anchor[:] = (0.0, 0.5)
        item.size_px = size
        item.color = color
        item.angle = angle

    if dvz.dvz_text_set_items(text, items, len(items)) != 0:
        raise RuntimeError("dvz_text_set_items() failed")

    ex.run_with_view(
        scene,
        figure,
        "Text",
        lambda view: ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY),
    )


if __name__ == "__main__":
    main()
