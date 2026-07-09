#!/usr/bin/env python3
"""Scientific plotting workflow with panels, guides, bars, bands, and traces."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


CORR_BINS = 101
MEAN_COUNT = 320
TRACE_COUNT = 32
TRACE_SAMPLES = 320
TRACE_VERTEX_COUNT = TRACE_COUNT * TRACE_SAMPLES
TAU = 2.0 * np.pi


def _lerp_color(a, b, t: float, alpha: int):
    return (
        int((1.0 - t) * a.r + t * b.r),
        int((1.0 - t) * a.g + t * b.g),
        int((1.0 - t) * a.b + t * b.b),
        alpha,
    )


def _trace_color(channel: int):
    u = channel / float(TRACE_COUNT - 1) if TRACE_COUNT > 1 else 0.0
    cyan = dvz.DvzColor(76, 201, 240, 255)
    mint = dvz.DvzColor(128, 255, 219, 255)
    amber = dvz.DvzColor(255, 183, 3, 255)
    rose = dvz.DvzColor(239, 71, 111, 255)
    if u < 0.38:
        return _lerp_color(cyan, mint, u / 0.38, 235)
    if u < 0.74:
        return _lerp_color(mint, amber, (u - 0.38) / 0.36, 235)
    return _lerp_color(amber, rose, (u - 0.74) / 0.26, 235)


def _autocorr_value(lag_ms):
    baseline = 38.0
    peak = 88.0 * np.exp(-(lag_ms * lag_ms) / (2.0 * 7.4 * 7.4))
    theta = 10.0 * np.cos(0.31 * lag_ms) * np.exp(-np.abs(lag_ms) / 34.0)
    refractory = 92.0 * np.exp(-(lag_ms * lag_ms) / (2.0 * 1.8 * 1.8))
    return np.maximum(baseline + peak + theta - refractory, 1.0)


def _autocorrelogram_data():
    starts = np.linspace(-50.0, 50.0, CORR_BINS, endpoint=False, dtype=np.float64)
    ends = starts + 100.0 / float(CORR_BINS)
    values = _autocorr_value(0.5 * (starts + ends)).astype(np.float64)
    return starts, ends, values


def _mean_error_data():
    t = np.linspace(0.0, 1.0, MEAN_COUNT, dtype=np.float64)
    y = 1.1 + 0.42 * np.sin(TAU * (0.82 * t + 0.08))
    y += 0.18 * np.sin(TAU * (2.4 * t + 0.42))
    e = 0.14 + 0.06 * np.sin(TAU * (1.7 * t + 0.20))
    x = 10.0 * t
    return x, y - e, y + e, y


def _stacked_trace_data():
    positions = np.zeros((TRACE_VERTEX_COUNT, 3), dtype=np.float32)
    colors = np.zeros((TRACE_VERTEX_COUNT, 4), dtype=np.uint8)
    widths = np.zeros(TRACE_VERTEX_COUNT, dtype=np.float32)
    subpaths = np.full(TRACE_COUNT, TRACE_SAMPLES, dtype=np.uint32)
    t = np.linspace(0.0, 1.0, TRACE_SAMPLES, dtype=np.float32)

    for ch in range(TRACE_COUNT):
        row = float(TRACE_COUNT - 1 - ch)
        phase = ch / float(TRACE_COUNT)
        y = row + 0.19 * np.sin(TAU * ((2.1 + 0.035 * ch) * t + phase))
        y += 0.055 * np.sin(TAU * (27.0 * t + 0.11 * ch))
        y += 0.035 * np.cos(TAU * (53.0 * t + 0.07 * ch))
        k0 = ch * TRACE_SAMPLES
        k1 = k0 + TRACE_SAMPLES
        positions[k0:k1, 0] = 10.0 * t
        positions[k0:k1, 1] = y
        colors[k0:k1] = _trace_color(ch)
        widths[k0:k1] = 1.9 if ch in (0, TRACE_COUNT - 1) else 1.55

    return positions, colors, widths, subpaths


def _set_domain(panel, x0: float, x1: float, y0: float, y1: float) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, x0, x1) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, y0, y1) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _configure_panel(panel) -> None:
    dvz.dvz_panel_set_background_color(panel, ex.BG)
    border = dvz.dvz_panel_border_desc()
    border.visible = True
    border.color = dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 220)
    border.width_px = 2.0
    border.inset_px = 1.0
    if dvz.dvz_panel_set_border(panel, ctypes.byref(border)) != 0:
        raise RuntimeError("dvz_panel_set_border() failed")


def _add_axes(
    panel,
    x_label: bytes | None,
    y_label: bytes | None,
    *,
    show_y_axis: bool = True,
) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")

    ticks = dvz.dvz_axis_tick_policy()
    ticks.target_count = 6
    ticks.min_pixel_spacing = 92.0
    ticks.minor_per_interval = 3
    for axis in (x_axis, y_axis):
        if dvz.dvz_axis_set_tick_policy(axis, ctypes.byref(ticks)) != 0:
            raise RuntimeError("dvz_axis_set_tick_policy() failed")

    for axis, vertical in ((x_axis, False), (y_axis, True)):
        style = dvz.dvz_axis_style()
        style.tick_size_px = 10.0
        style.label_size_px = 13.0
        style.tick_gap_px = 6.0
        style.label_gap_px = 18.0 if vertical else 0.0
        style.grid_color[:] = (96, 165, 250, 105)
        style.spine_color[:] = (217, 226, 236, 210)
        style.major_tick_color[:] = (217, 226, 236, 230)
        style.minor_tick_color[:] = (217, 226, 236, 190)
        if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
            raise RuntimeError("dvz_axis_set_style() failed")

    if dvz.dvz_axis_set_grid(x_axis, True) != 0:
        raise RuntimeError("dvz_axis_set_grid(X) failed")
    if dvz.dvz_axis_set_grid(y_axis, show_y_axis) != 0:
        raise RuntimeError("dvz_axis_set_grid(Y) failed")
    if dvz.dvz_axis_set_visible(y_axis, show_y_axis) != 0:
        raise RuntimeError("dvz_axis_set_visible(Y) failed")
    if x_label is not None and dvz.dvz_axis_set_label(x_axis, x_label) != 0:
        raise RuntimeError("dvz_axis_set_label(X) failed")
    if show_y_axis and y_label is not None and dvz.dvz_axis_set_label(y_axis, y_label) != 0:
        raise RuntimeError("dvz_axis_set_label(Y) failed")


def _add_label(panel, text: bytes, position, offset, color) -> None:
    style = dvz.dvz_text_style()
    style.size_px = 14.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = color

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_DATA
    placement.position[:] = position
    placement.offset[:] = offset
    placement.text_anchor[:] = (0.5, 0.5)
    placement.has_text_anchor = True
    placement.depth_test = False

    desc = dvz.dvz_label_desc()
    desc.text = text
    annotation = dvz.dvz_annotation_label(panel, ctypes.byref(desc))
    if not annotation:
        raise RuntimeError("dvz_annotation_label() failed")
    if dvz.dvz_annotation_set_style(annotation, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_annotation_set_style() failed")
    if dvz.dvz_annotation_set_placement(annotation, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_annotation_set_placement() failed")


def _add_autocorrelogram(panel) -> None:
    starts, ends, values = _autocorrelogram_data()

    bars_desc = dvz.dvz_bars_desc()
    bars_desc.fill_color = dvz.DvzColor(76, 201, 240, 185)
    bars_desc.outline_color = dvz.DvzColor(76, 201, 240, 75)
    bars_desc.outline_width_px = 0.8
    bars_desc.gap_fraction = 0.08
    bars = dvz.dvz_bars(panel, ctypes.byref(bars_desc))
    if not bars:
        raise RuntimeError("dvz_bars() failed")
    if dvz.dvz_bars_set_intervals(bars, starts, ends, values) != 0:
        raise RuntimeError("dvz_bars_set_intervals() failed")

    span_desc = dvz.dvz_guide_span_desc()
    span_desc.orientation = dvz.DVZ_GUIDE_ORIENTATION_VERTICAL
    span_desc.min_value = -2.0
    span_desc.max_value = 2.0
    span_desc.fill_color = dvz.DvzColor(239, 71, 111, 42)
    span_desc.outline_color = dvz.DvzColor(239, 71, 111, 165)
    span_desc.outline_width_px = 1.5
    if not dvz.dvz_guide_span(panel, ctypes.byref(span_desc)):
        raise RuntimeError("dvz_guide_span() failed")

    baseline = dvz.dvz_guide_line_desc()
    baseline.orientation = dvz.DVZ_GUIDE_ORIENTATION_HORIZONTAL
    baseline.value = 38.0
    baseline.color = dvz.DvzColor(128, 255, 219, 220)
    baseline.stroke_width_px = 2.25
    if not dvz.dvz_guide_line(panel, ctypes.byref(baseline)):
        raise RuntimeError("dvz_guide_line(baseline) failed")

    zero = dvz.dvz_guide_line_desc()
    zero.orientation = dvz.DVZ_GUIDE_ORIENTATION_VERTICAL
    zero.value = 0.0
    zero.color = dvz.DvzColor(255, 183, 3, 220)
    zero.stroke_width_px = 2.0
    if not dvz.dvz_guide_line(panel, ctypes.byref(zero)):
        raise RuntimeError("dvz_guide_line(zero) failed")

    _add_label(
        panel,
        b"bi-side refractory",
        (0.0, 112.0, 0.0),
        (0.0, 0.0),
        (217, 226, 236, 235),
    )
    _add_label(panel, b"baseline", (-30.0, 38.0, 0.0), (0.0, -14.0), (128, 255, 219, 235))


def _add_mean_error(panel) -> None:
    x, lower, upper, center = _mean_error_data()

    desc = dvz.dvz_band_desc()
    desc.fill_color = dvz.DvzColor(128, 255, 219, 58)
    desc.line_color = dvz.DvzColor(76, 201, 240, 255)
    desc.line_width_px = 5.5
    band = dvz.dvz_band(panel, ctypes.byref(desc))
    if not band:
        raise RuntimeError("dvz_band() failed")
    if dvz.dvz_band_set_bounds(band, x, lower, upper) != 0:
        raise RuntimeError("dvz_band_set_bounds() failed")
    if dvz.dvz_band_set_center(band, x, center) != 0:
        raise RuntimeError("dvz_band_set_center() failed")


def _add_stacked_traces(scene, panel) -> None:
    cursor = dvz.dvz_guide_line_desc()
    cursor.orientation = dvz.DVZ_GUIDE_ORIENTATION_VERTICAL
    cursor.value = 5.0
    cursor.color = dvz.DvzColor(76, 201, 240, 200)
    cursor.stroke_width_px = 1.75
    cursor.label = b"probe"
    if not dvz.dvz_guide_line(panel, ctypes.byref(cursor)):
        raise RuntimeError("dvz_guide_line(cursor) failed")

    positions, colors, widths, subpaths = _stacked_trace_data()
    traces = dvz.dvz_path(scene, 0)
    if not traces:
        raise RuntimeError("dvz_path() failed")
    if dvz.dvz_visual_set_data_many(
        traces,
        {
            "position": positions,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(traces) failed")
    lengths = np.ctypeslib.as_ctypes(subpaths)
    if dvz.dvz_path_set_subpaths(traces, TRACE_COUNT, lengths) != 0:
        raise RuntimeError("dvz_path_set_subpaths() failed")
    if dvz.dvz_path_set_caps(traces, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(traces, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    if dvz.dvz_visual_set_depth_test(traces, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(traces) failed")
    ex.add_visual(panel, traces)


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    grid = dvz.dvz_figure_grid(figure, 2, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")

    margins = dvz.DvzPanelReserve(24.0, 24.0, 18.0, 24.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 24.0, 34.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    correlogram = dvz.dvz_grid_panel(grid, 0, 0)
    mean_error = dvz.dvz_grid_panel(grid, 0, 1)
    stacked = dvz.dvz_grid_panel_span(grid, 1, 0, 1, 2)
    if not correlogram or not mean_error or not stacked:
        raise RuntimeError("dvz_grid_panel() failed")

    for panel in (correlogram, mean_error, stacked):
        _configure_panel(panel)

    _set_domain(correlogram, -50.0, 50.0, 0.0, 125.0)
    _set_domain(mean_error, 0.0, 10.0, 0.35, 1.95)
    _set_domain(stacked, 0.0, 10.0, -0.85, 31.85)

    reserve = dvz.DvzPanelReserve(56.0, 16.0, 0.0, 0.0)
    if dvz.dvz_panel_set_reserve(stacked, ctypes.byref(reserve)) != 0:
        raise RuntimeError("dvz_panel_set_reserve() failed")

    _add_autocorrelogram(correlogram)
    _add_mean_error(mean_error)
    _add_stacked_traces(scene, stacked)

    _add_axes(correlogram, b"lag (ms)", b"coincidence count")
    _add_axes(mean_error, b"time (s)", b"response")
    _add_axes(stacked, b"time (s)", None, show_y_axis=False)
    return scene, figure, (correlogram, mean_error, stacked)


def _configure_view(view, scene, panels) -> None:
    correlogram, mean_error, stacked = panels
    ex.bind_panzoom(view, scene, correlogram, dvz.DVZ_DIM_MASK_XY)
    ex.bind_panzoom(view, scene, mean_error, dvz.DVZ_DIM_MASK_XY)
    ex.bind_panzoom(view, scene, stacked, dvz.DVZ_DIM_MASK_X)


def main() -> None:
    scene, figure, panels = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panels)

    ex.run_with_view(scene, figure, "Scientific Plotting Workflow", configure)


if __name__ == "__main__":
    main()
