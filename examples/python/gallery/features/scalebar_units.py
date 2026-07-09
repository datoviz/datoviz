#!/usr/bin/env python3
"""Retained duration scale bar attached to a time-series panel."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SAMPLE_COUNT = 96
TAU = 2.0 * np.pi


def _add_signal(scene, panel) -> None:
    t = np.linspace(0.0, 1.0, SAMPLE_COUNT, dtype=np.float32)
    positions = np.zeros((SAMPLE_COUNT, 3), dtype=np.float32)
    positions[:, 0] = 250.0 * t
    positions[:, 1] = 0.35 * np.sin(TAU * 2.0 * t) + 0.16 * np.cos(TAU * 5.0 * t + 0.4)

    colors = ex.color_array(*([ex.CYAN] * SAMPLE_COUNT))
    colors[:, 3] = 230
    widths = np.full(SAMPLE_COUNT, 3.0, dtype=np.float32)

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
    if dvz.dvz_visual_set_depth_test(path, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(path) failed")
    ex.add_visual(panel, path)


def _add_scalebar(scene, panel) -> None:
    units = dvz.dvz_units_builtin(scene, dvz.DVZ_UNIT_LADDER_DURATION, 0.001)
    if not units:
        raise RuntimeError("dvz_units_builtin() failed")

    desc = dvz.dvz_scale_bar_desc()
    desc.dimension = dvz.DVZ_DIM_X
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_BOTTOM_LEFT
    desc.label_position = dvz.DVZ_SCALEBAR_LABEL_ABOVE
    desc.target_length_px = 240.0
    desc.min_length_px = 170.0
    desc.max_length_px = 310.0
    desc.offset_px[:] = (72.0, 82.0)
    desc.tick_length_px = 18.0
    desc.line_width_px = 4.0
    desc.line_color[:] = (ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 255)

    scalebar = dvz.dvz_scale_bar(panel, ctypes.byref(desc))
    if not scalebar:
        raise RuntimeError("dvz_scale_bar() failed")

    style = dvz.dvz_text_style()
    style.size_px = 17.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 255)
    if dvz.dvz_scale_bar_set_label_style(scalebar, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_scale_bar_set_label_style() failed")
    if dvz.dvz_scale_bar_set_units(scalebar, units) != 0:
        raise RuntimeError("dvz_scale_bar_set_units() failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 250.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(x) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(y) failed")
    _add_signal(scene, panel)
    _add_scalebar(scene, panel)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    desc = dvz.dvz_panzoom_desc()
    desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_FIXED_Y
    controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Scale Bar Units", configure)


if __name__ == "__main__":
    main()
