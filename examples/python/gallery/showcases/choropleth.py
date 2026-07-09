#!/usr/bin/env python3
"""U.S. state choropleth from prepared Census polygon-set arrays."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass
from pathlib import Path

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


DATA_BUNDLE = Path("data/examples/us_state_choropleth/prepared")
CACHE_BUNDLE = Path(".cache/datoviz/examples/us_state_choropleth/prepared")

METADATA_NAME = "metadata.tsv"
POINTS_NAME = "points_xy_f64.bin"
RINGS_NAME = "rings_u32.bin"
RING_FILL_NAME = "ring_fill_rgba8.bin"
RING_STROKE_NAME = "ring_stroke_rgba8.bin"
RING_WIDTH_NAME = "ring_width_f32.bin"
RING_ID_NAME = "ring_id_u64.bin"

CHOROPLETH_RAMP = (
    dvz.DvzColor(26, 35, 46, 235),
    dvz.DvzColor(33, 99, 126, 235),
    dvz.DvzColor(50, 160, 147, 235),
    dvz.DvzColor(237, 191, 94, 235),
    dvz.DvzColor(221, 96, 73, 235),
)


@dataclass
class ChoroplethBundle:
    path: Path
    region_count: int
    ring_count: int
    point_count: int
    xmin: float
    xmax: float
    ymin: float
    ymax: float
    value_min: float
    value_max: float
    density_min: float
    density_max: float
    rings: np.ndarray
    points: np.ndarray
    fill: np.ndarray
    stroke: np.ndarray
    widths: np.ndarray
    ids: np.ndarray


def _default_bundle_path() -> Path:
    for path in (DATA_BUNDLE, CACHE_BUNDLE):
        if (path / METADATA_NAME).is_file():
            return path
    raise RuntimeError(
        "choropleth: missing prepared bundle\n"
        "  python tools/data/prepare_us_state_choropleth.py"
    )


def _load_metadata(path: Path) -> dict[str, str]:
    metadata: dict[str, str] = {}
    with (path / METADATA_NAME).open("r", encoding="utf8") as f:
        for line in f:
            key, value = line.rstrip("\n").split("\t", 1)
            metadata[key] = value
    return metadata


def _metadata_u32(metadata: dict[str, str], key: str) -> int:
    value = int(metadata[key])
    if value <= 0 or value > np.iinfo(np.uint32).max:
        raise RuntimeError(f"choropleth: invalid metadata field {key}")
    return value


def _metadata_f64(metadata: dict[str, str], key: str) -> float:
    return float(metadata[key])


def _read_array(path: Path, name: str, dtype, count: int, shape: tuple[int, ...]) -> np.ndarray:
    array = np.fromfile(path / name, dtype=dtype)
    expected = int(np.prod(shape))
    if array.size != expected:
        raise RuntimeError(
            f"choropleth: {name} has {array.size} items, expected {expected}"
        )
    return np.ascontiguousarray(array.reshape(shape))


def _validate_rings(bundle: ChoroplethBundle) -> None:
    rings = bundle.rings
    if np.any(rings[:, 0] >= bundle.region_count):
        raise RuntimeError("choropleth: ring region index out of range")
    if np.any(rings[:, 2] < 3):
        raise RuntimeError("choropleth: ring with fewer than three points")
    ends = rings[:, 1].astype(np.uint64) + rings[:, 2].astype(np.uint64)
    if np.any(ends > bundle.point_count):
        raise RuntimeError("choropleth: ring point span out of range")


def _load_bundle(path: Path | None = None) -> ChoroplethBundle:
    path = _default_bundle_path() if path is None else path
    metadata = _load_metadata(path)

    region_count = _metadata_u32(metadata, "region_count")
    ring_count = _metadata_u32(metadata, "ring_count")
    point_count = _metadata_u32(metadata, "point_count")
    xmin = _metadata_f64(metadata, "xmin")
    xmax = _metadata_f64(metadata, "xmax")
    ymin = _metadata_f64(metadata, "ymin")
    ymax = _metadata_f64(metadata, "ymax")
    value_min = _metadata_f64(metadata, "value_min")
    value_max = _metadata_f64(metadata, "value_max")
    density_min = _metadata_f64(metadata, "density_min")
    density_max = _metadata_f64(metadata, "density_max")
    if not (xmin < xmax and ymin < ymax and value_min < value_max):
        raise RuntimeError("choropleth: invalid metadata domains")

    bundle = ChoroplethBundle(
        path=path,
        region_count=region_count,
        ring_count=ring_count,
        point_count=point_count,
        xmin=xmin,
        xmax=xmax,
        ymin=ymin,
        ymax=ymax,
        value_min=value_min,
        value_max=value_max,
        density_min=density_min,
        density_max=density_max,
        rings=_read_array(path, RINGS_NAME, np.uint32, ring_count, (ring_count, 3)),
        points=_read_array(path, POINTS_NAME, np.float64, point_count, (point_count, 2)),
        fill=_read_array(path, RING_FILL_NAME, np.uint8, ring_count, (ring_count, 4)),
        stroke=_read_array(path, RING_STROKE_NAME, np.uint8, ring_count, (ring_count, 4)),
        widths=_read_array(path, RING_WIDTH_NAME, np.float32, ring_count, (ring_count,)),
        ids=_read_array(path, RING_ID_NAME, np.uint64, ring_count, (ring_count,)),
    )
    _validate_rings(bundle)
    return bundle


def _configure_panel(panel, bundle: ChoroplethBundle) -> None:
    padding = dvz.DvzPanelReserve()
    padding.left_px = 14.0
    padding.right_px = 42.0
    padding.bottom_px = 13.5
    padding.top_px = 28.5
    if dvz.dvz_panel_set_padding(panel, ctypes.byref(padding)) != 0:
        raise RuntimeError("dvz_panel_set_padding() failed")

    desc = dvz.dvz_panel_view2d_desc()
    desc.mode = dvz.DVZ_PANEL_VIEW2D_CONTAIN
    desc.aspect = dvz.DVZ_PANEL_VIEW2D_ASPECT_EQUAL
    desc.padding = 0.035
    desc.domain_x[:] = (bundle.xmin, bundle.xmax)
    desc.domain_y[:] = (bundle.ymin, bundle.ymax)
    desc.has_domain_x = True
    desc.has_domain_y = True
    if dvz.dvz_panel_set_view2d(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_view2d() failed")


def _add_screen_text(panel, text: bytes, x: float, y: float, size: float) -> None:
    label = dvz.dvz_text(panel, 0)
    if not label:
        raise RuntimeError("dvz_text() failed")

    style = dvz.dvz_text_style()
    style.size_px = size
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, ex.TEXT.a)
    if dvz.dvz_text_set_style(label, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (x, y, 0.0)
    placement.text_anchor[:] = (0.0, 0.5)
    placement.has_text_anchor = True
    placement.depth_test = False
    if dvz.dvz_text_set_placement(label, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")
    if dvz.dvz_text_set_string(label, text) != 0:
        raise RuntimeError("dvz_text_set_string() failed")


def _add_scale(scene, bundle: ChoroplethBundle):
    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    desc.label = b"log10 people/km2"
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")

    fmt = dvz.dvz_format_desc()
    fmt.precision = 2
    fmt.trim_trailing_zeros = True
    if dvz.dvz_scale_set_format(scale, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_scale_set_format() failed")
    if dvz.dvz_scale_set_domain(scale, bundle.value_min, bundle.value_max) != 0:
        raise RuntimeError("dvz_scale_set_domain() failed")
    if dvz.dvz_scale_set_view_range(scale, bundle.value_min, bundle.value_max) != 0:
        raise RuntimeError("dvz_scale_set_view_range() failed")

    ramp = (dvz.DvzColor * len(CHOROPLETH_RAMP))(*CHOROPLETH_RAMP)
    colormap = dvz.dvz_colormap_custom(scene, b"us_state_density", ramp, len(ramp))
    if not colormap:
        raise RuntimeError("dvz_colormap_custom() failed")
    if dvz.dvz_scale_set_colormap(scale, colormap) != 0:
        raise RuntimeError("dvz_scale_set_colormap() failed")
    return scale


def _add_choropleth_polygons(scene, panel, bundle: ChoroplethBundle) -> None:
    polygons = dvz.dvz_polygons(scene, 0)
    if not polygons:
        raise RuntimeError("dvz_polygons() failed")

    for i, (_region_index, point_first, point_count) in enumerate(bundle.rings):
        desc = dvz.dvz_polygon_desc()
        points = bundle.points[point_first : point_first + point_count]
        desc.outer.xy = ctypes.c_void_p(points.ctypes.data)
        desc.outer.count = int(point_count)
        index = dvz.dvz_polygons_add_region(polygons, ctypes.byref(desc))
        if index == 0xFFFFFFFF or index != i:
            raise RuntimeError("dvz_polygons_add_region() failed")

    ids = bundle.ids.ctypes.data_as(ctypes.POINTER(ctypes.c_uint64))
    fill = bundle.fill.ctypes.data_as(ctypes.POINTER(dvz.DvzColor))
    stroke = bundle.stroke.ctypes.data_as(ctypes.POINTER(dvz.DvzColor))
    widths = bundle.widths.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

    if dvz.dvz_polygons_set_region_ids(polygons, 0, bundle.ring_count, ids) != 0:
        raise RuntimeError("dvz_polygons_set_region_ids() failed")
    if dvz.dvz_polygons_set_region_fill_colors(polygons, 0, bundle.ring_count, fill) != 0:
        raise RuntimeError("dvz_polygons_set_region_fill_colors() failed")
    if dvz.dvz_polygons_set_region_stroke_colors(polygons, 0, bundle.ring_count, stroke) != 0:
        raise RuntimeError("dvz_polygons_set_region_stroke_colors() failed")
    if dvz.dvz_polygons_set_region_stroke_widths_px(polygons, 0, bundle.ring_count, widths) != 0:
        raise RuntimeError("dvz_polygons_set_region_stroke_widths_px() failed")
    if dvz.dvz_polygons_set_stroke_join(polygons, dvz.DVZ_PATH_JOIN_ROUND, 3.0) != 0:
        raise RuntimeError("dvz_polygons_set_stroke_join() failed")

    attach = dvz.dvz_visual_attach_desc()
    attach.coord_space = dvz.DVZ_VISUAL_COORD_DATA
    attach.z_layer = 0
    composite = dvz.dvz_polygons_composite(polygons, 0)
    if not composite:
        raise RuntimeError("dvz_polygons_composite() failed")
    if dvz.dvz_panel_add_composite(panel, composite, ctypes.byref(attach)) != 0:
        raise RuntimeError("dvz_panel_add_composite() failed")


def _add_annotations(panel, scale) -> None:
    _add_screen_text(panel, b"Contiguous U.S. state population density", 32.0, 38.0, 34.0)
    _add_screen_text(
        panel,
        b"Census 2024 boundaries + Vintage 2025 population estimates",
        32.0,
        76.0,
        20.0,
    )

    desc = dvz.dvz_colorbar_desc()
    desc.orientation = dvz.DVZ_COLORBAR_ORIENTATION_VERTICAL
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_RIGHT
    desc.title = b"log10 people/km2"
    desc.reserve_px = 120.0
    desc.ramp_width_px = 28.0
    desc.plot_gap_px = 14.0
    desc.tick_length_px = 6.0
    desc.label_gap_px = 7.0
    colorbar = dvz.dvz_colorbar(panel, scale, ctypes.byref(desc))
    if not colorbar:
        raise RuntimeError("dvz_colorbar() failed")

    fmt = dvz.dvz_format_desc()
    fmt.precision = 2
    fmt.trim_trailing_zeros = True
    if dvz.dvz_colorbar_set_format(colorbar, ctypes.byref(fmt)) != 0:
        raise RuntimeError("dvz_colorbar_set_format() failed")


def _build_scene(path: Path | None = None):
    bundle = _load_bundle(path)
    scene, figure, panel = ex.scene_panel()
    _configure_panel(panel, bundle)
    scale = _add_scale(scene, bundle)
    _add_choropleth_polygons(scene, panel, bundle)
    _add_annotations(panel, scale)
    return scene, figure, panel, bundle


def _configure_view(view, scene, panel) -> None:
    desc = dvz.dvz_panzoom_desc()
    desc.controller_flags = dvz.DVZ_PANZOOM_FLAGS_KEEP_ASPECT
    controller = dvz.dvz_panzoom(scene, ctypes.byref(desc))
    if not controller:
        raise RuntimeError("dvz_panzoom() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")


def main() -> None:
    scene, figure, panel, bundle = _build_scene()
    print(
        "choropleth:"
        f" {bundle.region_count} regions,"
        f" {bundle.ring_count} rings,"
        f" {bundle.point_count} points from {bundle.path}"
    )

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "U.S. State Choropleth", configure)


if __name__ == "__main__":
    main()
