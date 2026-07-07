#!/usr/bin/env python3
"""Sphere item selection with depth-aware frontmost picking."""

from __future__ import annotations

import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SPHERE_COUNT = 54
QUERY_HOVER_ID = 17
QUERY_CLICK_ID = 18
TAU = 2.0 * math.pi


def _sphere_data():
    positions = np.zeros((SPHERE_COUNT, 3), dtype=np.float32)
    radii = np.zeros(SPHERE_COUNT, dtype=np.float32)
    colors = np.zeros((SPHERE_COUNT, 4), dtype=np.uint8)
    palette = (ex.YELLOW, ex.CYAN, ex.GREEN, ex.RED)

    for i in range(SPHERE_COUNT):
        t = i / (SPHERE_COUNT - 1)
        angle = TAU * (0.132 * i + 0.08 * math.sin(9.0 * t))
        layer = 2.0 * t - 1.0
        ring = 0.34 + 0.58 * math.sqrt(1.0 - 0.60 * layer * layer)
        wobble = 0.5 + 0.5 * math.sin(19.0 * t + 0.35)

        positions[i] = (
            ring * math.cos(angle),
            0.72 * layer + 0.10 * math.sin(3.0 * angle),
            ring * math.sin(angle) + 0.24 * layer,
        )
        radii[i] = 0.055 + 0.065 * (1.0 - abs(layer)) + 0.026 * wobble
        color = palette[i % len(palette)]
        colors[i] = (color.r, color.g, color.b, 245)

    return positions, radii, colors


def _add_sphere_cluster(scene, panel):
    sphere = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not sphere:
        raise RuntimeError("dvz_sphere() failed")
    dvz.dvz_visual_set_query_capabilities(sphere, dvz.DVZ_QUERY_CAPABILITY_ITEM)
    if dvz.dvz_sphere_set_mode(sphere, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")

    positions, radii, colors = _sphere_data()
    if dvz.dvz_visual_set_data_many(
        sphere,
        {
            "position": positions,
            "radius": radii,
            "color": colors,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(sphere) failed")
    ex.add_visual(panel, sphere)
    return sphere


def _handle_hover(state, query) -> None:
    if not state["cursor_valid"]:
        return
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_SPHERE, SPHERE_COUNT):
        latest = state["latest_hover_query"]
        same_item = (
            latest is not None
            and latest.visual_id == query.visual_id
            and latest.resolved_id == query.resolved_id
        )
        if not same_item:
            if dvz.dvz_hover_apply_query(state["hover"], query) != 0:
                raise RuntimeError("dvz_hover_apply_query() failed")
            print(f"hover sphere id={query.resolved_id}")
        state["latest_hover_query"] = query
        state["has_hover_query"] = True
    else:
        if state["has_hover_query"]:
            dvz.dvz_hover_clear(state["hover"])
        state["latest_hover_query"] = None
        state["has_hover_query"] = False


def _handle_click(state, query) -> None:
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_SPHERE, SPHERE_COUNT):
        if dvz.dvz_selection_apply_query(state["selection"], query) != 0:
            raise RuntimeError("dvz_selection_apply_query() failed")
        print(f"toggle sphere id={query.resolved_id}")
    else:
        dvz.dvz_selection_clear(state["selection"])


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    _add_sphere_cluster(scene, panel)
    selection = ex.create_item_selection(scene)
    hover = ex.create_item_hover(scene, 1.35)

    state = {
        "selection": selection,
        "hover": hover,
        "cursor_valid": False,
        "has_hover_query": False,
        "latest_hover_query": None,
    }

    def configure_view(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")

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
        "Sphere Selection",
        on_pointer=on_pointer,
        on_frame=on_frame,
        configure_view=configure_view,
    )


if __name__ == "__main__":
    main()
