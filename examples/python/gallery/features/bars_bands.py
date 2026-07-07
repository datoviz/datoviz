#!/usr/bin/env python3
"""Bars with explicit intervals and a continuous uncertainty band."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


BAR_COUNT = 9
BAND_COUNT = 96


def _configure_panel(panel) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -0.5, 8.5) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -0.35, 2.25) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")
    if dvz.dvz_axis_set_grid(x_axis, False) != 0:
        raise RuntimeError("dvz_axis_set_grid(X) failed")
    if dvz.dvz_axis_set_grid(y_axis, False) != 0:
        raise RuntimeError("dvz_axis_set_grid(Y) failed")
    if dvz.dvz_axis_set_label(x_axis, b"sample") != 0:
        raise RuntimeError("dvz_axis_set_label(X) failed")
    if dvz.dvz_axis_set_label(y_axis, b"value") != 0:
        raise RuntimeError("dvz_axis_set_label(Y) failed")


def _add_bars(panel) -> None:
    samples = np.arange(BAR_COUNT, dtype=np.float64)
    starts = samples - 0.42
    ends = samples + 0.42
    values = 0.42 + 0.12 * samples + 0.32 * np.sin(0.70 * samples)

    desc = dvz.dvz_bars_desc()
    desc.fill_color = dvz.DvzColor(76, 201, 240, 150)
    desc.outline_color = dvz.DvzColor(76, 201, 240, 95)
    desc.outline_width_px = 1.0
    desc.gap_fraction = 0.12

    bars = dvz.dvz_bars(panel, ctypes.byref(desc))
    if not bars:
        raise RuntimeError("dvz_bars() failed")
    if dvz.dvz_bars_set_intervals(bars, starts, ends, values) != 0:
        raise RuntimeError("dvz_bars_set_intervals() failed")


def _add_band(panel) -> None:
    t = np.linspace(0.0, 1.0, BAND_COUNT, dtype=np.float64)
    x = 8.0 * t
    center = 0.74 + 0.48 * t + 0.22 * np.sin(2.0 * np.pi * (1.35 * t + 0.08))
    half_width = 0.18 + 0.07 * np.cos(2.0 * np.pi * t)
    lower = center - half_width
    upper = center + half_width

    desc = dvz.dvz_band_desc()
    desc.fill_color = dvz.DvzColor(128, 255, 219, 58)
    desc.line_color = dvz.DvzColor(128, 255, 219, 255)
    desc.line_width_px = 5.0
    desc.show_bounds = True
    desc.bound_color = dvz.DvzColor(128, 255, 219, 150)
    desc.bound_width_px = 1.5

    band = dvz.dvz_band(panel, ctypes.byref(desc))
    if not band:
        raise RuntimeError("dvz_band() failed")
    if dvz.dvz_band_set_bounds(band, x, lower, upper) != 0:
        raise RuntimeError("dvz_band_set_bounds() failed")
    if dvz.dvz_band_set_center(band, x, center) != 0:
        raise RuntimeError("dvz_band_set_center() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _configure_panel(panel)
    _add_bars(panel)
    _add_band(panel)
    ex.run(scene, figure, "Bars And Bands")


if __name__ == "__main__":
    main()
