#!/usr/bin/env python3
"""Classic SVG Tiger rendered from the prepared path bundle."""

from __future__ import annotations

import ctypes
import struct
from pathlib import Path

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex


PATHS = (Path("data/examples/svg_tiger/prepared/tiger_paths.bin"), Path(".cache/datoviz/examples/svg_tiger/prepared/tiger_paths.bin"))
HEADER = struct.Struct("<8sIIIIdd4d")
RECORD = np.dtype(
    [("offset", "<u4"), ("count", "<u4"), ("closed", "u1"), ("fill", "u1"), ("stroke", "u1"), ("reserved", "u1"), ("fill_rgba", "u1", 4), ("stroke_rgba", "u1", 4), ("width", "<f4"), ("order", "<u4")]
)


def _load():
    path = next((candidate for candidate in PATHS if candidate.exists()), None)
    if path is None:
        raise FileNotFoundError("missing SVG Tiger bundle; run `python3 tools/data/prepare_svg_tiger.py --download`")
    payload = path.read_bytes()
    magic, version, path_count, point_count, record_size, width, height, *bounds = HEADER.unpack_from(payload)
    if magic != b"DVZSVG1\0" or version != 2 or record_size != RECORD.itemsize:
        raise ValueError(f"invalid SVG Tiger bundle: {path}")
    offset = HEADER.size
    records = np.frombuffer(payload, RECORD, path_count, offset).copy()
    offset += path_count * RECORD.itemsize
    points = np.frombuffer(payload, "<f8", 2 * point_count, offset).reshape(-1, 2).copy()
    if offset + points.nbytes != len(payload):
        raise ValueError(f"unexpected SVG Tiger bundle size: {path}")
    return records, points, float(width), float(height), bounds


def _polygon_desc(points: np.ndarray):
    points = np.ascontiguousarray(points, dtype=np.float64)
    desc = dvz.dvz_polygon_desc()
    desc.outer.xy = ctypes.c_void_p(points.ctypes.data)
    desc.outer.count = len(points)
    return desc, points


def _set_view(panel, width: float, height: float) -> None:
    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.02
    desc.domain_x[:] = (0.0, width)
    desc.domain_y[:] = (0.0, height)
    desc.has_domain_x = desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def _build_scene():
    records, points, width, height, _bounds = _load()
    scene, figure, panel = ex.scene_panel()
    _set_view(panel, width, height)

    closed = records[(records["closed"] != 0) & (records["count"] >= 3)]
    polygons = dvz.dvz_polygons(scene, 0)
    if not polygons:
        raise RuntimeError("dvz_polygons() failed")
    keepalive = []
    for record in closed:
        ring = points[int(record["offset"]): int(record["offset"] + record["count"])].copy()
        ring[:, 1] = height - ring[:, 1]
        desc, ring = _polygon_desc(ring)
        keepalive.append(ring)
        if dvz.dvz_polygons_add_region(polygons, ctypes.byref(desc)) == 0xFFFFFFFF:
            raise RuntimeError("dvz_polygons_add_region() failed")

    fills = (dvz.DvzColor * len(closed))(*(dvz.DvzColor(*map(int, rgba)) for rgba in closed["fill_rgba"]))
    strokes = (dvz.DvzColor * len(closed))(*(dvz.DvzColor(*map(int, rgba)) for rgba in closed["stroke_rgba"]))
    widths = (ctypes.c_float * len(closed))(*(float(value) for value in closed["width"]))
    dvz.dvz_polygons_set_region_fill_colors(polygons, 0, len(closed), fills)
    dvz.dvz_polygons_set_region_stroke_colors(polygons, 0, len(closed), strokes)
    dvz.dvz_polygons_set_region_stroke_widths_px(polygons, 0, len(closed), widths)
    dvz.dvz_polygons_set_stroke_join(polygons, dvz.DVZ_PATH_JOIN_MITER, 4.0)
    composite = dvz.dvz_polygons_composite(polygons, 0)
    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    if dvz.dvz_panel_add_composite(panel, composite, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_composite() failed")

    open_records = records[(records["closed"] == 0) & (records["stroke"] != 0) & (records["count"] >= 2)]
    if len(open_records):
        positions, colors = [], []
        for record in open_records:
            xy = points[int(record["offset"]): int(record["offset"] + record["count"])].copy()
            xyz = np.column_stack((xy[:, 0], height - xy[:, 1], np.full(len(xy), 0.001 * int(record["order"]))))
            positions.append(np.stack((xyz[:-1], xyz[1:]), axis=1).reshape(-1, 3))
            colors.append(np.repeat(record["stroke_rgba"][None, :], 2 * (len(xy) - 1), axis=0))
        whiskers = dvz.dvz_primitive(scene, dvz.DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0)
        dvz.dvz_visual_set_data_many(whiskers, {"position": np.vstack(positions).astype(np.float32), "color": np.vstack(colors).astype(np.uint8)})
        ex.add_visual(panel, whiskers)
    return scene, figure, panel, len(records), len(points)


def main() -> None:
    scene, figure, panel, path_count, point_count = _build_scene()
    print(f"svg_tiger: {path_count} paths, {point_count} flattened points")

    def configure(view) -> None:
        desc = dvz.dvz_panzoom_desc()
        desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_KEEP_ASPECT
        controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
        if not controller or dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
            raise RuntimeError("panzoom setup failed")

    ex.run_with_view(scene, figure, "SVG Tiger", configure)


if __name__ == "__main__":
    main()
