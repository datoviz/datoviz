#!/usr/bin/env python3
"""Synthetic wind-speed field with vectors and streamlines."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_WIDTH = 384
FIELD_HEIGHT = 240
VECTOR_COLS = 43
VECTOR_ROWS = 27
STREAMLINE_COUNT = 76
STREAMLINE_POINT_COUNT = 128
PROBE_SEGMENTS = 36
COLORMAP_LUT_SIZE = 256
DOMAIN_X_MIN = -620.0
DOMAIN_X_MAX = +620.0
DOMAIN_Y_MIN = -390.0
DOMAIN_Y_MAX = +390.0
PROBE_X = +345.0
PROBE_Y = +14.0
TAU = 2.0 * np.pi

FRAME_BG = dvz.DvzColor(14, 17, 23, 255)
PANEL_BG = dvz.DvzColor(22, 27, 34, 255)
GRID = dvz.DvzColor(48, 54, 61, 160)
TEXT = dvz.DvzColor(201, 209, 217, 255)
CYAN = dvz.DvzColor(76, 201, 240, 255)
MINT = dvz.DvzColor(128, 255, 219, 255)
AMBER = dvz.DvzColor(255, 183, 3, 255)
ERROR = dvz.DvzColor(239, 71, 111, 255)


@dataclass
class WindParams:
    time_scale: float = 1.0
    speed_max_mps: float = 80.0
    storm_center_x_km: float = 10.0
    storm_center_y_km: float = 30.0
    storm_drift_x_km: float = 40.0
    storm_drift_y_km: float = 20.0
    storm_drift_rate_x: float = 0.25
    storm_drift_rate_y: float = 0.50
    storm_drift_phase_y: float = 1.8
    eye_radius_km: float = 125.0
    vortex_strength_mps: float = 76.0
    spiral_radius_km: float = 550.0
    inflow_strength_mps: float = -9.5
    breathing_amplitude: float = 0.060
    breathing_rate: float = 0.35
    background_u_mps: float = 16.0
    background_u_y_gradient: float = 0.005
    background_u_wave_mps: float = 0.0
    background_u_wave_rate: float = 0.40
    background_u_wave_phase: float = 0.400000
    background_v_mps: float = -8.0
    background_v_wave_mps: float = 8.5
    background_v_wave_k: float = 0.008000
    background_v_wave_rate: float = 0.10
    background_v_wave_phase: float = 0.700000
    shear_strength_mps: float = 4.5
    shear_wave_k: float = 0.005500
    shear_rate: float = 0.15
    shear_y_center_km: float = -125.000000
    shear_y_radius_km: float = 310.000000
    cross_wind_strength_mps: float = 4.5
    cross_wind_wave_k: float = 0.007000
    cross_wind_rate: float = 0.25
    cross_wind_phase: float = -0.300000
    cross_wind_x_center_km: float = -220.000000
    cross_wind_x_radius_km: float = 520.000000
    terrain_friction: float = 0.24
    scalar_terrain_mix_mps: float = 6.5
    vector_scale: float = 0.50
    vector_alpha_base: float = 30.0
    vector_alpha_range: float = 70.0
    vector_width_base_px: float = 2.0
    vector_width_range_px: float = 1.8
    streamline_seed_wobble_km: float = 60.0
    streamline_seed_wobble_rate: float = 0.50
    streamline_inner_rotation_rate: float = 0.15
    streamline_inner_radius_km: float = 106.000000
    streamline_inner_radius_jitter_km: float = 4.100000
    streamline_inner_y_scale: float = 0.780000
    streamline_alpha_outer: float = 118.0
    streamline_alpha_inner: float = 166.0
    streamline_width_outer_px: float = 1.6
    streamline_width_inner_px: float = 1.5
    streamline_step_outer_km: float = 6.0
    streamline_step_inner_km: float = 6.0
    streamline_min_speed_mps: float = 7.0


def _clamp01(value):
    return np.clip(value, 0.0, 1.0)


def _u8(value):
    return np.clip(255.0 * _clamp01(value) + 0.5, 0.0, 255.0).astype(np.uint8)


def _alpha_u8(value):
    return int(np.clip(value + 0.5, 0.0, 255.0))


def _mix(a, b, t):
    return a + (b - a) * t


def _color_tuple(color, alpha: int | None = None):
    return (
        color.r,
        color.g,
        color.b,
        color.a if alpha is None else int(np.clip(alpha, 0, 255)),
    )


def _terrain_mask(x, y):
    n0 = np.sin(0.0062 * x + 0.0028 * y)
    n1 = np.sin(0.0110 * x - 0.0075 * y + 1.8)
    ridge = 0.45 * n0 + 0.30 * n1 + 0.20 * np.sin(0.0048 * (x + 1.7 * y))
    return _clamp01(0.48 + 0.55 * ridge)


def _wind_sample(x, y, params: WindParams, time_s: float):
    center_x = params.storm_center_x_km + params.storm_drift_x_km * np.sin(
        params.storm_drift_rate_x * time_s
    )
    center_y = params.storm_center_y_km + params.storm_drift_y_km * np.cos(
        params.storm_drift_rate_y * time_s + params.storm_drift_phase_y
    )
    dx = x - center_x
    dy = y - center_y
    r = np.sqrt(dx * dx + dy * dy) + 1e-3

    eye_radius = max(params.eye_radius_km, 1e-3)
    rr = r / eye_radius
    breathing = 1.0 + params.breathing_amplitude * np.sin(params.breathing_rate * time_s)
    vortex = params.vortex_strength_mps * breathing * rr * np.exp(0.5 * (1.0 - rr * rr))
    spiral_radius = max(params.spiral_radius_km, 1e-3)
    spiral = np.exp(-(r * r) / (2.0 * spiral_radius * spiral_radius))
    inflow = params.inflow_strength_mps * spiral
    terrain = _terrain_mask(x, y)

    u = (
        params.background_u_mps
        + params.background_u_y_gradient * y
        + params.background_u_wave_mps
        * np.sin(params.background_u_wave_rate * time_s + params.background_u_wave_phase)
    )
    v = params.background_v_mps + params.background_v_wave_mps * np.sin(
        params.background_v_wave_k * x
        + params.background_v_wave_phase
        + params.background_v_wave_rate * time_s
    )

    u += -vortex * dy / r + inflow * dx / r
    v += +vortex * dx / r + inflow * dy / r

    shear = params.shear_strength_mps * np.sin(
        params.shear_wave_k * (x - 0.8 * y) + params.shear_rate * time_s
    )
    shear_dy = y - params.shear_y_center_km
    shear_radius = max(params.shear_y_radius_km, 1e-3)
    u += shear * np.exp(-(shear_dy * shear_dy) / (2.0 * shear_radius * shear_radius))

    cross_dx = x - params.cross_wind_x_center_km
    cross_radius = max(params.cross_wind_x_radius_km, 1e-3)
    v += (
        params.cross_wind_strength_mps
        * np.sin(params.cross_wind_wave_k * y + params.cross_wind_phase + params.cross_wind_rate * time_s)
        * np.exp(-(cross_dx * cross_dx) / (2.0 * cross_radius * cross_radius))
    )

    friction = 1.0 - params.terrain_friction * terrain
    u *= friction
    v *= friction

    speed = np.sqrt(u * u + v * v)
    direction = np.degrees(np.arctan2(u, v))
    direction = np.where(direction < 0.0, direction + 360.0, direction)
    return u, v, speed, direction


def _wind_colormap_values(speed_mps, params: WindParams):
    t = _clamp01(speed_mps / max(params.speed_max_mps, 1e-3))
    stops = (
        (0.00, FRAME_BG),
        (0.24, dvz.DvzColor(23, 65, 92, 255)),
        (0.52, CYAN),
        (0.72, MINT),
        (0.90, AMBER),
        (1.00, ERROR),
    )
    out = np.zeros(np.shape(t) + (4,), dtype=np.uint8)
    for (left_t, left), (right_t, right) in zip(stops[:-1], stops[1:], strict=True):
        mask = (t >= left_t) & (t <= right_t if right_t == 1.0 else t < right_t)
        local = _clamp01((t - left_t) / max(right_t - left_t, 1e-6))
        out[..., 0] = np.where(mask, _u8(_mix(left.r / 255.0, right.r / 255.0, local)), out[..., 0])
        out[..., 1] = np.where(mask, _u8(_mix(left.g / 255.0, right.g / 255.0, local)), out[..., 1])
        out[..., 2] = np.where(mask, _u8(_mix(left.b / 255.0, right.b / 255.0, local)), out[..., 2])
        out[..., 3] = np.where(mask, 255, out[..., 3])
    return out


def _flow_color(speed_mps, alpha, midpoint: float, gamma: float, params: WindParams):
    normalized = _clamp01(speed_mps / max(params.speed_max_mps, 1e-3))
    t = normalized**gamma
    local0 = _clamp01(t / midpoint)
    local1 = _clamp01((t - midpoint) / (1.0 - midpoint))
    low = np.column_stack(
        [
            _u8(_mix(CYAN.r / 255.0, MINT.r / 255.0, local0)),
            _u8(_mix(CYAN.g / 255.0, MINT.g / 255.0, local0)),
            _u8(_mix(CYAN.b / 255.0, MINT.b / 255.0, local0)),
        ]
    )
    high = np.column_stack(
        [
            _u8(_mix(MINT.r / 255.0, AMBER.r / 255.0, local1)),
            _u8(_mix(MINT.g / 255.0, AMBER.g / 255.0, local1)),
            _u8(_mix(MINT.b / 255.0, AMBER.b / 255.0, local1)),
        ]
    )
    rgb = np.where((t < midpoint)[:, None], low, high)
    return np.column_stack([rgb, np.asarray(alpha, dtype=np.uint8)]).astype(np.uint8)


def _scalar_field(params: WindParams, time_s: float):
    x = np.linspace(DOMAIN_X_MIN, DOMAIN_X_MAX, FIELD_WIDTH, dtype=np.float32)
    y = np.linspace(DOMAIN_Y_MIN, DOMAIN_Y_MAX, FIELD_HEIGHT, dtype=np.float32)
    xx, yy = np.meshgrid(x, y)
    _u, _v, speed, _direction = _wind_sample(xx, yy, params, time_s)
    terrain = _terrain_mask(xx, yy)
    values = _clamp01((speed + params.scalar_terrain_mix_mps * terrain) / params.speed_max_mps)
    return np.ascontiguousarray((values * params.speed_max_mps).astype(np.float32))


def _vector_data(params: WindParams, time_s: float):
    x = np.linspace(DOMAIN_X_MIN, DOMAIN_X_MAX, VECTOR_COLS, dtype=np.float32)
    y = np.linspace(DOMAIN_Y_MIN, DOMAIN_Y_MAX, VECTOR_ROWS, dtype=np.float32)
    xx, yy = np.meshgrid(x, y)
    u, v, speed, _direction = _wind_sample(xx.ravel(), yy.ravel(), params, time_s)
    positions = np.column_stack([xx.ravel(), yy.ravel(), np.full(VECTOR_COLS * VECTOR_ROWS, 0.03)])
    vectors = np.column_stack([params.vector_scale * u, params.vector_scale * v, np.zeros_like(u)])
    alpha = params.vector_alpha_base + params.vector_alpha_range * _clamp01(
        speed / max(params.vortex_strength_mps, 1e-3)
    )
    colors = _flow_color(speed, [_alpha_u8(a) for a in alpha], 0.44, 0.78, params)
    widths = params.vector_width_base_px + params.vector_width_range_px * _clamp01(
        speed / max(params.speed_max_mps, 1e-3)
    )
    return (
        np.ascontiguousarray(positions, dtype=np.float32),
        np.ascontiguousarray(vectors, dtype=np.float32),
        np.ascontiguousarray(colors, dtype=np.uint8),
        np.ascontiguousarray(widths, dtype=np.float32),
    )


def _streamline_data(params: WindParams, time_s: float):
    total = STREAMLINE_COUNT * STREAMLINE_POINT_COUNT
    positions = np.zeros((total, 3), dtype=np.float32)
    colors = np.zeros((total, 4), dtype=np.uint8)
    widths = np.zeros(total, dtype=np.float32)
    subpaths = np.full(STREAMLINE_COUNT, STREAMLINE_POINT_COUNT, dtype=np.uint32)

    for line in range(STREAMLINE_COUNT):
        band = line / (STREAMLINE_COUNT - 1)
        upper_band = band**0.68
        x = (
            DOMAIN_X_MIN
            + 78.0
            + 54.0 * (line % 8)
            + params.streamline_seed_wobble_km
            * np.sin(params.streamline_seed_wobble_rate * time_s + 3.1 * band)
        )
        y = _mix(DOMAIN_Y_MIN + 132.0, DOMAIN_Y_MAX - 42.0, upper_band)
        if line >= STREAMLINE_COUNT // 2:
            inner = line - STREAMLINE_COUNT // 2
            a = TAU * inner / (STREAMLINE_COUNT // 2) + params.streamline_inner_rotation_rate * time_s
            radius = params.streamline_inner_radius_km + params.streamline_inner_radius_jitter_km * (
                inner % 7
            )
            x = params.storm_center_x_km + radius * np.cos(a)
            y = params.storm_center_y_km + params.streamline_inner_y_scale * radius * np.sin(a)

        active = True
        held_position = np.zeros(3, dtype=np.float32)
        held_color = np.array(_color_tuple(CYAN, 0), dtype=np.uint8)
        for point in range(STREAMLINE_POINT_COUNT):
            idx = line * STREAMLINE_POINT_COUNT + point
            if not active:
                positions[idx] = held_position
                colors[idx] = held_color
                continue

            positions[idx] = (x, y, 0.02)
            held_position = positions[idx].copy()
            u, v, speed, _direction = _wind_sample(x, y, params, time_s)
            alpha = (
                params.streamline_alpha_outer
                if line < STREAMLINE_COUNT // 2
                else params.streamline_alpha_inner
            )
            colors[idx] = _flow_color(np.array([speed]), [_alpha_u8(alpha)], 0.36, 0.64, params)[0]
            held_color = colors[idx].copy()
            held_color[3] = 0
            widths[idx] = (
                params.streamline_width_outer_px
                if line < STREAMLINE_COUNT // 2
                else params.streamline_width_inner_px
            )

            norm = max(float(speed), params.streamline_min_speed_mps)
            step = (
                params.streamline_step_outer_km
                if line < STREAMLINE_COUNT // 2
                else params.streamline_step_inner_km
            )
            x += step * float(u) / norm
            y += step * float(v) / norm
            if x < DOMAIN_X_MIN or x > DOMAIN_X_MAX or y < DOMAIN_Y_MIN or y > DOMAIN_Y_MAX:
                active = False
    return positions, colors, widths, subpaths


def _configure_panel(panel) -> None:
    padding = dvz.DvzPanelReserve()
    padding.left_px = 66.0
    padding.right_px = 12.0
    padding.bottom_px = 12.0
    padding.top_px = 12.0
    if dvz.dvz_panel_set_padding(panel, ctypes.byref(padding)) != 0:
        raise RuntimeError("dvz_panel_set_padding() failed")

    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.02
    desc.domain_x[:] = (DOMAIN_X_MIN, DOMAIN_X_MAX)
    desc.domain_y[:] = (DOMAIN_Y_MIN, DOMAIN_Y_MAX)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def _attach(panel, visual, z_layer: int) -> None:
    attach = dvz.dvz_visual_attach_desc()
    attach.z_layer = z_layer
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    if dvz.dvz_panel_add_visual(panel, visual, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual() failed")


def _add_scale(scene, params: WindParams):
    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    desc.label = b"wind speed"
    desc.unit = b"m/s"
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    fmt = dvz.dvz_format_desc()
    fmt.precision = 0
    fmt.trim_trailing_zeros = True
    if dvz.dvz_scale_set_format(scale, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_scale_set_format() failed")
    if dvz.dvz_scale_set_domain(scale, 0.0, params.speed_max_mps) != 0:
        raise RuntimeError("dvz_scale_set_domain() failed")
    if dvz.dvz_scale_set_view_range(scale, 0.0, params.speed_max_mps) != 0:
        raise RuntimeError("dvz_scale_set_view_range() failed")

    speeds = np.linspace(0.0, params.speed_max_mps, COLORMAP_LUT_SIZE, dtype=np.float32)
    rgba = _wind_colormap_values(speeds, params)
    colors = (dvz.DvzColor * COLORMAP_LUT_SIZE)(
        *(dvz.DvzColor(int(r), int(g), int(b), int(a)) for r, g, b, a in rgba)
    )
    colormap = dvz.dvz_colormap_custom(scene, b"showcase_wind_speed", colors, COLORMAP_LUT_SIZE)
    if not colormap:
        raise RuntimeError("dvz_colormap_custom() failed")
    if dvz.dvz_scale_set_colormap(scale, colormap) != 0:
        raise RuntimeError("dvz_scale_set_colormap() failed")
    return scale


def _add_colorbar(panel, scale) -> None:
    desc = dvz.dvz_colorbar_desc()
    desc.orientation = dvz.DVZ_COLORBAR_ORIENTATION_VERTICAL
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_LEFT
    desc.title = b"m/s"
    desc.reserve_px = 66.0
    desc.ramp_width_px = 24.0
    desc.plot_gap_px = 10.0
    desc.tick_length_px = 5.0
    desc.label_gap_px = 4.0
    desc.text_renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    colorbar = dvz.dvz_colorbar(panel, scale, ctypes.byref(desc))
    if not colorbar:
        raise RuntimeError("dvz_colorbar() failed")
    fmt = dvz.dvz_format_desc()
    fmt.precision = 0
    fmt.trim_trailing_zeros = True
    if dvz.dvz_colorbar_set_format(colorbar, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_colorbar_set_format() failed")


def _add_image(scene, panel, scale, params: WindParams) -> None:
    values = _scalar_field(params, 0.0)
    positions = np.array(
        [
            [DOMAIN_X_MIN, DOMAIN_Y_MIN, 0.0],
            [DOMAIN_X_MIN, DOMAIN_Y_MAX, 0.0],
            [DOMAIN_X_MAX, DOMAIN_Y_MIN, 0.0],
            [DOMAIN_X_MAX, DOMAIN_Y_MAX, 0.0],
        ],
        dtype=np.float32,
    )
    texcoords = np.array([[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]], dtype=np.float32)
    image = dvz.dvz_image(scene, 0)
    if not image:
        raise RuntimeError("dvz_image() failed")
    if dvz.dvz_visual_set_data_many(image, {"position": positions, "texcoords": texcoords}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(image) failed")
    if dvz.dvz_visual_set_scale(image, b"color", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale(image) failed")
    field = dvz.dvz_sampled_field_from_array(scene, values)
    if dvz.dvz_visual_set_field(image, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(image) failed")
    if dvz.dvz_visual_set_depth_test(image, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(image) failed")
    _attach(panel, image, 0)


def _add_vectors(scene, panel, params: WindParams) -> None:
    positions, vectors, colors, widths = _vector_data(params, 0.0)
    visual = dvz.dvz_vector(scene, 0)
    if not visual:
        raise RuntimeError("dvz_vector() failed")
    style = dvz.dvz_vector_style()
    style.end_cap = dvz.DVZ_SEGMENT_CAP_TRIANGLE_OUT
    if dvz.dvz_vector_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_vector_set_style() failed")
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "vector": vectors,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(vector) failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(vector) failed")
    _attach(panel, visual, 2)


def _add_streamlines(scene, panel, params: WindParams) -> None:
    positions, colors, widths, subpaths = _streamline_data(params, 0.0)
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
    if dvz.dvz_path_set_subpaths(
        path, STREAMLINE_COUNT, subpaths.ctypes.data_as(ctypes.POINTER(ctypes.c_uint32))
    ) != 0:
        raise RuntimeError("dvz_path_set_subpaths() failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    if dvz.dvz_visual_set_depth_test(path, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(path) failed")
    _attach(panel, path, 1)


def _add_probe(scene, panel, params: WindParams) -> None:
    angles = np.linspace(0.0, TAU, PROBE_SEGMENTS + 1, dtype=np.float32)
    starts = np.column_stack(
        [PROBE_X + 18.0 * np.cos(angles[:-1]), PROBE_Y + 14.0 * np.sin(angles[:-1])]
    )
    ends = np.column_stack(
        [PROBE_X + 18.0 * np.cos(angles[1:]), PROBE_Y + 14.0 * np.sin(angles[1:])]
    )
    starts = np.column_stack([starts, np.full(PROBE_SEGMENTS, 0.05)]).astype(np.float32)
    ends = np.column_stack([ends, np.full(PROBE_SEGMENTS, 0.05)]).astype(np.float32)
    colors = np.tile(np.array([_color_tuple(CYAN, 235)], dtype=np.uint8), (PROBE_SEGMENTS, 1))
    widths = np.full(PROBE_SEGMENTS, 2.0, dtype=np.float32)

    ring = dvz.dvz_segment(scene, 0)
    if not ring:
        raise RuntimeError("dvz_segment(probe ring) failed")
    if dvz.dvz_visual_set_data_many(
        ring,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(probe ring) failed")
    if dvz.dvz_segment_set_caps(ring, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_segment_set_caps(probe ring) failed")
    if dvz.dvz_visual_set_depth_test(ring, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(probe ring) failed")
    _attach(panel, ring, 3)

    dot = dvz.dvz_point(scene, 0)
    if not dot:
        raise RuntimeError("dvz_point(probe dot) failed")
    dot_position = np.array([[PROBE_X, PROBE_Y, 0.06]], dtype=np.float32)
    dot_color = np.array([_color_tuple(CYAN, 245)], dtype=np.uint8)
    dot_diameter = np.array([7.0], dtype=np.float32)
    if dvz.dvz_visual_set_data_many(
        dot,
        {
            "position": dot_position,
            "color": dot_color,
            "diameter_px": dot_diameter,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(probe dot) failed")
    ex.set_filled_point_style(dot)
    if dvz.dvz_visual_set_depth_test(dot, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(probe dot) failed")
    _attach(panel, dot, 4)

    _u, _v, speed, direction = _wind_sample(PROBE_X, PROBE_Y, params, 0.0)
    _add_readout(panel, f"Wind Speed  {float(speed):.1f} m/s    Dir  {float(direction):.0f} deg".encode())


def _add_readout(panel, text: bytes) -> None:
    overlay = dvz.dvz_overlay(panel, 0)
    if not overlay:
        raise RuntimeError("dvz_overlay() failed")
    style = dvz.dvz_overlay_card_style()
    style.background_color = dvz.DvzColor(PANEL_BG.r, PANEL_BG.g, PANEL_BG.b, 226)
    style.text_color = TEXT
    style.padding_px[:] = (12.0, 7.0)
    style.min_width_px = 322.0
    style.height_px = 32.0
    style.glyph_advance_px = 7.5
    style.text_size_px = 14.0
    style.text_renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.max_text_chars = 96

    desc = dvz.dvz_overlay_card_desc()
    desc.text = text
    desc.placement = dvz.DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT
    desc.offset_px[:] = (-112.0, -46.0)
    card = dvz.dvz_overlay_card(overlay, ctypes.byref(desc))
    if not card:
        raise RuntimeError("dvz_overlay_card() failed")
    if dvz.dvz_overlay_card_set_style(card, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_overlay_card_set_style() failed")


def _build_scene():
    params = WindParams()
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    _configure_panel(panel)
    scale = _add_scale(scene, params)
    _add_colorbar(panel, scale)
    _add_image(scene, panel, scale, params)
    _add_streamlines(scene, panel, params)
    _add_vectors(scene, panel, params)
    _add_probe(scene, panel, params)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    desc = dvz.dvz_panzoom_desc()
    desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_KEEP_ASPECT
    controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Wind Field", configure)


if __name__ == "__main__":
    main()
