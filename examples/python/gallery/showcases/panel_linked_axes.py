#!/usr/bin/env python3
"""Linked time-series panels with shared X navigation and retained axes."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


PATH_COUNT = 360
EVENT_COUNT = 96
EVENT_ROWS = 8
POINT_COUNT = 168
PHASE_COUNT = 360
BAND_COUNT = 2
CURSOR_COUNT = BAND_COUNT
TAU = 2.0 * np.pi


def _signal(t):
    return (
        0.85 * np.sin(TAU * (1.08 * t + 0.06))
        + 0.34 * np.sin(TAU * (3.20 * t + 0.18))
        + 0.12 * np.cos(TAU * (6.00 * t + 0.10))
    )


def _lagged_signal(t):
    return 0.76 * np.sin(TAU * (1.08 * t - 0.09)) + 0.30 * np.sin(
        TAU * (2.50 * t + 0.32)
    )


def _signal_data():
    t = np.linspace(0.0, 1.0, PATH_COUNT, dtype=np.float32)
    positions = np.column_stack((12.0 * t, _signal(t), np.zeros(PATH_COUNT))).astype(np.float32)
    colors = np.empty((PATH_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = 72
    colors[:, 1] = np.clip(188.0 + 44.0 * t, 0, 255).astype(np.uint8)
    colors[:, 2] = 242
    colors[:, 3] = 255
    widths = np.full(PATH_COUNT, 3.2, dtype=np.float32)
    return positions, colors, widths


def _event_data():
    starts = np.zeros((EVENT_COUNT, 3), dtype=np.float32)
    ends = np.zeros((EVENT_COUNT, 3), dtype=np.float32)
    colors = np.zeros((EVENT_COUNT, 4), dtype=np.uint8)
    widths = np.zeros(EVENT_COUNT, dtype=np.float32)
    groups = EVENT_COUNT // EVENT_ROWS

    for i in range(EVENT_COUNT):
        row = i % EVENT_ROWS
        group = i // EVENT_ROWS
        base = group / float(groups - 1)
        phase = row / float(EVENT_ROWS - 1)
        x = 12.0 * base + 0.18 * np.sin(TAU * (0.23 * i + 0.17 * phase))
        starts[i] = (np.clip(x, 0.0, 12.0), row - 0.34, 0.0)
        ends[i] = (starts[i, 0], row + 0.34, 0.0)
        colors[i] = (100, 170 + 9 * row, 220, 230)
        widths[i] = 2.2 + 0.8 * (row % 3)
    return starts, ends, colors, widths


def _residual_data():
    t = np.linspace(0.0, 1.0, POINT_COUNT, dtype=np.float32)
    y = 0.55 * (_signal(t) - _lagged_signal(t)) + 0.12 * np.sin(TAU * (9.0 * t + 0.20))
    positions = np.column_stack((12.0 * t, y, np.zeros(POINT_COUNT))).astype(np.float32)

    mag = np.minimum(np.abs(y), 1.0)
    colors = np.empty((POINT_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = (110.0 + 65.0 * mag).astype(np.uint8)
    colors[:, 1] = (170.0 + 52.0 * (1.0 - mag)).astype(np.uint8)
    colors[:, 2] = 216
    colors[:, 3] = 238
    diameters = (4.0 + 5.0 * mag).astype(np.float32)
    return positions, colors, diameters


def _phase_data():
    t = np.linspace(0.0, 1.0, PHASE_COUNT, dtype=np.float32)
    positions = np.column_stack((_signal(t), _lagged_signal(t), np.zeros(PHASE_COUNT))).astype(
        np.float32
    )
    colors = np.empty((PHASE_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = (64.0 + 64.0 * t).astype(np.uint8)
    colors[:, 1] = 214
    colors[:, 2] = 205
    colors[:, 3] = 245
    widths = np.full(PHASE_COUNT, 2.4, dtype=np.float32)
    return positions, colors, widths


def _band_data(ymin: float, ymax: float):
    x0 = [3.10, 8.05]
    x1 = [3.76, 8.72]
    band_colors = [(72, 170, 205, 45), (128, 220, 185, 38)]
    positions = np.zeros((6 * BAND_COUNT, 3), dtype=np.float32)
    colors = np.zeros((6 * BAND_COUNT, 4), dtype=np.uint8)
    for b in range(BAND_COUNT):
        k = 6 * b
        positions[k : k + 6] = [
            [x0[b], ymin, -0.02],
            [x1[b], ymin, -0.02],
            [x1[b], ymax, -0.02],
            [x0[b], ymin, -0.02],
            [x1[b], ymax, -0.02],
            [x0[b], ymax, -0.02],
        ]
        colors[k : k + 6] = band_colors[b]
    return positions, colors


def _cursor_data(ymin: float, ymax: float):
    xs = [3.43, 8.38]
    line_colors = [(100, 220, 245, 185), (150, 240, 205, 175)]
    starts = np.zeros((CURSOR_COUNT, 3), dtype=np.float32)
    ends = np.zeros((CURSOR_COUNT, 3), dtype=np.float32)
    colors = np.zeros((CURSOR_COUNT, 4), dtype=np.uint8)
    widths = np.full(CURSOR_COUNT, 2.2, dtype=np.float32)
    for i, x in enumerate(xs):
        starts[i] = (x, ymin, 0.0)
        ends[i] = (x, ymax, 0.0)
        colors[i] = line_colors[i]
    return starts, ends, colors, widths


def _set_domains(panel, x0: float, x1: float, y0: float, y1: float) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, x0, x1) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, y0, y1) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _configure_panel(panel) -> None:
    dvz.dvz_panel_set_background_color(panel, ex.BG)
    border = dvz.dvz_panel_border_desc()
    border.visible = True
    border.color = dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 220)
    border.width_px = 2.25
    border.inset_px = 1.125
    if dvz.dvz_panel_set_border(panel, ctypes.byref(border)) != 0:
        raise RuntimeError("dvz_panel_set_border() failed")


def _add_axes(panel, x_label: bytes | None, y_label: bytes) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")

    ticks = dvz.dvz_axis_tick_policy()
    ticks.target_count = 6
    ticks.min_pixel_spacing = 96.0
    ticks.minor_per_interval = 3
    for axis in (x_axis, y_axis):
        if dvz.dvz_axis_set_tick_policy(axis, ctypes.byref(ticks)) != 0:
            raise RuntimeError("dvz_axis_set_tick_policy() failed")

    for axis, vertical in ((x_axis, False), (y_axis, True)):
        style = dvz.dvz_axis_style()
        style.tick_size_px = 11.0
        style.label_size_px = 14.0
        style.tick_gap_px = 7.0
        style.label_gap_px = 14.0 if vertical else 0.0
        style.grid_color[:] = (96, 165, 250, 145)
        style.spine_color[:] = (217, 226, 236, 210)
        style.major_tick_color[:] = (217, 226, 236, 230)
        style.minor_tick_color[:] = (217, 226, 236, 210)
        if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
            raise RuntimeError("dvz_axis_set_style() failed")
        if dvz.dvz_axis_set_grid(axis, True) != 0:
            raise RuntimeError("dvz_axis_set_grid() failed")

    if x_label is not None and dvz.dvz_axis_set_label(x_axis, x_label) != 0:
        raise RuntimeError("dvz_axis_set_label(X) failed")
    if dvz.dvz_axis_set_label(y_axis, y_label) != 0:
        raise RuntimeError("dvz_axis_set_label(Y) failed")


def _add_bands(scene, panel, ymin: float, ymax: float) -> None:
    positions, colors = _band_data(ymin, ymax)
    visual = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)
    if not visual:
        raise RuntimeError("dvz_primitive() failed")
    if dvz.dvz_visual_set_data_many(visual, {"position": positions, "color": colors}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(bands) failed")
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(bands) failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(bands) failed")
    ex.add_visual(panel, visual)


def _add_cursor_lines(scene, panel, ymin: float, ymax: float) -> None:
    starts, ends, colors, widths = _cursor_data(ymin, ymax)
    visual = dvz.dvz_segment(scene, 0)
    if not visual:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(cursor) failed")
    if dvz.dvz_segment_set_caps(visual, dvz.DVZ_SEGMENT_CAP_SQUARE, dvz.DVZ_SEGMENT_CAP_SQUARE) != 0:
        raise RuntimeError("dvz_segment_set_caps(cursor) failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(cursor) failed")
    ex.add_visual(panel, visual)


def _add_path(scene, panel, positions, colors, widths) -> None:
    visual = dvz.dvz_path(scene, 0)
    if not visual:
        raise RuntimeError("dvz_path() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(path) failed")
    if dvz.dvz_path_set_caps(visual, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(visual, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(path) failed")
    ex.add_visual(panel, visual)


def _add_event_panel(scene, panel) -> None:
    starts, ends, colors, widths = _event_data()
    visual = dvz.dvz_segment(scene, 0)
    if not visual:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(events) failed")
    if dvz.dvz_segment_set_caps(visual, dvz.DVZ_SEGMENT_CAP_SQUARE, dvz.DVZ_SEGMENT_CAP_SQUARE) != 0:
        raise RuntimeError("dvz_segment_set_caps(events) failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(events) failed")
    ex.add_visual(panel, visual)


def _add_residual_panel(scene, panel) -> None:
    positions, colors, diameters = _residual_data()
    visual = dvz.dvz_point(scene, 0)
    if not visual:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(residuals) failed")
    style = dvz.dvz_point_style_desc()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
    if dvz.dvz_point_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_point_set_style() failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(residuals) failed")
    ex.add_visual(panel, visual)


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    grid = dvz.dvz_figure_grid(figure, 3, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")

    margins = dvz.DvzPanelReserve(36.0, 30.0, 24.0, 30.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 28.0, 26.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    signal = dvz.dvz_grid_panel(grid, 0, 0)
    events = dvz.dvz_grid_panel(grid, 1, 0)
    residuals = dvz.dvz_grid_panel(grid, 2, 0)
    summary = dvz.dvz_grid_panel_span(grid, 0, 1, 3, 1)
    if not signal or not events or not residuals or not summary:
        raise RuntimeError("dvz_grid_panel() failed")

    for panel in (signal, events, residuals, summary):
        _configure_panel(panel)

    _set_domains(signal, 0.0, 12.0, -1.6, 1.6)
    _set_domains(events, 0.0, 12.0, -0.8, 7.8)
    _set_domains(residuals, 0.0, 12.0, -1.0, 1.0)
    _set_domains(summary, -1.45, 1.45, -1.45, 1.45)

    for panel, ymin, ymax in ((signal, -1.6, 1.6), (events, -0.8, 7.8), (residuals, -1.0, 1.0)):
        _add_bands(scene, panel, ymin, ymax)
    _add_path(scene, signal, *_signal_data())
    _add_event_panel(scene, events)
    _add_residual_panel(scene, residuals)
    _add_path(scene, summary, *_phase_data())
    for panel, ymin, ymax in ((signal, -1.6, 1.6), (events, -0.8, 7.8), (residuals, -1.0, 1.0)):
        _add_cursor_lines(scene, panel, ymin, ymax)

    _add_axes(signal, None, b"signal")
    _add_axes(events, None, b"events")
    _add_axes(residuals, b"time (s)", b"residual")
    _add_axes(summary, b"signal", b"lagged")
    return scene, figure, (signal, events, residuals), summary


def _configure_view(view, scene, left_panels, summary) -> None:
    first_x = None
    for panel in left_panels:
        x_controller, x_panzoom = ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_X)
        _, y_panzoom = ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_Y)
        if not dvz.dvz_panzoom_zoom_limits(
            y_panzoom, (ctypes.c_float * 2)(1.0, 1.0), (ctypes.c_float * 2)(1.0, 1.0)
        ):
            raise RuntimeError("dvz_panzoom_zoom_limits() failed")
        if first_x is None:
            first_x = x_controller
            if dvz.dvz_panzoom_zoom(x_panzoom, (ctypes.c_float * 2)(1.36, 1.0)) != 0:
                raise RuntimeError("dvz_panzoom_zoom(left X) failed")
        else:
            link = dvz.dvz_controller_link(
                scene,
                first_x,
                x_controller,
                dvz.DVZ_CONTROLLER_LINK_EXTENT_X,
                dvz.DVZ_CONTROLLER_LINK_TWO_WAY,
            )
            if not link:
                raise RuntimeError("dvz_controller_link() failed")
    ex.bind_panzoom(view, scene, summary, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, figure, left_panels, summary = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, left_panels, summary)

    ex.run_with_view(scene, figure, "Linked Panels With Axes", configure)


if __name__ == "__main__":
    main()
