#!/usr/bin/env python3
"""Retained semantic text items in panel screen coordinates."""

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
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    if dvz.dvz_text_set_placement(text, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")

    strings = [
        b"Retained text",
        b"semantic strings, panel anchored",
        b"MSDF atlas renderer",
        b"screen placement in logical pixels",
        b"rotated label",
    ]
    positions = [
        (128.0, 180.0, 0.0),
        (132.0, 310.0, 0.0),
        (134.0, 415.0, 0.0),
        (136.0, 510.0, 0.0),
        (845.0, 585.0, 0.0),
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

    ex.run(scene, figure, "Text")


if __name__ == "__main__":
    main()
