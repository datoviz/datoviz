#!/usr/bin/env python3
"""Retained text annotation anchored to one highlighted data point."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 96
READOUT_INDEX = 61


def _point_data():
    t = np.linspace(0.0, 1.0, POINT_COUNT, dtype=np.float32)
    x = 10.0 * t
    y = 0.45 * np.sin(2.0 * np.pi * 1.7 * t)
    y += 0.22 * np.cos(2.0 * np.pi * 4.0 * t + 0.2)

    positions = np.column_stack((x, y, np.zeros(POINT_COUNT, dtype=np.float32))).astype(np.float32)
    colors = np.tile(
        np.array([[ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 190]], dtype=np.uint8), (POINT_COUNT, 1)
    )
    diameters = np.full(POINT_COUNT, 8.0, dtype=np.float32)

    colors[READOUT_INDEX] = (ex.YELLOW.r, ex.YELLOW.g, ex.YELLOW.b, 255)
    diameters[READOUT_INDEX] = 16.0
    return positions, colors, diameters


def _add_points(scene, panel, positions, colors, diameters) -> None:
    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.set_filled_point_style(point)
    ex.add_visual(panel, point)


def _add_readout(panel, position) -> None:
    style = dvz.dvz_text_style()
    style.size_px = 24.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, ex.TEXT.a)

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_DATA
    placement.position[:] = (float(position[0]), float(position[1]), float(position[2]))
    placement.offset[:] = (28.0, -24.0)
    placement.text_anchor[:] = (0.0, 0.5)
    placement.has_text_anchor = True
    placement.depth_test = False

    desc = dvz.dvz_label_desc()
    desc.text = f"peak  x {position[0]:.2f}  y {position[1]:.2f}".encode()
    annotation = dvz.dvz_annotation_label(panel, ctypes.byref(desc))
    if not annotation:
        raise RuntimeError("dvz_annotation_label() failed")
    if dvz.dvz_annotation_set_style(annotation, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_annotation_set_style() failed")
    if dvz.dvz_annotation_set_placement(annotation, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_annotation_set_placement() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 10.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, -1.0, 1.0) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")

    positions, colors, diameters = _point_data()
    _add_points(scene, panel, positions, colors, diameters)
    _add_readout(panel, positions[READOUT_INDEX])

    ex.run(scene, figure, "Annotation Readout")


if __name__ == "__main__":
    main()
