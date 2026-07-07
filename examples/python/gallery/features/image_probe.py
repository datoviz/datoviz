#!/usr/bin/env python3
"""Scalar image probing with a live marker and query readback."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_WIDTH = 256
FIELD_HEIGHT = 192
PROBE_X = 0.68
PROBE_Y = 0.56
PROBE_REQUEST_ID = 1
PROBE_RING_SEGMENTS = 28
PROBE_SEGMENTS = PROBE_RING_SEGMENTS + 4
TAU = 2.0 * math.pi


def _clamp01(value: float) -> float:
    return min(max(value, 0.0), 1.0)


def _sample_field(x: float, y: float) -> float:
    value = 0.11 + 0.05 * math.sin(TAU * (2.3 * x + 0.35 * y))
    value += 0.04 * math.cos(TAU * (0.55 * x - 3.6 * y))

    filament = math.sin(TAU * (x * 1.15 + 0.22 * math.sin(TAU * y)))
    value += 0.18 * math.exp(-18.0 * (filament - 0.18) * (filament - 0.18))

    centers = (
        (0.16, 0.22, 0.050),
        (0.31, 0.71, 0.042),
        (0.46, 0.38, 0.035),
        (0.58, 0.84, 0.040),
        (0.70, 0.56, 0.038),
        (0.78, 0.24, 0.046),
        (0.86, 0.69, 0.035),
        (0.24, 0.50, 0.030),
    )
    for i, (cx, cy, sigma) in enumerate(centers):
        dx = x - cx
        dy = y - cy
        d2 = (dx * dx + 1.4 * dy * dy) / (2.0 * sigma * sigma)
        value += (0.20 + 0.06 * (i % 3)) * math.exp(-d2)

    hot_dx = x - 0.69
    value += (
        0.30
        * math.exp(-(hot_dx * hot_dx) / (2.0 * 0.115 * 0.115))
        * (0.78 + 0.22 * math.cos(TAU * (y - 0.48)))
    )

    hot_dy = y - 0.57
    value += 0.50 * math.exp(
        -(hot_dx * hot_dx + hot_dy * hot_dy) / (2.0 * 0.022 * 0.022)
    )

    mirror_dy = y - 0.43
    value += 0.42 * math.exp(
        -(hot_dx * hot_dx + mirror_dy * mirror_dy) / (2.0 * 0.024 * 0.024)
    )
    return _clamp01(value)


def _probe_field() -> np.ndarray:
    x = np.linspace(0.0, 1.0, FIELD_WIDTH, dtype=np.float32)
    y = np.linspace(0.0, 1.0, FIELD_HEIGHT, dtype=np.float32)
    u, v = np.meshgrid(x, y)

    values = 0.11 + 0.05 * np.sin(TAU * (2.3 * u + 0.35 * v))
    values += 0.04 * np.cos(TAU * (0.55 * u - 3.6 * v))

    filament = np.sin(TAU * (u * 1.15 + 0.22 * np.sin(TAU * v)))
    values += 0.18 * np.exp(-18.0 * (filament - 0.18) * (filament - 0.18))

    centers = np.array(
        [
            [0.16, 0.22, 0.050],
            [0.31, 0.71, 0.042],
            [0.46, 0.38, 0.035],
            [0.58, 0.84, 0.040],
            [0.70, 0.56, 0.038],
            [0.78, 0.24, 0.046],
            [0.86, 0.69, 0.035],
            [0.24, 0.50, 0.030],
        ],
        dtype=np.float32,
    )
    for i, (cx, cy, sigma) in enumerate(centers):
        dx = u - cx
        dy = v - cy
        d2 = (dx * dx + 1.4 * dy * dy) / (2.0 * sigma * sigma)
        values += (0.20 + 0.06 * (i % 3)) * np.exp(-d2)

    hot_dx = u - 0.69
    values += (
        0.30
        * np.exp(-(hot_dx * hot_dx) / (2.0 * 0.115 * 0.115))
        * (0.78 + 0.22 * np.cos(TAU * (v - 0.48)))
    )

    hot_dy = v - 0.57
    values += 0.50 * np.exp(
        -(hot_dx * hot_dx + hot_dy * hot_dy) / (2.0 * 0.022 * 0.022)
    )

    mirror_dy = v - 0.43
    values += 0.42 * np.exp(
        -(hot_dx * hot_dx + mirror_dy * mirror_dy) / (2.0 * 0.024 * 0.024)
    )
    return np.clip(values, 0.0, 1.0).astype(np.float32)


def _set_probe_domain(panel) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _add_probe_image(scene, panel, values: np.ndarray):
    image = dvz.dvz_image(scene, 0)
    if not image:
        raise RuntimeError("dvz_image() failed")

    positions = np.array(
        [
            [0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [1.0, 0.0, 0.0],
            [1.0, 1.0, 0.0],
        ],
        dtype=np.float32,
    )
    texcoords = np.array(
        [[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]],
        dtype=np.float32,
    )

    scale = ex.continuous_scale(scene, b"python_image_probe")
    field = dvz.dvz_sampled_field_from_array(scene, values)
    if dvz.dvz_visual_set_data_many(
        image,
        {
            "position": positions,
            "texcoords": texcoords,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(image) failed")
    if dvz.dvz_visual_set_scale(image, b"color", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale() failed")
    if dvz.dvz_visual_set_field(image, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field() failed")
    if dvz.dvz_visual_set_depth_test(image, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(image) failed")
    dvz.dvz_visual_set_query_capabilities(image, dvz.DVZ_QUERY_CAPABILITY_PIXEL)
    ex.add_visual(panel, image)
    return image


def _plot_size(panel) -> tuple[float, float]:
    rect = dvz.DvzRect()
    if (
        dvz.dvz_panel_plot_rect_px(panel, ctypes.byref(rect))
        and rect.width > 0.0
        and rect.height > 0.0
    ):
        return float(rect.width), float(rect.height)
    return float(ex.WIDTH), float(ex.HEIGHT)


def _probe_marker_arrays(panel, x: float, y: float):
    plot_width, plot_height = _plot_size(panel)
    starts = np.zeros((PROBE_SEGMENTS, 3), dtype=np.float32)
    ends = np.zeros((PROBE_SEGMENTS, 3), dtype=np.float32)
    colors = np.zeros((PROBE_SEGMENTS, 4), dtype=np.uint8)
    widths = np.zeros(PROBE_SEGMENTS, dtype=np.float32)

    gap_x = 6.0 / plot_width
    gap_y = 6.0 / plot_height
    arm_x = 20.0 / plot_width
    arm_y = 20.0 / plot_height

    starts[:4] = np.array(
        [
            [x - arm_x, y, 0.02],
            [x + gap_x, y, 0.02],
            [x, y - arm_y, 0.02],
            [x, y + gap_y, 0.02],
        ],
        dtype=np.float32,
    )
    ends[:4] = np.array(
        [
            [x - gap_x, y, 0.02],
            [x + arm_x, y, 0.02],
            [x, y - gap_y, 0.02],
            [x, y + arm_y, 0.02],
        ],
        dtype=np.float32,
    )

    cyan = np.array([ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 245], dtype=np.uint8)
    colors[:4] = cyan
    widths[:4] = 1.8

    rx = 12.0 / plot_width
    ry = 12.0 / plot_height
    for i in range(PROBE_RING_SEGMENTS):
        k = i + 4
        a0 = TAU * i / PROBE_RING_SEGMENTS
        a1 = TAU * (i + 1) / PROBE_RING_SEGMENTS
        starts[k] = (x + rx * math.cos(a0), y + ry * math.sin(a0), 0.02)
        ends[k] = (x + rx * math.cos(a1), y + ry * math.sin(a1), 0.02)
        colors[k] = (ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 225)
        widths[k] = 1.7

    return starts, ends, colors, widths


def _add_probe_marker(scene, panel):
    starts, ends, colors, widths = _probe_marker_arrays(panel, PROBE_X, PROBE_Y)
    segments = dvz.dvz_segment(scene, 0)
    if not segments:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        segments,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(segments) failed")
    if dvz.dvz_segment_set_caps(
        segments, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND
    ) != 0:
        raise RuntimeError("dvz_segment_set_caps() failed")
    if dvz.dvz_visual_set_depth_test(segments, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(segments) failed")
    ex.add_visual(panel, segments)

    dot = dvz.dvz_point(scene, 0)
    if not dot:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        dot,
        {
            "position": np.array([[PROBE_X, PROBE_Y, 0.03]], dtype=np.float32),
            "color": ex.color_array(ex.CYAN),
            "diameter_px": np.array([6.0], dtype=np.float32),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(dot) failed")
    ex.set_filled_point_style(dot)
    if dvz.dvz_visual_set_depth_test(dot, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(dot) failed")
    ex.add_visual(panel, dot)
    return segments, dot


def _update_probe_marker(panel, segments, dot, x: float, y: float) -> None:
    starts, ends, _colors, _widths = _probe_marker_arrays(panel, x, y)
    if dvz.dvz_visual_set_data_many(
        segments,
        {
            "position_start": starts,
            "position_end": ends,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(update segments) failed")
    if dvz.dvz_visual_set_data(
        dot, b"position", np.array([[x, y, 0.03]], dtype=np.float32)
    ) != 0:
        raise RuntimeError("dvz_visual_set_data(update dot) failed")


def _resolve_probe_value(panel, query) -> float | None:
    if query.status != dvz.DVZ_QUERY_STATUS_HIT or not query.hit:
        return None
    data = ex.panel_px_to_data(panel, query.panel_position[0], query.panel_position[1])
    if data is None:
        return None
    return _sample_field(data[0], data[1])


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _set_probe_domain(panel)

    values = _probe_field()
    _add_probe_image(scene, panel, values)
    probe_segments, probe_dot = _add_probe_marker(scene, panel)

    state = {
        "cursor": None,
        "last_hit": None,
        "last_value": None,
    }

    def set_probe_from_data(x: float, y: float) -> None:
        _update_probe_marker(panel, probe_segments, probe_dot, x, y)
        src = (ctypes.c_double * 2)(float(x), float(y))
        dst = (ctypes.c_double * 2)()
        ok = dvz.dvz_panel_transform_point(
            panel,
            dvz.DVZ_PANEL_COORD_DATA,
            dvz.DVZ_PANEL_COORD_PANEL_PX,
            src,
            dst,
        )
        if ok:
            state["cursor"] = (float(dst[0]), float(dst[1]))

    def on_pointer(event) -> None:
        if event.type not in (
            dvz.DVZ_POINTER_EVENT_MOVE,
            dvz.DVZ_POINTER_EVENT_PRESS,
            dvz.DVZ_POINTER_EVENT_CLICK,
        ):
            return
        panel_pos = ex.figure_to_panel_px(panel, event.pos[0], event.pos[1])
        if panel_pos is None:
            return
        data = ex.panel_px_to_data(panel, panel_pos[0], panel_pos[1])
        if data is None:
            return
        x = _clamp01(data[0])
        y = _clamp01(data[1])
        _update_probe_marker(panel, probe_segments, probe_dot, x, y)
        state["cursor"] = panel_pos

    def on_frame(_view, frame_index: int, _elapsed: float) -> None:
        if frame_index == 0 and state["cursor"] is None:
            set_probe_from_data(PROBE_X, PROBE_Y)
        cursor = state["cursor"]
        if cursor is not None:
            ex.queue_panel_query(
                panel,
                cursor[0],
                cursor[1],
                PROBE_REQUEST_ID,
                dvz.DVZ_SCENE_TARGET_PIXEL,
            )

        for query in ex.poll_queries(scene):
            if query.request_id != PROBE_REQUEST_ID:
                continue
            value = _resolve_probe_value(panel, query)
            hit = value is not None
            last_value = state["last_value"]
            changed = state["last_hit"] is None or state["last_hit"] != hit
            if hit and (last_value is None or abs(value - last_value) >= 1e-3):
                changed = True
            if changed:
                if hit:
                    print(f"probe value={value:.3f}")
                    state["last_value"] = value
                else:
                    print("probe miss")
                    state["last_value"] = None
                state["last_hit"] = hit

    ex.run_with_input_callbacks(
        scene,
        figure,
        "Image Probe",
        on_pointer=on_pointer,
        on_frame=on_frame,
    )


if __name__ == "__main__":
    main()
