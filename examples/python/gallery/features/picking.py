#!/usr/bin/env python3
"""Marker item picking with hover and click selection."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


GRID_COLS = 9
GRID_ROWS = 6
MARKER_COUNT = GRID_COLS * GRID_ROWS + 5
BASE_SIZE = 34.0
HOVER_SIZE = 50.0
QUERY_HOVER_ID = 1
QUERY_CLICK_ID = 2


def _marker_shape(index: int) -> int:
    shapes = (
        dvz.DVZ_MARKER_SHAPE_DISC,
        dvz.DVZ_MARKER_SHAPE_SQUARE,
        dvz.DVZ_MARKER_SHAPE_TRIANGLE,
        dvz.DVZ_MARKER_SHAPE_DIAMOND,
        dvz.DVZ_MARKER_SHAPE_CROSS,
        dvz.DVZ_MARKER_SHAPE_RING,
    )
    return shapes[index % len(shapes)]


def _marker_palette(index: int):
    palette = (ex.CYAN, ex.GREEN, ex.TEXT)
    color = palette[index % len(palette)]
    return color.r, color.g, color.b, 245


def _marker_data():
    positions = np.zeros((MARKER_COUNT, 3), dtype=np.float32)
    colors = np.zeros((MARKER_COUNT, 4), dtype=np.uint8)
    diameters = np.zeros(MARKER_COUNT, dtype=np.float32)
    angles = np.zeros(MARKER_COUNT, dtype=np.float32)
    shapes = np.zeros(MARKER_COUNT, dtype=np.uint32)

    for row in range(GRID_ROWS):
        for col in range(GRID_COLS):
            index = row * GRID_COLS + col
            positions[index] = (
                -0.88 + 1.76 * (col / (GRID_COLS - 1)),
                -0.72 + 1.44 * (row / (GRID_ROWS - 1)),
                0.0,
            )
            colors[index] = _marker_palette(index)
            diameters[index] = BASE_SIZE
            angles[index] = ((index % 12) / 12.0) * 2.0 * np.pi
            shapes[index] = _marker_shape(index)

    for i in range(5):
        index = GRID_COLS * GRID_ROWS + i
        positions[index] = (-0.05 + 0.025 * i, -0.02 + 0.020 * (i % 3), 0.0)
        colors[index] = (ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 245)
        diameters[index] = BASE_SIZE
        angles[index] = 0.25 * np.pi * i
        shapes[index] = _marker_shape(index + 2)

    return positions, colors, diameters, angles.astype(np.float32), shapes


def _add_marker_grid(scene, panel):
    marker = dvz.dvz_marker(scene, 0)
    if not marker:
        raise RuntimeError("dvz_marker() failed")
    dvz.dvz_visual_set_query_capabilities(marker, dvz.DVZ_QUERY_CAPABILITY_ITEM)

    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE
    style.edge_color = dvz.DvzColor(ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, 210)
    style.stroke_width_px = 2.0
    if dvz.dvz_marker_set_style(marker, style) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")

    positions, colors, diameters, angles, shapes = _marker_data()
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
    ex.add_visual(panel, marker)
    return marker


def _handle_hover(state, query) -> None:
    if not state["cursor_valid"]:
        return
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_MARKER, MARKER_COUNT):
        latest = state["latest_hover_query"]
        same_item = (
            latest is not None
            and latest.visual_id == query.visual_id
            and latest.resolved_id == query.resolved_id
        )
        if not same_item:
            if dvz.dvz_hover_apply_query(state["hover"], query) != 0:
                raise RuntimeError("dvz_hover_apply_query() failed")
            print(f"hover marker id={query.resolved_id}")
        state["latest_hover_query"] = query
        state["has_hover_query"] = True
    else:
        if state["has_hover_query"]:
            dvz.dvz_hover_clear(state["hover"])
        state["latest_hover_query"] = None
        state["has_hover_query"] = False


def _handle_click(state, query) -> None:
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_MARKER, MARKER_COUNT):
        if dvz.dvz_selection_apply_query(state["selection"], query) != 0:
            raise RuntimeError("dvz_selection_apply_query() failed")
        print(f"toggle marker id={query.resolved_id}")
    else:
        dvz.dvz_selection_clear(state["selection"])


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    _add_marker_grid(scene, panel)
    selection = ex.create_item_selection(scene)
    hover = ex.create_item_hover(scene, HOVER_SIZE / BASE_SIZE)

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
        "Picking",
        on_pointer=on_pointer,
        on_frame=on_frame,
        configure_view=configure_view,
    )


if __name__ == "__main__":
    main()
