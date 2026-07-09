#!/usr/bin/env python3
"""Native Datoviz GUI controls updating a retained point visual."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 5
PALETTE_ITEMS = (ctypes.c_char_p * 4)(b"Cyan", b"Amber", b"Violet", b"Slate")


class GuiControlsState:
    def __init__(self, point) -> None:
        self.point = point
        self.diameter_px = ctypes.c_float(42.0)
        self.color = (ctypes.c_float * 4)(0.28, 0.78, 1.00, 1.00)
        self.visible = ctypes.c_bool(True)
        self.pulse = ctypes.c_bool(True)
        self.palette = ctypes.c_int(0)
        self.glyph_count = ctypes.c_int(96)
        self.opacity = ctypes.c_float(1.0)
        self.jitter = (ctypes.c_float * 2)(0.12, -0.08)
        self.contrast = (ctypes.c_float * 4)(0.08, 0.32, 0.72, 0.94)
        self.bloom_enabled = ctypes.c_bool(True)
        self.bloom_radius = ctypes.c_float(3.0)
        self.bloom_threshold = ctypes.c_float(0.62)
        self.contour_enabled = ctypes.c_bool(False)
        self.contour_width = ctypes.c_float(1.4)
        self.contour_min = ctypes.c_float(0.18)
        self.contour_max = ctypes.c_float(0.82)
        self.diagnostic_overlay = ctypes.c_bool(False)
        self.show_histogram = ctypes.c_bool(True)
        self.clip_min = [ctypes.c_float(0.05), ctypes.c_float(0.05), ctypes.c_float(0.05)]
        self.clip_max = [ctypes.c_float(0.95), ctypes.c_float(0.95), ctypes.c_float(0.95)]
        self.light_direction = (ctypes.c_float * 3)(-0.35, 0.55, 0.75)


def _upload(state: GuiControlsState) -> None:
    rgba = np.array(
        [
            int(np.clip(255.0 * state.color[0], 0, 255)),
            int(np.clip(255.0 * state.color[1], 0, 255)),
            int(np.clip(255.0 * state.color[2], 0, 255)),
            int(np.clip(255.0 * state.opacity.value, 0, 255)),
        ],
        dtype=np.uint8,
    )
    colors = np.tile(rgba, (POINT_COUNT, 1))
    diameters = np.full(POINT_COUNT, state.diameter_px.value, dtype=np.float32)
    if dvz.dvz_visual_set_data_many(
        state.point,
        {
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point style) failed")


def _point_positions() -> np.ndarray:
    return np.array(
        [
            [-0.70, -0.35, 0.0],
            [-0.35, +0.20, 0.0],
            [+0.00, -0.10, 0.0],
            [+0.35, +0.35, 0.0],
            [+0.70, -0.20, 0.0],
        ],
        dtype=np.float32,
    )


def _build_scene():
    scene, figure, panel = ex.scene_panel()

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")

    state = GuiControlsState(point)
    if dvz.dvz_visual_set_data(point, "position", _point_positions()) != 0:
        raise RuntimeError("dvz_visual_set_data(position) failed")
    _upload(state)

    ex.set_filled_point_style(point)
    if dvz.dvz_visual_set_depth_test(point, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")
    ex.add_visual(panel, point)
    return scene, figure, panel, state


def _reset_mock_values(state: GuiControlsState) -> None:
    state.bloom_radius.value = 3.0
    state.bloom_threshold.value = 0.62
    state.contour_width.value = 1.4
    state.contour_min.value = 0.18
    state.contour_max.value = 0.82
    for value in state.clip_min:
        value.value = 0.05
    for value in state.clip_max:
        value.value = 0.95


def _gui_callback_factory(state: GuiControlsState):
    def gui_callback(gui, _view, _user_data) -> None:
        changed = False
        visible_changed = False
        if dvz.dvz_gui_begin(gui, b"Widget controls", None, 0):
            dvz.dvz_gui_separator_text(gui, b"Marker")
            changed |= dvz.dvz_gui_slider_float(
                gui,
                b"Diameter##gui_controls_marker_diameter",
                ctypes.byref(state.diameter_px),
                8.0,
                96.0,
            )
            changed |= dvz.dvz_gui_color_edit4(
                gui, b"Tint##gui_controls_marker_tint", state.color, 0
            )
            changed |= dvz.dvz_gui_slider_float(
                gui, b"Alpha##gui_controls_marker_alpha", ctypes.byref(state.opacity), 0.15, 1.0
            )
            visible_changed |= dvz.dvz_gui_checkbox(
                gui, b"Visible##gui_controls_marker_visible", ctypes.byref(state.visible)
            )
            dvz.dvz_gui_checkbox(gui, b"Pulse preview##gui_controls_marker_pulse", ctypes.byref(state.pulse))

            dvz.dvz_gui_separator_text(gui, b"Synthetic data")
            dvz.dvz_gui_combo(
                gui,
                b"Palette##gui_controls_data_palette",
                ctypes.byref(state.palette),
                PALETTE_ITEMS,
                len(PALETTE_ITEMS),
            )
            dvz.dvz_gui_slider_int(
                gui,
                b"Sample count##gui_controls_data_sample_count",
                ctypes.byref(state.glyph_count),
                16,
                256,
            )
            dvz.dvz_gui_slider_float2(
                gui, b"Jitter XY##gui_controls_data_jitter_xy", state.jitter, -1.0, 1.0
            )
            dvz.dvz_gui_slider_float4(
                gui, b"Contrast curve##gui_controls_data_contrast_curve", state.contrast, 0.0, 1.0
            )

            if dvz.dvz_gui_collapsing_header(gui, b"Mock effects##gui_controls_effects_section", 0):
                dvz.dvz_gui_checkbox(
                    gui,
                    b"Bloom enabled##gui_controls_effects_bloom_enabled",
                    ctypes.byref(state.bloom_enabled),
                )
                dvz.dvz_gui_slider_float_format(
                    gui,
                    b"Bloom radius##gui_controls_effects_bloom_radius",
                    ctypes.byref(state.bloom_radius),
                    0.5,
                    12.0,
                    b"%.1f px",
                )
                dvz.dvz_gui_slider_float(
                    gui,
                    b"Bloom threshold##gui_controls_effects_bloom_threshold",
                    ctypes.byref(state.bloom_threshold),
                    0.0,
                    1.0,
                )
                dvz.dvz_gui_checkbox(
                    gui,
                    b"Contours enabled##gui_controls_effects_contours_enabled",
                    ctypes.byref(state.contour_enabled),
                )
                dvz.dvz_gui_slider_float(
                    gui,
                    b"Contour width##gui_controls_effects_contour_width",
                    ctypes.byref(state.contour_width),
                    0.25,
                    5.0,
                )
                dvz.dvz_gui_slider_range_float(
                    gui,
                    b"Contour range##gui_controls_effects_contour_range",
                    ctypes.byref(state.contour_min),
                    ctypes.byref(state.contour_max),
                    0.0,
                    1.0,
                    b"%.2f",
                )

            if dvz.dvz_gui_collapsing_header(gui, b"Mock volume##gui_controls_volume_section", 0):
                dvz.dvz_gui_slider_float3(
                    gui,
                    b"Light vector##gui_controls_volume_light_vector",
                    state.light_direction,
                    -1.0,
                    1.0,
                )
                for axis, label in enumerate((b"Clip X", b"Clip Y", b"Clip Z")):
                    dvz.dvz_gui_range_float(
                        gui,
                        label + b"##gui_controls_volume_clip_" + bytes([ord("x") + axis]),
                        ctypes.byref(state.clip_min[axis]),
                        ctypes.byref(state.clip_max[axis]),
                        0.01,
                        0.0,
                        1.0,
                        b"%.2f",
                    )

            dvz.dvz_gui_separator_text(gui, b"Diagnostics")
            dvz.dvz_gui_checkbox(
                gui,
                b"Overlay##gui_controls_diagnostics_overlay",
                ctypes.byref(state.diagnostic_overlay),
            )
            dvz.dvz_gui_same_line(gui, 0.0, -1.0)
            dvz.dvz_gui_checkbox(
                gui,
                b"Histogram##gui_controls_diagnostics_histogram",
                ctypes.byref(state.show_histogram),
            )
            if dvz.dvz_gui_button(gui, b"Reset mock values##gui_controls_diagnostics_reset"):
                _reset_mock_values(state)
        dvz.dvz_gui_end(gui)

        if changed:
            _upload(state)
        if visible_changed:
            dvz.dvz_visual_set_visible(state.point, state.visible.value)

    return gui_callback


def _configure_view(view, state: GuiControlsState, *, gui: bool = True) -> None:
    if not gui:
        return
    overlay = dvz.dvz_view_gui(view, None)
    if not overlay:
        return
    if dvz.dvz_view_set_gui_callback(view, _gui_callback_factory(state), None) != 0:
        raise RuntimeError("dvz_view_set_gui_callback() failed")


def main() -> None:
    scene, figure, _panel, state = _build_scene()

    def configure(view) -> None:
        _configure_view(view, state)

    ex.run_with_view(scene, figure, "GUI Controls", configure)


if __name__ == "__main__":
    main()
