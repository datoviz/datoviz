#!/usr/bin/env python3
"""Instanced mesh item selection with hover and click feedback."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


GRID_X = 6
GRID_Y = 4
GRID_Z = 2
INSTANCE_COUNT = GRID_X * GRID_Y * GRID_Z
QUERY_HOVER_ID = 23
QUERY_CLICK_ID = 24
TAU = 2.0 * math.pi


def _cube_transform(tx, ty, tz, scale, angle_x, angle_z, angle_y):
    cx = math.cos(angle_x)
    sx = math.sin(angle_x)
    cz = math.cos(angle_z)
    sz = math.sin(angle_z)
    cy = math.cos(angle_y)
    sy = math.sin(angle_y)
    return np.array(
        [
            [scale * cz * cy, scale * (cz * sy * sx - sz * cx), scale * (cz * sy * cx + sz * sx), tx],
            [scale * sz * cy, scale * (sz * sy * sx + cz * cx), scale * (sz * sy * cx - cz * sx), ty],
            [scale * -sy, scale * cy * sx, scale * cy * cx, tz],
            [0.0, 0.0, 0.0, 1.0],
        ],
        dtype=np.float32,
    ).T


def _cube_transforms():
    transforms = np.zeros((INSTANCE_COUNT, 4, 4), dtype=np.float32)
    spacing = 0.50
    half_x = 0.5 * (GRID_X - 1)
    half_y = 0.5 * (GRID_Y - 1)
    half_z = 0.5 * (GRID_Z - 1)
    idx = 0
    for z in range(GRID_Z):
        for y in range(GRID_Y):
            for x in range(GRID_X):
                fx = (x - half_x) * spacing + 0.035 * math.sin(1.70 * x + 0.90 * y)
                fy = (y - half_y) * spacing + 0.030 * math.cos(1.10 * y + 0.80 * z)
                fz = (z - half_z) * spacing + 0.045 * math.sin(0.80 * x - 1.15 * y)
                wave = 0.5 + 0.5 * math.sin(TAU * (x / GRID_X + z * 0.11))
                scale = 0.62 + 0.30 * wave
                angle_x = 0.22 * math.sin(0.95 * x + 1.55 * y + 0.70 * z)
                angle_z = 0.10 * x + 0.17 * y
                angle_y = 0.24 * z - 0.06 * y + 0.14 * math.cos(1.20 * x + 0.50 * y)
                transforms[idx] = _cube_transform(fx, fy, fz, scale, angle_x, angle_z, angle_y)
                idx += 1
    return transforms


def _selection_cube_mesh(scene):
    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")

    face_colors = (dvz.DvzColor * 6)(
        dvz.DvzColor(76, 201, 240, 255),
        dvz.DvzColor(38, 132, 167, 255),
        dvz.DvzColor(128, 255, 219, 255),
        dvz.DvzColor(42, 176, 142, 255),
        dvz.DvzColor(132, 142, 239, 255),
        dvz.DvzColor(176, 112, 221, 255),
    )
    desc = dvz.dvz_geometry_cube_desc()
    desc.size = 0.32
    desc.face_colors = face_colors
    desc.face_color_count = len(face_colors)
    geometry = dvz.dvz_geometry_cube(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_cube() failed")
    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)

    dvz.dvz_visual_set_query_capabilities(mesh, dvz.DVZ_QUERY_CAPABILITY_ITEM)
    transforms = _cube_transforms()
    if dvz.dvz_visual_set_data(mesh, b"instance_transform", transforms) != 0:
        raise RuntimeError("dvz_visual_set_data(instance_transform) failed")
    return mesh


def _handle_hover(state, query) -> None:
    if not state["cursor_valid"]:
        return
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_MESH, INSTANCE_COUNT):
        latest = state["latest_hover_query"]
        same_item = (
            latest is not None
            and latest.visual_id == query.visual_id
            and latest.resolved_id == query.resolved_id
        )
        if not same_item:
            if dvz.dvz_hover_apply_query(state["hover"], query) != 0:
                raise RuntimeError("dvz_hover_apply_query() failed")
            print(f"hover mesh instance id={query.resolved_id}")
        state["latest_hover_query"] = query
        state["has_hover_query"] = True
    else:
        if state["has_hover_query"]:
            dvz.dvz_hover_clear(state["hover"])
        state["latest_hover_query"] = None
        state["has_hover_query"] = False


def _handle_click(state, query) -> None:
    if ex.query_item_hit(query, dvz.DVZ_SCENE_VISUAL_FAMILY_MESH, INSTANCE_COUNT):
        if dvz.dvz_selection_apply_query(state["selection"], query) != 0:
            raise RuntimeError("dvz_selection_apply_query() failed")
        print(f"toggle mesh instance id={query.resolved_id}")
    else:
        dvz.dvz_selection_clear(state["selection"])


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    mesh = _selection_cube_mesh(scene)
    ex.add_visual(panel, mesh)
    selection = ex.create_item_selection(scene)
    hover = ex.create_item_hover(scene, 1.24)

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

        request_id = QUERY_HOVER_ID if event.type == dvz.DVZ_POINTER_EVENT_MOVE else QUERY_CLICK_ID
        if request_id == QUERY_CLICK_ID and event.button != dvz.DVZ_POINTER_BUTTON_LEFT:
            return
        ex.queue_panel_query(
            panel,
            panel_pos[0],
            panel_pos[1],
            request_id,
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
        "Mesh Instance Selection",
        on_pointer=on_pointer,
        on_frame=on_frame,
        configure_view=configure_view,
    )


if __name__ == "__main__":
    main()
