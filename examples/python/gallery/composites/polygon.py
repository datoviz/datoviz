#!/usr/bin/env python3
"""Semantic polygon composite with a hole and styled polygon set."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def _polygon_desc(points: np.ndarray) -> tuple[dvz.DvzPolygonDesc, np.ndarray]:
    points = np.ascontiguousarray(points, dtype=np.float64)
    desc = dvz.dvz_polygon_desc()
    desc.outer.xy = ctypes.c_void_p(points.ctypes.data)
    desc.outer.count = len(points)
    return desc, points


def _set_equal_view2d(panel) -> None:
    padding = dvz.DvzPanelReserve()
    padding.left_px = 24.0
    padding.right_px = 24.0
    padding.top_px = 18.0
    padding.bottom_px = 18.0
    if dvz.dvz_panel_set_padding(panel, ctypes.byref(padding)) != 0:
        raise RuntimeError("dvz_panel_set_padding() failed")

    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.05
    desc.domain_x[:] = (-2.18, +3.06)
    desc.domain_y[:] = (-0.88, +0.88)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def _attach_composite(panel, composite, *, z_layer: int = 0) -> None:
    if not composite:
        raise RuntimeError("composite creation failed")
    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    attach.z_layer = z_layer
    if dvz.dvz_panel_add_composite(panel, composite, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_composite() failed")


def _add_holed_polygon(scene, panel) -> None:
    outer = np.array(
        [
            [-2.12, +0.00],
            [-1.88, -0.58],
            [-1.30, -0.82],
            [-0.72, -0.58],
            [-0.48, +0.00],
            [-0.72, +0.58],
            [-1.30, +0.82],
            [-1.88, +0.58],
        ],
        dtype=np.float64,
    )
    hole = np.array(
        [
            [-1.66, +0.00],
            [-1.52, -0.26],
            [-1.22, -0.26],
            [-1.08, +0.00],
            [-1.22, +0.26],
            [-1.52, +0.26],
        ],
        dtype=np.float64,
    )

    polygon = dvz.dvz_polygon(scene, 0)
    if not polygon:
        raise RuntimeError("dvz_polygon() failed")

    desc, outer = _polygon_desc(outer)
    if dvz.dvz_polygon_set_geometry(polygon, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_polygon_set_geometry() failed")
    if dvz.dvz_polygon_set_hole(polygon, 0, ctypes.c_void_p(hole.ctypes.data), len(hole)) != 0:
        raise RuntimeError("dvz_polygon_set_hole() failed")
    if dvz.dvz_polygon_set_id(polygon, 10) != 0:
        raise RuntimeError("dvz_polygon_set_id() failed")
    if dvz.dvz_polygon_set_fill_color(polygon, dvz.DvzColor(36, 151, 178, 210)) != 0:
        raise RuntimeError("dvz_polygon_set_fill_color() failed")
    if dvz.dvz_polygon_set_stroke_color(polygon, dvz.DvzColor(214, 240, 255, 255)) != 0:
        raise RuntimeError("dvz_polygon_set_stroke_color() failed")
    if dvz.dvz_polygon_set_stroke_width_px(polygon, 8.0) != 0:
        raise RuntimeError("dvz_polygon_set_stroke_width_px() failed")
    if dvz.dvz_polygon_set_stroke_join(polygon, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_polygon_set_stroke_join() failed")

    _attach_composite(panel, dvz.dvz_polygon_composite(polygon, 0))


def _add_polygon_set(scene, panel) -> None:
    regions = [
        np.array(
            [
                [+0.30, +0.74],
                [+0.49, +0.24],
                [+1.03, +0.24],
                [+0.60, -0.06],
                [+0.77, -0.58],
                [+0.30, -0.26],
                [-0.17, -0.58],
                [+0.00, -0.06],
                [-0.43, +0.24],
                [+0.11, +0.24],
            ],
            dtype=np.float64,
        ),
        np.array(
            [[+1.12, -0.82], [+1.92, -0.82], [+1.92, -0.10], [+1.12, -0.10]],
            dtype=np.float64,
        ),
        np.array(
            [[+2.20, +0.10], [+3.00, +0.10], [+3.00, +0.82], [+2.20, +0.82]],
            dtype=np.float64,
        ),
    ]

    polygons = dvz.dvz_polygons(scene, 0)
    if not polygons:
        raise RuntimeError("dvz_polygons() failed")

    for region in regions:
        desc, _region = _polygon_desc(region)
        if dvz.dvz_polygons_add_region(polygons, ctypes.byref(desc)) == 0xFFFFFFFF:
            raise RuntimeError("dvz_polygons_add_region() failed")

    ids = (ctypes.c_uint64 * 3)(21, 22, 23)
    if dvz.dvz_polygons_set_region_ids(polygons, 0, 3, ids) != 0:
        raise RuntimeError("dvz_polygons_set_region_ids() failed")

    fills = (dvz.DvzColor * 3)(
        dvz.DvzColor(231, 98, 82, 220),
        dvz.DvzColor(240, 189, 72, 220),
        dvz.DvzColor(92, 189, 132, 220),
    )
    strokes = (dvz.DvzColor * 3)(
        dvz.DvzColor(85, 42, 38, 255),
        dvz.DvzColor(88, 68, 26, 255),
        dvz.DvzColor(26, 74, 54, 255),
    )
    widths = (ctypes.c_float * 3)(5.0, 7.0, 5.0)

    if dvz.dvz_polygons_set_region_fill_colors(polygons, 0, 3, fills) != 0:
        raise RuntimeError("dvz_polygons_set_region_fill_colors() failed")
    if dvz.dvz_polygons_set_region_stroke_colors(polygons, 0, 3, strokes) != 0:
        raise RuntimeError("dvz_polygons_set_region_stroke_colors() failed")
    if dvz.dvz_polygons_set_region_stroke_widths_px(polygons, 0, 3, widths) != 0:
        raise RuntimeError("dvz_polygons_set_region_stroke_widths_px() failed")
    if dvz.dvz_polygons_set_stroke_join(polygons, dvz.DVZ_PATH_JOIN_BEVEL, 4.0) != 0:
        raise RuntimeError("dvz_polygons_set_stroke_join() failed")

    _attach_composite(panel, dvz.dvz_polygons_composite(polygons, 0), z_layer=2)


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    _set_equal_view2d(panel)
    _add_holed_polygon(scene, panel)
    _add_polygon_set(scene, panel)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    desc = dvz.dvz_panzoom_desc()
    desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_KEEP_ASPECT
    controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Polygon Composite", configure)


if __name__ == "__main__":
    main()
