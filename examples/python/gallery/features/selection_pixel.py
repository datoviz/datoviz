#!/usr/bin/env python3
"""Pixel-grid item selection with hover and click feedback."""

from __future__ import annotations

import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


GRID_WIDTH = 40
GRID_HEIGHT = 24
PIXEL_COUNT = GRID_WIDTH * GRID_HEIGHT
PIXEL_SIZE = 18.0
QUERY_HOVER_ID = 11
QUERY_CLICK_ID = 12
TAU = 2.0 * math.pi


def _clamp01(value: float) -> float:
    return min(max(value, 0.0), 1.0)


def _sample_value(u: float, v: float) -> float:
    ridge = 0.5 + 0.5 * math.sin(TAU * (1.7 * u + 0.35 * v))
    wave = 0.5 + 0.5 * math.cos(TAU * (0.6 * u - 1.9 * v))
    dx = u - 0.34
    dy = v - 0.64
    blob = math.exp(-(dx * dx + 1.6 * dy * dy) / 0.012)
    value = 0.08 + 0.36 * u + 0.18 * v + 0.16 * ridge + 0.10 * wave + 0.28 * blob
    return _clamp01(value)


def _mix(a, b, t: float):
    return (
        int((1.0 - t) * a.r + t * b.r + 0.5),
        int((1.0 - t) * a.g + t * b.g + 0.5),
        int((1.0 - t) * a.b + t * b.b + 0.5),
        245,
    )


def _ramp(t: float):
    t = _clamp01(t)
    c0 = dvz.DvzColor(42, 56, 71, 255)
    c1 = ex.CYAN
    c2 = ex.GREEN
    if t < 0.5:
        return _mix(c0, c1, 2.0 * t)
    return _mix(c1, c2, 2.0 * (t - 0.5))


def _pixel_data():
    positions = np.zeros((PIXEL_COUNT, 3), dtype=np.float32)
    colors = np.zeros((PIXEL_COUNT, 4), dtype=np.uint8)
    sizes = np.full(PIXEL_COUNT, PIXEL_SIZE, dtype=np.float32)

    step_x = 1.82 / (GRID_WIDTH - 1)
    step_y = 1.36 / (GRID_HEIGHT - 1)
    for y in range(GRID_HEIGHT):
        for x in range(GRID_WIDTH):
            index = y * GRID_WIDTH + x
            u = x / (GRID_WIDTH - 1)
            v = y / (GRID_HEIGHT - 1)
            positions[index] = (-0.91 + step_x * x, -0.68 + step_y * y, 0.0)
            colors[index] = _ramp(_sample_value(u, v))
    return positions, colors, sizes


def _add_pixel_grid(scene, panel):
    pixel = dvz.dvz_pixel(scene, 0)
    if not pixel:
        raise RuntimeError("dvz_pixel() failed")
    dvz.dvz_visual_set_query_capabilities(pixel, dvz.DVZ_QUERY_CAPABILITY_ITEM)

    positions, colors, sizes = _pixel_data()
    if dvz.dvz_visual_set_data_many(
        pixel,
        {
            "position": positions,
            "color": colors,
            "pixel_size_px": sizes,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(pixel) failed")
    if dvz.dvz_visual_set_depth_test(pixel, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(pixel) failed")
    ex.add_visual(panel, pixel)
    return pixel


def _handle_hover(state, query) -> None:
    if not state["cursor_valid"]:
        return
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_PIXEL, PIXEL_COUNT):
        latest = state["latest_hover_query"]
        same_item = (
            latest is not None
            and latest.visual_id == query.visual_id
            and latest.resolved_id == query.resolved_id
        )
        if not same_item:
            if dvz.dvz_hover_apply_query(state["hover"], query) != 0:
                raise RuntimeError("dvz_hover_apply_query() failed")
            print(f"hover pixel id={query.resolved_id}")
        state["latest_hover_query"] = query
        state["has_hover_query"] = True
    else:
        if state["has_hover_query"]:
            dvz.dvz_hover_clear(state["hover"])
        state["latest_hover_query"] = None
        state["has_hover_query"] = False


def _handle_click(state, query) -> None:
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_PIXEL, PIXEL_COUNT):
        if dvz.dvz_selection_apply_query(state["selection"], query) != 0:
            raise RuntimeError("dvz_selection_apply_query() failed")
        print(f"toggle pixel id={query.resolved_id}")
    else:
        dvz.dvz_selection_clear(state["selection"])


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _add_pixel_grid(scene, panel)
    selection = ex.create_item_selection(scene)
    hover = ex.create_item_hover(scene, 1.7)

    state = {
        "selection": selection,
        "hover": hover,
        "cursor_valid": False,
        "has_hover_query": False,
        "latest_hover_query": None,
    }

    def configure_view(view) -> None:
        ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)

    def on_pointer(event) -> None:
        if event.type not in (
            dvz.DVZ_POINTER_EVENT_MOVE,
            dvz.DVZ_POINTER_EVENT_PRESS,
            dvz.DVZ_POINTER_EVENT_CLICK,
        ):
            return

        panel_pos = ex.figure_to_panel_px(panel, event.pos[0], event.pos[1])
        state["cursor_valid"] = panel_pos is not None
        if panel_pos is None:
            if state["has_hover_query"]:
                dvz.dvz_hover_clear(hover)
                state["has_hover_query"] = False
            if event.type in (dvz.DVZ_POINTER_EVENT_PRESS, dvz.DVZ_POINTER_EVENT_CLICK):
                dvz.dvz_selection_clear(selection)
            return

        if event.type == dvz.DVZ_POINTER_EVENT_MOVE:
            ex.queue_panel_query(
                panel,
                panel_pos[0],
                panel_pos[1],
                QUERY_HOVER_ID,
                dvz.DVZ_SCENE_TARGET_ITEM,
                hit_policy=dvz.DVZ_QUERY_HIT_FRONTMOST,
            )
        elif event.button == dvz.DVZ_POINTER_BUTTON_LEFT:
            ex.queue_panel_query(
                panel,
                panel_pos[0],
                panel_pos[1],
                QUERY_CLICK_ID,
                dvz.DVZ_SCENE_TARGET_ITEM,
                hit_policy=dvz.DVZ_QUERY_HIT_FRONTMOST,
            )

    def on_frame(_view, _frame_index: int, _elapsed: float) -> None:
        for query in ex.poll_queries(scene):
            if query.request_id == QUERY_CLICK_ID:
                _handle_click(state, query)
            elif query.request_id == QUERY_HOVER_ID:
                _handle_hover(state, query)

    ex.run_with_input_callbacks(
        scene,
        figure,
        "Pixel Selection",
        on_pointer=on_pointer,
        on_frame=on_frame,
        configure_view=configure_view,
    )


if __name__ == "__main__":
    main()
