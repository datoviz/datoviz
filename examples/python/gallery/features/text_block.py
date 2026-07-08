#!/usr/bin/env python3
"""Retained multiline text block in panel screen coordinates."""

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
    style.size_px = 24.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, ex.TEXT.a)
    if dvz.dvz_text_set_style(text, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")

    layout = dvz.dvz_text_layout()
    layout.line_height = 1.18
    layout.line_gap_px = 6.0
    if dvz.dvz_text_set_layout(text, ctypes.byref(layout)) != 0:
        raise RuntimeError("dvz_text_set_layout() failed")

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (138.0, 245.0, 0.0)
    placement.text_anchor[:] = (0.0, 0.0)
    placement.has_text_anchor = True
    if dvz.dvz_text_set_placement(text, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")

    content = (
        b"Retained text can hold a compact note that reads\n"
        b"like ordinary prose across multiple explicit lines.\n"
        b"The whole paragraph remains one scene-owned string,\n"
        b"so placement, style, and updates stay together."
    )
    if dvz.dvz_text_set_string(text, content) != 0:
        raise RuntimeError("dvz_text_set_string() failed")

    ex.run(scene, figure, "Text Block")


if __name__ == "__main__":
    main()
