#!/usr/bin/env python3
"""UTC datetime tick labels on a numeric x axis."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SAMPLE_COUNT = 320
TAU = 2.0 * np.pi
MAY_1_UTC_US = 1_714_554_000_000_000


def _signal():
    t = np.linspace(0.0, 1.0, SAMPLE_COUNT, dtype=np.float32)
    x = 8.0 * t
    y = 0.46 * np.sin(3.0 * TAU * t) + 0.18 * np.cos(9.0 * TAU * t + 0.3)
    positions = np.column_stack([x, y, np.zeros(SAMPLE_COUNT)]).astype(np.float32)
    colors = ex.color_array(*([ex.CYAN] * SAMPLE_COUNT))
    colors[:, 3] = 235
    widths = np.full(SAMPLE_COUNT, 4.0, dtype=np.float32)
    return positions, colors, widths


def _add_signal(scene, panel) -> None:
    positions, colors, widths = _signal()
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
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    ex.add_visual(panel, path)


def _configure_axis(axis, label: bytes, vertical: bool) -> None:
    ticks = dvz.dvz_axis_tick_policy()
    ticks.target_count = 7
    ticks.min_pixel_spacing = 130.0
    ticks.minor_per_interval = 3
    if dvz.dvz_axis_set_tick_policy(axis, ctypes.byref(ticks)) != 0:
        raise RuntimeError("dvz_axis_set_tick_policy() failed")

    style = dvz.dvz_axis_style()
    style.tick_size_px = 14.0
    style.label_size_px = 18.0
    style.tick_gap_px = 9.0
    style.label_gap_px = 12.0
    style.grid_color[:] = (96, 165, 250, 72)
    style.spine_color[:] = (217, 226, 236, 190)
    style.major_tick_color[:] = (217, 226, 236, 190)
    style.minor_tick_color[:] = (217, 226, 236, 110)
    if vertical:
        style.reserve_px = 80.0
    if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_axis_set_style() failed")
    if dvz.dvz_axis_set_grid(axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid() failed")
    if dvz.dvz_axis_set_label(axis, label) != 0:
        raise RuntimeError("dvz_axis_set_label() failed")


def _datetime_format(scene):
    datetime = dvz.dvz_datetime_format_create(scene)
    if not datetime:
        raise RuntimeError("dvz_datetime_format_create() failed")
    if dvz.dvz_datetime_format_timezone(datetime, b"UTC") != 0:
        raise RuntimeError("dvz_datetime_format_timezone() failed")

    rules = [
        (dvz.DVZ_TIME_INTERVAL_MICROSECOND, b"%H:%M:%S.fff"),
        (dvz.DVZ_TIME_INTERVAL_MILLISECOND, b"%H:%M:%S.fff"),
        (dvz.DVZ_TIME_INTERVAL_SECOND, b"%H:%M:%S"),
        (dvz.DVZ_TIME_INTERVAL_MINUTE, b"%H:%M"),
        (dvz.DVZ_TIME_INTERVAL_HOUR, b"%H:%M"),
        (dvz.DVZ_TIME_INTERVAL_DAY, b"%b %d"),
        (dvz.DVZ_TIME_INTERVAL_MONTH, b"%Y-%m"),
        (dvz.DVZ_TIME_INTERVAL_YEAR, b"%Y"),
    ]
    for interval, fmt in rules:
        if dvz.dvz_datetime_format_rule(datetime, interval, fmt) != 0:
            raise RuntimeError("dvz_datetime_format_rule() failed")
    return datetime


def _configure_axes(scene, panel) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")

    _configure_axis(x_axis, b"UTC time", False)
    _configure_axis(y_axis, b"signal", True)

    datetime = _datetime_format(scene)
    if dvz.dvz_axis_set_datetime(x_axis, datetime) != 0:
        raise RuntimeError("dvz_axis_set_datetime() failed")
    end_utc = MAY_1_UTC_US + 8 * 3600 * 1_000_000
    if dvz.dvz_axis_set_datetime_range(x_axis, 0.0, 8.0, MAY_1_UTC_US, end_utc) != 0:
        raise RuntimeError("dvz_axis_set_datetime_range() failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 8.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -0.9, 0.9) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")
    _add_signal(scene, panel)
    _configure_axes(scene, panel)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    controller = dvz.dvz_panzoom(scene, None)
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_X) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Datetime Axis", configure)


if __name__ == "__main__":
    main()
