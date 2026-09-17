#!/usr/bin/env python3
"""Bin deterministic samples and render their counts as explicit bars."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex

SAMPLE_COUNT = 512
BIN_COUNT = 24
VALUE_MIN = -3.0
VALUE_MAX = 3.0


def _generate_samples() -> np.ndarray:
    t = np.arange(SAMPLE_COUNT, dtype=np.float64) / SAMPLE_COUNT
    carrier = np.sin(2.0 * np.pi * (17.0 * t + 0.13))
    detail = 0.42 * np.sin(2.0 * np.pi * (43.0 * t + 0.37))
    centers = np.where(np.arange(SAMPLE_COUNT) < 310, -0.72, 1.08)
    spreads = np.where(np.arange(SAMPLE_COUNT) < 310, 0.92, 0.58)
    return centers + spreads * (carrier + detail)


def _configure_panel(panel, max_count: float) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, VALUE_MIN, VALUE_MAX) != 0:
        raise RuntimeError('dvz_panel_set_domain(X) failed')
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 1.15 * max_count) != 0:
        raise RuntimeError('dvz_panel_set_domain(Y) failed')

    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError('dvz_panel_axis() failed')
    if dvz.dvz_axis_set_grid(x_axis, False) != 0:
        raise RuntimeError('dvz_axis_set_grid(X) failed')
    if dvz.dvz_axis_set_grid(y_axis, True) != 0:
        raise RuntimeError('dvz_axis_set_grid(Y) failed')
    if dvz.dvz_axis_set_label(x_axis, b'value') != 0:
        raise RuntimeError('dvz_axis_set_label(X) failed')
    if dvz.dvz_axis_set_label(y_axis, b'count') != 0:
        raise RuntimeError('dvz_axis_set_label(Y) failed')


def main() -> None:
    samples = _generate_samples()
    counts, edges = np.histogram(samples, bins=BIN_COUNT, range=(VALUE_MIN, VALUE_MAX))
    starts = np.ascontiguousarray(edges[:-1], dtype=np.float64)
    ends = np.ascontiguousarray(edges[1:], dtype=np.float64)
    values = np.ascontiguousarray(counts, dtype=np.float64)

    scene, figure, panel = ex.scene_panel()
    _configure_panel(panel, float(values.max()))

    desc = dvz.dvz_bars_desc()
    desc.fill_color = dvz.DvzColor(76, 201, 240, 190)
    desc.outline_color = dvz.DvzColor(128, 255, 219, 210)
    desc.outline_width_px = 1.25
    desc.gap_fraction = 0.08

    bars = dvz.dvz_bars(panel, ctypes.byref(desc))
    if not bars:
        raise RuntimeError('dvz_bars() failed')
    if dvz.dvz_bars_set_intervals(bars, starts, ends, values) != 0:
        raise RuntimeError('dvz_bars_set_intervals() failed')

    ex.run(scene, figure, 'Histogram')


if __name__ == '__main__':
    main()
