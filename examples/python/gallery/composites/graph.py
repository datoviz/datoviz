#!/usr/bin/env python3
"""Semantic graph composite with community-colored nodes and Bezier edges."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


COMMUNITIES = (
    dvz.DvzColor(65, 201, 226, 255),
    dvz.DvzColor(246, 185, 72, 255),
    dvz.DvzColor(234, 104, 91, 255),
)

NODES = (
    ("V1", 0, (-1.05, +0.42, 0.0), 101, 0.88),
    ("V2", 0, (-0.80, +0.62, 0.0), 102, 0.72),
    ("V4", 0, (-0.55, +0.42, 0.0), 103, 0.76),
    ("LGN", 0, (-0.92, +0.12, 0.0), 104, 0.66),
    ("MT", 0, (-0.62, +0.12, 0.0), 105, 0.70),
    ("M1", 1, (+0.55, +0.45, 0.0), 201, 0.86),
    ("S1", 1, (+0.82, +0.62, 0.0), 202, 0.78),
    ("SMA", 1, (+1.05, +0.36, 0.0), 203, 0.70),
    ("PMd", 1, (+0.70, +0.15, 0.0), 204, 0.67),
    ("Thal", 1, (+1.00, +0.08, 0.0), 205, 0.80),
    ("HPC-L", 2, (-0.30, -0.55, 0.0), 301, 0.84),
    ("HPC-R", 2, (+0.00, -0.78, 0.0), 302, 0.82),
    ("Ent", 2, (-0.58, -0.70, 0.0), 303, 0.63),
    ("PCC", 2, (+0.34, -0.50, 0.0), 304, 0.74),
    ("Amy", 2, (+0.55, -0.72, 0.0), 305, 0.60),
)

EDGES = (
    (0, 1, 1001, 0.84, False),
    (1, 2, 1002, 0.76, False),
    (0, 3, 1003, 0.62, False),
    (3, 4, 1004, 0.58, False),
    (2, 4, 1005, 0.71, False),
    (0, 4, 1006, 0.66, False),
    (5, 6, 2001, 0.82, False),
    (6, 7, 2002, 0.70, False),
    (5, 8, 2003, 0.64, False),
    (8, 9, 2004, 0.68, False),
    (7, 9, 2005, 0.74, False),
    (5, 9, 2006, 0.60, False),
    (10, 11, 3001, 0.86, False),
    (10, 12, 3002, 0.72, False),
    (11, 13, 3003, 0.66, False),
    (12, 14, 3004, 0.57, False),
    (13, 14, 3005, 0.61, False),
    (10, 13, 3006, 0.64, False),
    (2, 5, 4001, 0.48, True),
    (4, 8, 4002, 0.42, True),
    (1, 6, 4003, 0.35, True),
    (3, 12, 4004, 0.44, True),
    (4, 10, 4005, 0.39, True),
    (8, 13, 4006, 0.50, True),
    (9, 14, 4007, 0.46, True),
    (5, 13, 4008, 0.37, True),
)


def _positions() -> np.ndarray:
    return np.array([node[2] for node in NODES], dtype=np.float64)


def _edge_endpoints() -> np.ndarray:
    return np.array([(edge[0], edge[1]) for edge in EDGES], dtype=np.uint32).reshape(-1)


def _default_edge_controls(p0: np.ndarray, p3: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    delta = p3 - p0
    length = float(np.hypot(delta[0], delta[1]))
    bend = 0.18 * length if length > 0.0 else 0.0
    normal = np.array((-delta[1] / length, delta[0] / length, 0.0)) if length > 0.0 else 0.0
    c0 = p0 + delta / 3.0 + normal * bend
    c1 = p0 + 2.0 * delta / 3.0 + normal * bend
    return c0, c1


def _bridge_edge_controls(edge_index: int, bridge_index: int, positions: np.ndarray):
    source, target, _semantic_id, weight, _bridge = EDGES[edge_index]
    p0 = positions[source]
    p3 = positions[target]
    delta = p3 - p0
    bend = (1.0 if bridge_index % 2 == 0 else -1.0) * (0.14 + 0.08 * weight)
    offset = np.array((-bend * delta[1], bend * delta[0], 0.0), dtype=np.float64)
    c0 = p0 + 0.36 * delta + offset
    c1 = p0 + 0.64 * delta + offset
    return c0, c1


def _add_cubic_bounds(bounds: list[float], p0, c0, c1, p3) -> None:
    for t in np.linspace(0.0, 1.0, 33):
        u = 1.0 - t
        point = u * u * u * p0 + 3.0 * u * u * t * c0 + 3.0 * u * t * t * c1 + t * t * t * p3
        bounds[0] = min(bounds[0], float(point[0]))
        bounds[1] = max(bounds[1], float(point[0]))
        bounds[2] = min(bounds[2], float(point[1]))
        bounds[3] = max(bounds[3], float(point[1]))


def _graph_bounds(positions: np.ndarray) -> tuple[float, float, float, float]:
    bounds = [
        float(np.min(positions[:, 0])),
        float(np.max(positions[:, 0])),
        float(np.min(positions[:, 1])),
        float(np.max(positions[:, 1])),
    ]
    bridge_index = 0
    for edge_index, (source, target, _semantic_id, _weight, bridge) in enumerate(EDGES):
        p0 = positions[source]
        p3 = positions[target]
        if bridge:
            c0, c1 = _bridge_edge_controls(edge_index, bridge_index, positions)
            bridge_index += 1
        else:
            c0, c1 = _default_edge_controls(p0, p3)
        _add_cubic_bounds(bounds, p0, c0, c1, p3)
    return bounds[0], bounds[1], bounds[2], bounds[3]


def _set_equal_view2d(panel, positions: np.ndarray) -> None:
    padding = dvz.DvzPanelReserve()
    padding.left_px = 24.0
    padding.right_px = 24.0
    padding.top_px = 18.0
    padding.bottom_px = 18.0
    if dvz.dvz_panel_set_padding(panel, ctypes.byref(padding)) != 0:
        raise RuntimeError("dvz_panel_set_padding() failed")

    xmin, xmax, ymin, ymax = _graph_bounds(positions)
    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.14
    desc.domain_x[:] = (xmin, xmax)
    desc.domain_y[:] = (ymin, ymax)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def _color_array(colors: list[dvz.DvzColor]):
    array_type = dvz.DvzColor * len(colors)
    return array_type(*colors)


def _float_array(values):
    array_type = ctypes.c_float * len(values)
    return array_type(*(float(value) for value in values))


def _uint64_array(values: list[int]):
    array_type = ctypes.c_uint64 * len(values)
    return array_type(*values)


def _add_graph(scene, panel, positions: np.ndarray) -> None:
    graph = dvz.dvz_graph(scene, 0)
    if not graph:
        raise RuntimeError("dvz_graph() failed")

    endpoints = _edge_endpoints()
    if dvz.dvz_graph_set_node_count(graph, len(NODES)) != 0:
        raise RuntimeError("dvz_graph_set_node_count() failed")
    if (
        dvz.dvz_graph_set_node_positions(
            graph, 0, len(NODES), ctypes.c_void_p(positions.ctypes.data)
        )
        != 0
    ):
        raise RuntimeError("dvz_graph_set_node_positions() failed")
    if dvz.dvz_graph_set_edge_count(graph, len(EDGES)) != 0:
        raise RuntimeError("dvz_graph_set_edge_count() failed")
    if (
        dvz.dvz_graph_set_edge_endpoints(
            graph, 0, len(EDGES), endpoints.ctypes.data_as(ctypes.POINTER(ctypes.c_uint32))
        )
        != 0
    ):
        raise RuntimeError("dvz_graph_set_edge_endpoints() failed")

    node_ids = _uint64_array([node[3] for node in NODES])
    edge_ids = _uint64_array([edge[2] for edge in EDGES])
    if dvz.dvz_graph_set_node_ids(graph, 0, len(NODES), node_ids) != 0:
        raise RuntimeError("dvz_graph_set_node_ids() failed")
    if dvz.dvz_graph_set_edge_ids(graph, 0, len(EDGES), edge_ids) != 0:
        raise RuntimeError("dvz_graph_set_edge_ids() failed")

    node_colors = _color_array([COMMUNITIES[node[1]] for node in NODES])
    node_sizes = np.array([18.0 + 24.0 * node[4] for node in NODES], dtype=np.float32)
    if dvz.dvz_graph_set_node_colors(graph, 0, len(NODES), node_colors) != 0:
        raise RuntimeError("dvz_graph_set_node_colors() failed")
    if dvz.dvz_graph_set_node_sizes(graph, 0, len(NODES), _float_array(node_sizes)) != 0:
        raise RuntimeError("dvz_graph_set_node_sizes() failed")

    edge_colors = []
    edge_widths = np.empty(len(EDGES), dtype=np.float32)
    for i, (source, _target, _semantic_id, weight, bridge) in enumerate(EDGES):
        if bridge:
            edge_colors.append(dvz.DvzColor(222, 236, 244, 185))
        else:
            community = NODES[source][1]
            color = COMMUNITIES[community]
            edge_colors.append(dvz.DvzColor(color.r, color.g, color.b, 105))
        edge_widths[i] = 1.1 + 4.2 * weight

    if dvz.dvz_graph_set_edge_colors(graph, 0, len(EDGES), _color_array(edge_colors)) != 0:
        raise RuntimeError("dvz_graph_set_edge_colors() failed")
    if dvz.dvz_graph_set_edge_widths(graph, 0, len(EDGES), _float_array(edge_widths)) != 0:
        raise RuntimeError("dvz_graph_set_edge_widths() failed")

    style = dvz.dvz_graph_edge_style()
    style.mode = dvz.DVZ_GRAPH_EDGE_MODE_BEZIER
    style.tessellation_segment_count = 22
    if dvz.dvz_graph_set_edge_style(graph, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_graph_set_edge_style() failed")

    bridge_first_edge = next((i for i, edge in enumerate(EDGES) if edge[4]), len(EDGES))
    bridge_count = len(EDGES) - bridge_first_edge
    if bridge_count:
        controls0 = np.empty((bridge_count, 3), dtype=np.float64)
        controls1 = np.empty((bridge_count, 3), dtype=np.float64)
        for bridge_index in range(bridge_count):
            edge_index = bridge_first_edge + bridge_index
            controls0[bridge_index], controls1[bridge_index] = _bridge_edge_controls(
                edge_index, bridge_index, positions
            )
        if (
            dvz.dvz_graph_set_edge_controls(
                graph,
                bridge_first_edge,
                bridge_count,
                ctypes.c_void_p(controls0.ctypes.data),
                ctypes.c_void_p(controls1.ctypes.data),
            )
            != 0
        ):
            raise RuntimeError("dvz_graph_set_edge_controls() failed")

    composite = dvz.dvz_graph_composite(graph, 0)
    if not composite:
        raise RuntimeError("dvz_graph_composite() failed")
    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    if dvz.dvz_panel_add_composite(panel, composite, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_composite() failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    positions = _positions()
    _set_equal_view2d(panel, positions)
    _add_graph(scene, panel, positions)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Graph Composite", configure)


if __name__ == "__main__":
    main()
