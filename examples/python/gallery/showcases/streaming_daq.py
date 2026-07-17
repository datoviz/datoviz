#!/usr/bin/env python3
"""Live 64-channel synthetic data-acquisition sweep."""

from __future__ import annotations

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex


CHANNEL_COUNT = 64
SAMPLE_COUNT = 512
DURATION = 4.0


def _trace_geometry(elapsed: float):
    x = np.linspace(0.0, DURATION, SAMPLE_COUNT, dtype=np.float32)
    channel = np.arange(CHANNEL_COUNT, dtype=np.float32)[:, None]
    phase = 0.17 * channel + 2.0 * np.pi * (x[None, :] / DURATION + 0.18 * elapsed)
    carrier = 0.12 * np.sin(phase * (1.0 + 0.013 * channel))
    correlated = 0.06 * np.sin(3.2 * phase + 0.6 * np.sin(0.45 * elapsed))
    spike_center = np.mod(0.73 * elapsed + 0.031 * channel, DURATION)
    distance = np.minimum(np.abs(x[None, :] - spike_center), DURATION - np.abs(x[None, :] - spike_center))
    spikes = (0.34 + 0.10 * np.sin(0.21 * channel)) * np.exp(-(distance / 0.022) ** 2)
    signal = carrier + correlated + spikes
    rows = CHANNEL_COUNT - 1 - channel + 0.5
    y = rows + 0.58 * signal

    starts = np.stack((np.broadcast_to(x[:-1], (CHANNEL_COUNT, SAMPLE_COUNT - 1)), y[:, :-1]), axis=-1)
    ends = np.stack((np.broadcast_to(x[1:], (CHANNEL_COUNT, SAMPLE_COUNT - 1)), y[:, 1:]), axis=-1)
    positions = np.zeros((CHANNEL_COUNT, SAMPLE_COUNT - 1, 2, 3), dtype=np.float32)
    positions[..., 0, :2] = starts
    positions[..., 1, :2] = ends
    return positions.reshape(-1, 3)


def _trace_colors():
    palette = np.array([[94, 213, 220, 218], [88, 193, 222, 212]], dtype=np.uint8)
    per_channel = palette[(np.arange(CHANNEL_COUNT) // 32) % len(palette)]
    return np.repeat(per_channel, 2 * (SAMPLE_COUNT - 1), axis=0)


def _line_visual(scene, panel, positions, colors):
    visual = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0)
    if not visual:
        raise RuntimeError("dvz_primitive() failed")
    if dvz.dvz_visual_set_data_many(visual, {"position": positions, "color": colors}) != 0:
        raise RuntimeError("line upload failed")
    dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED)
    dvz.dvz_visual_set_depth_test(visual, False)
    ex.add_visual(panel, visual)
    return visual


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(17, 22, 29, 255))
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, DURATION)
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -0.5, CHANNEL_COUNT - 0.5)

    traces = _line_visual(scene, panel, _trace_geometry(0.0), _trace_colors())
    event_x = np.array([0.42, 1.18, 2.05, 2.83, 3.61], dtype=np.float32)
    events = np.zeros((len(event_x), 2, 3), dtype=np.float32)
    events[:, :, 0] = event_x[:, None]
    events[:, 0, 1], events[:, 1, 1] = -0.5, CHANNEL_COUNT - 0.5
    event_colors = np.repeat(np.array([[255, 179, 71, 105]], np.uint8), 2 * len(event_x), axis=0)
    _line_visual(scene, panel, events.reshape(-1, 3), event_colors)

    cursor_positions = np.array([[0.0, -0.5, 0.0], [0.0, CHANNEL_COUNT - 0.5, 0.0]], np.float32)
    cursor = _line_visual(scene, panel, cursor_positions, np.repeat(np.array([[250, 183, 3, 255]], np.uint8), 2, axis=0))

    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    dvz.dvz_axis_set_grid(x_axis, True)
    dvz.dvz_axis_set_label(x_axis, b"sweep time (s)")
    dvz.dvz_axis_set_label(y_axis, b"channel")
    return scene, figure, traces, cursor


def main() -> None:
    scene, figure, traces, cursor = _build_scene()

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        if dvz.dvz_visual_set_data(traces, "position", _trace_geometry(elapsed)) != 0:
            raise RuntimeError("trace update failed")
        x = np.float32(elapsed % DURATION)
        positions = np.array([[x, -0.5, 0.0], [x, CHANNEL_COUNT - 0.5, 0.0]], np.float32)
        if dvz.dvz_visual_set_data(cursor, "position", positions) != 0:
            raise RuntimeError("cursor update failed")

    ex.run_with_frame_callback(scene, figure, "Streaming DAQ - 64 channels", on_frame)


if __name__ == "__main__":
    main()
