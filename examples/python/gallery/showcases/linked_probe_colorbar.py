#!/usr/bin/env python3
"""Linked scalar image panels with a probe readout and shared colorbar."""

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
PROBE_REQUEST_ID = 11
TAU = 2.0 * math.pi


def _clamp01(value):
    return np.clip(value, 0.0, 1.0)


def _sample_measurement(x, y):
    value = 0.13 + 0.07 * np.sin(TAU * (2.3 * x + 0.35 * y))
    value += 0.05 * np.cos(TAU * (0.55 * x - 3.6 * y))
    filament = np.sin(TAU * (x * 1.15 + 0.22 * np.sin(TAU * y)))
    value += 0.19 * np.exp(-18.0 * (filament - 0.18) * (filament - 0.18))

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
        value += (0.20 + 0.06 * (i % 3)) * np.exp(-d2)

    hot_dx = x - 0.69
    hot_dy = y - 0.57
    value += 0.40 * np.exp(-(hot_dx * hot_dx + hot_dy * hot_dy) / (2.0 * 0.030 * 0.030))
    return _clamp01(value)


def _sample_derived(x, y):
    value = 0.18 + 0.72 * _sample_measurement(x, y)
    value -= 0.16 * np.exp(-((x - 0.69) * (x - 0.69)) / (2.0 * 0.100 * 0.100))
    value += 0.11 * np.sin(TAU * (0.85 * x + 1.40 * y))
    value += 0.07 * np.cos(TAU * (2.20 * x - 0.45 * y))
    return _clamp01(value)


def _fields():
    x = np.linspace(0.0, 1.0, FIELD_WIDTH, dtype=np.float32)
    y = np.linspace(0.0, 1.0, FIELD_HEIGHT, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    measurement = _sample_measurement(u, v).astype(np.float32)
    derived = _sample_derived(u, v).astype(np.float32)
    return measurement, derived


def _set_image_domain(panel) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _add_scale(scene):
    scale = ex.continuous_scale(scene, b"python_linked_probe_colorbar")
    fmt = dvz.dvz_format_desc()
    fmt.precision = 2
    fmt.trim_trailing_zeros = True
    if dvz.dvz_scale_set_format(scale, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_scale_set_format() failed")
    if dvz.dvz_scale_set_view_range(scale, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_scale_set_view_range() failed")
    return scale


def _add_image(scene, panel, scale, values, queryable: bool):
    positions = np.array(
        [[0.0, 0.0, 0.0], [0.0, 1.0, 0.0], [1.0, 0.0, 0.0], [1.0, 1.0, 0.0]],
        dtype=np.float32,
    )
    texcoords = np.array(
        [[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]],
        dtype=np.float32,
    )
    image = dvz.dvz_image(scene, 0)
    if not image:
        raise RuntimeError("dvz_image() failed")
    if dvz.dvz_visual_set_data_many(image, {"position": positions, "texcoords": texcoords}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(image) failed")
    if dvz.dvz_visual_set_scale(image, b"color", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale() failed")
    field = dvz.dvz_sampled_field_from_array(scene, values)
    if dvz.dvz_visual_set_field(image, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field() failed")
    if dvz.dvz_visual_set_depth_test(image, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(image) failed")
    if queryable:
        dvz.dvz_visual_set_query_capabilities(image, dvz.DVZ_QUERY_CAPABILITY_PIXEL)
    ex.add_visual(panel, image)
    return image


def _add_probe_marker(scene, panel):
    marker = dvz.dvz_marker(scene, 0)
    if not marker:
        raise RuntimeError("dvz_marker() failed")

    positions = np.array([[PROBE_X, PROBE_Y, 0.04]], dtype=np.float32)
    colors = ex.color_array(ex.YELLOW)
    colors[0, 3] = 245
    diameters = np.array([34.0], dtype=np.float32)
    angles = np.array([0.0], dtype=np.float32)
    shapes = np.array([dvz.DVZ_MARKER_SHAPE_TARGET], dtype=np.uint32)
    if dvz.dvz_visual_set_data_many(
        marker,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
            "angle": angles,
            "shape": shapes,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(marker) failed")

    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
    style.stroke_width_px = 0.0
    if dvz.dvz_marker_set_style(marker, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")
    if dvz.dvz_visual_set_depth_test(marker, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(marker) failed")
    ex.add_visual(panel, marker)
    return marker


def _update_probe_marker(marker, x: float, y: float) -> None:
    position = np.array([[x, y, 0.04]], dtype=np.float32)
    if dvz.dvz_visual_set_data(marker, b"position", position) != 0:
        raise RuntimeError("dvz_visual_set_data(marker position) failed")


def _add_text(panel, text: bytes, x: float, y: float, size: float, color):
    visual = dvz.dvz_text(panel, 0)
    if not visual:
        raise RuntimeError("dvz_text() failed")
    style = dvz.dvz_text_style()
    style.size_px = size
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (color.r, color.g, color.b, 255)
    if dvz.dvz_text_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")
    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (x, y, 0.0)
    placement.text_anchor[:] = (0.0, 0.5)
    placement.has_text_anchor = True
    placement.depth_test = False
    if dvz.dvz_text_set_placement(visual, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")
    if dvz.dvz_text_set_string(visual, text) != 0:
        raise RuntimeError("dvz_text_set_string() failed")
    return visual


def _readout_text(x: float, y: float, hit: bool = True) -> bytes:
    if not hit:
        return b"probe outside image"
    measurement = float(_sample_measurement(x, y))
    derived = float(_sample_derived(x, y))
    return f"x {x:.3f}  y {y:.3f}    measurement {measurement:.3f}    derived {derived:.3f}".encode()


def _update_readout(text, x: float, y: float, hit: bool = True) -> None:
    if dvz.dvz_text_set_string(text, _readout_text(x, y, hit)) != 0:
        raise RuntimeError("dvz_text_set_string(readout) failed")


def _add_axes(panel) -> None:
    x_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_X)
    y_axis = dvz.dvz_panel_axis(panel, dvz.DVZ_DIM_Y)
    if not x_axis or not y_axis:
        raise RuntimeError("dvz_panel_axis() failed")

    ticks = dvz.dvz_axis_tick_policy()
    ticks.target_count = 5
    ticks.min_pixel_spacing = 90.0
    ticks.minor_per_interval = 1
    for axis, vertical in ((x_axis, False), (y_axis, True)):
        style = dvz.dvz_axis_style()
        style.tick_size_px = 12.0
        style.label_size_px = 15.0
        style.label_gap_px = 42.0 if vertical else 30.0
        style.grid_color[:] = (96, 165, 250, 120)
        style.spine_color[:] = (217, 226, 236, 190)
        style.major_tick_color[:] = (217, 226, 236, 210)
        style.minor_tick_color[:] = (217, 226, 236, 160)
        if dvz.dvz_axis_set_style(axis, ctypes.byref(style)) != 0:
            raise RuntimeError("dvz_axis_set_style() failed")
        if dvz.dvz_axis_set_grid(axis, True) != 0:
            raise RuntimeError("dvz_axis_set_grid() failed")
        if dvz.dvz_axis_set_tick_policy(axis, ctypes.byref(ticks)) != 0:
            raise RuntimeError("dvz_axis_set_tick_policy() failed")


def _add_colorbar(panel, scale) -> None:
    desc = dvz.dvz_colorbar_desc()
    desc.orientation = dvz.DVZ_COLORBAR_ORIENTATION_VERTICAL
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_RIGHT
    desc.title = b"intensity"
    desc.reserve_px = 112.0
    desc.ramp_width_px = 28.0
    desc.plot_gap_px = 14.0
    desc.tick_length_px = 6.0
    desc.label_gap_px = 7.0
    colorbar = dvz.dvz_colorbar(panel, scale, ctypes.byref(desc))
    if not colorbar:
        raise RuntimeError("dvz_colorbar() failed")
    fmt = dvz.dvz_format_desc()
    fmt.precision = 2
    fmt.trim_trailing_zeros = True
    if dvz.dvz_colorbar_set_format(colorbar, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_colorbar_set_format() failed")


def _data_to_panel_px(panel, x: float, y: float):
    src = (ctypes.c_double * 2)(float(x), float(y))
    dst = (ctypes.c_double * 2)()
    ok = dvz.dvz_panel_data_to_position(panel, dvz.DVZ_PANEL_COORD_PANEL_PX, src, dst)
    if not ok:
        return None
    return float(dst[0]), float(dst[1])


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    grid = dvz.dvz_figure_grid(figure, 1, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")
    margins = dvz.DvzPanelReserve(42.0, 34.0, 46.0, 34.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 28.0, 0.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    source = dvz.dvz_grid_panel(grid, 0, 0)
    derived_panel = dvz.dvz_grid_panel(grid, 0, 1)
    if not source or not derived_panel:
        raise RuntimeError("dvz_grid_panel() failed")
    for panel in (source, derived_panel):
        dvz.dvz_panel_set_background_color(panel, ex.BG)
        _set_image_domain(panel)

    measurement, derived = _fields()
    scale = _add_scale(scene)
    _add_image(scene, source, scale, measurement, True)
    _add_image(scene, derived_panel, scale, derived, False)
    _add_axes(source)
    _add_axes(derived_panel)
    _add_colorbar(derived_panel, scale)

    source_marker = _add_probe_marker(scene, source)
    derived_marker = _add_probe_marker(scene, derived_panel)
    _add_text(source, b"measurement", 96.0, 74.0, 18.0, ex.BLUE)
    _add_text(derived_panel, b"derived", 96.0, 74.0, 18.0, ex.BLUE)
    readout = _add_text(source, _readout_text(PROBE_X, PROBE_Y), 78.0, 56.0, 16.0, ex.TEXT)
    return scene, figure, source, derived_panel, source_marker, derived_marker, readout


def _configure_view(view, scene, source, derived_panel) -> None:
    source_controller, _ = ex.bind_panzoom(view, scene, source, dvz.DVZ_DIM_MASK_XY)
    derived_controller, _ = ex.bind_panzoom(view, scene, derived_panel, dvz.DVZ_DIM_MASK_XY)
    link = dvz.dvz_controller_link(
        scene,
        source_controller,
        derived_controller,
        dvz.DVZ_CONTROLLER_LINK_EXTENT_X | dvz.DVZ_CONTROLLER_LINK_EXTENT_Y,
        dvz.DVZ_CONTROLLER_LINK_TWO_WAY,
    )
    if not link:
        raise RuntimeError("dvz_controller_link() failed")


def _set_probe(source, derived_panel, source_marker, derived_marker, readout, x: float, y: float):
    x = float(np.clip(x, 0.0, 1.0))
    y = float(np.clip(y, 0.0, 1.0))
    _update_probe_marker(source_marker, x, y)
    _update_probe_marker(derived_marker, x, y)
    _update_readout(readout, x, y, True)
    return _data_to_panel_px(source, x, y)


def main() -> None:
    scene, figure, source, derived_panel, source_marker, derived_marker, readout = _build_scene()
    state = {"cursor": None, "last": None}

    def configure(view) -> None:
        _configure_view(view, scene, source, derived_panel)

    def on_pointer(event) -> None:
        if event.type not in (
            dvz.DVZ_POINTER_EVENT_MOVE,
            dvz.DVZ_POINTER_EVENT_CLICK,
            dvz.DVZ_POINTER_EVENT_PRESS,
        ):
            return
        data = None
        for panel in (source, derived_panel):
            panel_pos = ex.figure_to_panel_px(panel, event.pos[0], event.pos[1])
            if panel_pos is None:
                continue
            data = ex.panel_px_to_data(panel, panel_pos[0], panel_pos[1])
            if data is not None:
                break
        if data is None or not (0.0 <= data[0] <= 1.0 and 0.0 <= data[1] <= 1.0):
            return
        state["cursor"] = _set_probe(
            source, derived_panel, source_marker, derived_marker, readout, data[0], data[1]
        )

    def on_frame(_view, frame_index: int, _elapsed: float) -> None:
        if frame_index == 0 and state["cursor"] is None:
            state["cursor"] = _set_probe(
                source, derived_panel, source_marker, derived_marker, readout, PROBE_X, PROBE_Y
            )
        cursor = state["cursor"]
        if cursor is not None:
            ex.queue_panel_query(
                source, cursor[0], cursor[1], PROBE_REQUEST_ID, dvz.DVZ_SCENE_TARGET_PIXEL
            )
        for query in ex.poll_queries(scene):
            if query.request_id != PROBE_REQUEST_ID:
                continue
            data = ex.panel_px_to_data(source, query.panel_position[0], query.panel_position[1])
            if data is None:
                _update_readout(readout, 0.0, 0.0, False)
                state["last"] = None
                continue
            x, y = data
            hit = bool(query.hit) and 0.0 <= x <= 1.0 and 0.0 <= y <= 1.0
            value = (round(x, 3), round(y, 3), hit)
            if value != state["last"]:
                _update_readout(readout, x, y, hit)
                state["last"] = value

    ex.run_with_input_callbacks(
        scene,
        figure,
        "Linked Probe With Colorbar",
        on_pointer=on_pointer,
        on_frame=on_frame,
        configure_view=configure,
    )


if __name__ == "__main__":
    main()
