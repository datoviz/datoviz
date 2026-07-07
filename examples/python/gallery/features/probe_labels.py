#!/usr/bin/env python3
"""Categorical label-image probing with query readback."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_WIDTH = 256
FIELD_HEIGHT = 192
LABEL_IDS = (7, 11, 17, 23, 31)
LABEL_NAMES = ("cortex", "fiber", "nucleus", "vessel", "island")
PROBE_X = 0.68
PROBE_Y = 0.56
PROBE_REQUEST_ID = 1
TAU = 2.0 * math.pi


def _sample_label(x: float, y: float) -> int:
    dx = x - 0.50
    dy = y - 0.50
    tissue = dx * dx / 0.43 + dy * dy / 0.31
    if tissue > 1.0:
        return 0

    island_dx = x - 0.68
    island_dy = y - 0.56
    if island_dx * island_dx + 1.6 * island_dy * island_dy < 0.014:
        return LABEL_IDS[4]

    wave = 0.5 + 0.5 * math.sin(TAU * (1.7 * x + 0.9 * y))
    band = math.floor(4.0 * x + 1.4 * y + 0.45 * wave)
    return LABEL_IDS[band % 4]


def _label_field() -> np.ndarray:
    x = np.linspace(0.0, 1.0, FIELD_WIDTH, dtype=np.float32)
    y = np.linspace(0.0, 1.0, FIELD_HEIGHT, dtype=np.float32)
    labels = np.zeros((FIELD_HEIGHT, FIELD_WIDTH), dtype=np.int32)
    for j, v in enumerate(y):
        for i, u in enumerate(x):
            labels[j, i] = _sample_label(float(u), float(v))
    return labels


def _set_probe_domain(panel) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _labels_scale(scene):
    categories = (
        (LABEL_IDS[0], LABEL_NAMES[0], ex.CYAN),
        (LABEL_IDS[1], LABEL_NAMES[1], ex.GREEN),
        (LABEL_IDS[2], LABEL_NAMES[2], ex.YELLOW),
        (LABEL_IDS[3], LABEL_NAMES[3], ex.RED),
        (LABEL_IDS[4], LABEL_NAMES[4], ex.TEXT),
    )
    return ex.categorical_scale(scene, categories, b"labels")


def _add_labels(scene, panel, scale, labels: np.ndarray):
    field = dvz.dvz_sampled_field_from_array(
        scene,
        labels,
        format=dvz.DVZ_FIELD_FORMAT_R32_SINT,
        semantic=dvz.DVZ_FIELD_SEMANTIC_LABEL,
    )

    visual = dvz.dvz_labels(scene, 0)
    if not visual:
        raise RuntimeError("dvz_labels() failed")
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
    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "texcoords": texcoords,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(labels) failed")
    if dvz.dvz_visual_set_field(visual, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field() failed")
    if dvz.dvz_visual_set_scale(visual, b"labels", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale() failed")
    if dvz.dvz_labels_set_opacity(visual, 0.92) != 0:
        raise RuntimeError("dvz_labels_set_opacity() failed")
    if dvz.dvz_labels_set_background(visual, 0) != 0:
        raise RuntimeError("dvz_labels_set_background() failed")
    if dvz.dvz_labels_set_boundary(visual, True, 1.25, ex.BG) != 0:
        raise RuntimeError("dvz_labels_set_boundary() failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(labels) failed")
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(labels) failed")
    dvz.dvz_visual_set_query_capabilities(visual, dvz.DVZ_QUERY_CAPABILITY_ITEM)
    ex.add_visual(panel, visual)
    return visual


def _add_probe_marker(scene, panel):
    ring = dvz.dvz_marker(scene, 0)
    if not ring:
        raise RuntimeError("dvz_marker() failed")
    if dvz.dvz_visual_set_data_many(
        ring,
        {
            "position": np.array([[PROBE_X, PROBE_Y, 0.04]], dtype=np.float32),
            "color": ex.color_array(ex.TEXT),
            "diameter_px": np.array([24.0], dtype=np.float32),
            "angle": np.array([0.0], dtype=np.float32),
            "shape": np.array([dvz.DVZ_MARKER_SHAPE_RING], dtype=np.uint32),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(ring) failed")
    if dvz.dvz_visual_set_depth_test(ring, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(ring) failed")
    ex.add_visual(panel, ring)

    dot = dvz.dvz_point(scene, 0)
    if not dot:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        dot,
        {
            "position": np.array([[PROBE_X, PROBE_Y, 0.05]], dtype=np.float32),
            "color": ex.color_array(ex.CYAN),
            "diameter_px": np.array([6.0], dtype=np.float32),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(dot) failed")
    ex.set_filled_point_style(dot)
    if dvz.dvz_visual_set_depth_test(dot, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(dot) failed")
    ex.add_visual(panel, dot)
    return ring, dot


def _update_probe_marker(ring, dot, x: float, y: float) -> None:
    ring_pos = np.array([[x, y, 0.04]], dtype=np.float32)
    dot_pos = np.array([[x, y, 0.05]], dtype=np.float32)
    if dvz.dvz_visual_set_data(ring, b"position", ring_pos) != 0:
        raise RuntimeError("dvz_visual_set_data(ring position) failed")
    if dvz.dvz_visual_set_data(dot, b"position", dot_pos) != 0:
        raise RuntimeError("dvz_visual_set_data(dot position) failed")


def _data_to_panel_px(panel, x: float, y: float):
    src = (ctypes.c_double * 2)(float(x), float(y))
    dst = (ctypes.c_double * 2)()
    ok = dvz.dvz_panel_data_to_position(panel, dvz.DVZ_PANEL_COORD_PANEL_PX, src, dst)
    if not ok:
        return None
    return float(dst[0]), float(dst[1])


def _category_name(category_id: int) -> str:
    try:
        return LABEL_NAMES[LABEL_IDS.index(category_id)]
    except ValueError:
        return "background"


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _set_probe_domain(panel)
    labels = _label_field()
    scale = _labels_scale(scene)
    _add_labels(scene, panel, scale, labels)
    ring, dot = _add_probe_marker(scene, panel)

    state = {
        "cursor": _data_to_panel_px(panel, PROBE_X, PROBE_Y),
        "last_hit": None,
        "last_category_id": None,
    }

    def on_pointer(event) -> None:
        if event.type not in (dvz.DVZ_POINTER_EVENT_MOVE, dvz.DVZ_POINTER_EVENT_CLICK):
            return
        panel_pos = ex.figure_to_panel_px(panel, event.pos[0], event.pos[1])
        if panel_pos is None:
            state["cursor"] = None
            return
        data = ex.panel_px_to_data(panel, panel_pos[0], panel_pos[1])
        if data is None:
            state["cursor"] = None
            return
        x = min(max(data[0], 0.0), 1.0)
        y = min(max(data[1], 0.0), 1.0)
        _update_probe_marker(ring, dot, x, y)
        state["cursor"] = panel_pos

    def on_frame(_view, _frame_index: int, _elapsed: float) -> None:
        cursor = state["cursor"]
        if cursor is not None:
            ex.queue_panel_query(
                panel,
                cursor[0],
                cursor[1],
                PROBE_REQUEST_ID,
                dvz.DVZ_SCENE_TARGET_SEGMENT,
            )

        for query in ex.poll_queries(scene):
            if query.request_id != PROBE_REQUEST_ID:
                continue
            hit = bool(query.hit) and query.value_kind == dvz.DVZ_QUERY_VALUE_CATEGORY
            category_id = int(query.category_id) if hit else None
            if state["last_hit"] == hit and state["last_category_id"] == category_id:
                continue

            if hit:
                data = ex.panel_px_to_data(panel, query.panel_position[0], query.panel_position[1])
                if data is None:
                    data = (float(query.uvw[0]), float(query.uvw[1]))
                print(
                    f'probe label_id={category_id} label="{_category_name(category_id)}" '
                    f"data=({data[0]:0.3f},{data[1]:0.3f})"
                )
            else:
                print(
                    f"probe miss panel=({query.panel_position[0]:0.1f},"
                    f"{query.panel_position[1]:0.1f})"
                )
            state["last_hit"] = hit
            state["last_category_id"] = category_id

    ex.run_with_input_callbacks(
        scene,
        figure,
        "Label Probe",
        on_pointer=on_pointer,
        on_frame=on_frame,
    )


if __name__ == "__main__":
    main()
