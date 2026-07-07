"""Shared helpers for Python gallery examples."""

from __future__ import annotations

import ctypes

import datoviz as dvz


WIDTH = 1280
HEIGHT = 720

BG = dvz.DvzColor(13, 18, 25, 255)
TEXT = dvz.DvzColor(217, 226, 236, 255)
CYAN = dvz.DvzColor(34, 211, 238, 255)
BLUE = dvz.DvzColor(96, 165, 250, 255)
GREEN = dvz.DvzColor(74, 222, 128, 255)
YELLOW = dvz.DvzColor(250, 204, 21, 255)
RED = dvz.DvzColor(248, 113, 113, 255)
WHITE = dvz.DvzColor(255, 255, 255, 255)


def color_array(*colors):
    import numpy as np

    return np.array([[c.r, c.g, c.b, c.a] for c in colors], dtype=np.uint8)


def scene_panel(width: int = WIDTH, height: int = HEIGHT):
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, width, height, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, BG)
    return scene, figure, panel


def panel_rect(figure, x: float, y: float, width: float, height: float):
    desc = dvz.dvz_panel_desc()
    desc.x = x
    desc.y = y
    desc.width = width
    desc.height = height
    panel = dvz.dvz_panel(figure, ctypes.byref(desc))
    if not panel:
        raise RuntimeError("dvz_panel() failed")
    dvz.dvz_panel_set_background_color(panel, BG)
    return panel


def run(scene, figure, title: str):
    dvz.run(scene, figure, WIDTH, HEIGHT, title)


def set_filled_point_style(visual):
    style = dvz.dvz_point_style_desc()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
    style.stroke_width_px = 0.0
    if dvz.dvz_point_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_point_set_style() failed")


def set_outline_marker_style(visual, edge_color=TEXT):
    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE
    style.edge_color = edge_color
    style.stroke_width_px = 2.25
    if dvz.dvz_marker_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")


def add_visual(panel, visual):
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError("dvz_panel_add_visual() failed")


def add_points(scene, panel, positions, colors, diameters):
    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    )
    set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    add_visual(panel, point)
    return point
