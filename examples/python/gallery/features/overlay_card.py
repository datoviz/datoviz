#!/usr/bin/env python3
"""Screen-space overlay card over a simple signal."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


PATH_COUNT = 72


def _add_view_visual(panel, visual) -> None:
    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_VIEW
    if dvz.dvz_panel_add_visual(panel, visual, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual() failed")


def _add_signal(scene, panel) -> None:
    t = np.linspace(0.0, 1.0, PATH_COUNT, dtype=np.float32)
    x = -0.86 + 1.72 * t
    y = 0.16 * np.sin(2.0 * np.pi * (1.6 * t + 0.08)) + 0.10 * np.cos(
        2.0 * np.pi * (3.1 * t - 0.15)
    )
    positions = np.column_stack((x, y, np.zeros(PATH_COUNT, dtype=np.float32))).astype(np.float32)
    colors = np.tile(
        np.array([[ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 235]], dtype=np.uint8), (PATH_COUNT, 1)
    )
    widths = np.full(PATH_COUNT, 3.0, dtype=np.float32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
    if dvz.dvz_visual_set_data_many(
        path,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(path) failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    _add_view_visual(panel, path)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": np.array([[0.22, 0.13, 0.0]], dtype=np.float32),
            "color": ex.color_array(ex.YELLOW),
            "diameter_px": np.array([42.0], dtype=np.float32),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.set_filled_point_style(point)
    if dvz.dvz_visual_set_depth_test(point, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(point) failed")
    _add_view_visual(panel, point)


def _add_overlay_card(panel) -> None:
    overlay = dvz.dvz_overlay(panel, 0)
    if not overlay:
        raise RuntimeError("dvz_overlay() failed")

    style = dvz.dvz_overlay_card_style()
    background = dvz.DvzColor(ex.BG.r, ex.BG.g, ex.BG.b, 238)
    style.background_color = background
    style.text_color = ex.TEXT
    style.padding_px[:] = (16.0, 10.0)
    style.min_width_px = 300.0
    style.height_px = 46.0
    style.glyph_advance_px = 8.8
    style.text_size_px = 18.0
    style.text_renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.max_text_chars = 96

    desc = dvz.dvz_overlay_card_desc()
    desc.text = b"selected sample 42   value 0.43   status stable"
    desc.placement = dvz.DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT
    desc.offset_px[:] = (24.0, 24.0)
    card = dvz.dvz_overlay_card(overlay, ctypes.byref(desc))
    if not card:
        raise RuntimeError("dvz_overlay_card() failed")
    if dvz.dvz_overlay_card_set_style(card, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_overlay_card_set_style() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _add_signal(scene, panel)
    _add_overlay_card(panel)
    ex.run(scene, figure, "Overlay Card")


if __name__ == "__main__":
    main()
