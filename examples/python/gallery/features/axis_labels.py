#!/usr/bin/env python3
"""Axis titles and tick-label layout around an empty plotting panel."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


def _configure_axis(axis, label: bytes) -> None:
    ticks = dvz.dvz_axis_tick_policy()
    ticks.target_count = 5
    ticks.min_pixel_spacing = 150.0
    ticks.minor_per_interval = 2
    if dvz.dvz_axis_set_tick_policy(axis, ctypes.byref(ticks)) != 0:
        raise RuntimeError("dvz_axis_set_tick_policy() failed")

    style = dvz.dvz_axis_style()
    style.tick_size_px = 14.0
    style.label_size_px = 20.0
    style.tick_gap_px = 10.0
    style.grid_color[:] = (116, 132, 148, 130)
    if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_axis_set_style() failed")

    if dvz.dvz_axis_set_grid(axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid() failed")
    if dvz.dvz_axis_set_label(axis, label) != 0:
        raise RuntimeError("dvz_axis_set_label() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, -40.0, 120.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.5, 2.5) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")

    _configure_axis(x_axis, b"sample offset (ms)")
    _configure_axis(y_axis, b"normalized response")

    ex.run(scene, figure, "Axis Labels")


if __name__ == "__main__":
    main()
