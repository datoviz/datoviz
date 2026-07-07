#!/usr/bin/env python3
"""Marker symbol rows comparing built-in and image-backed glyphs."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SYMBOL_PIXELS = 96
ROW_COUNT = 5
ROW_SYMBOLS = 5
ROW_LABEL_X = -0.78
INVALID_SYMBOL_ID = 0xFFFFFFFF


def _saturate(x: float) -> float:
    return min(max(x, 0.0), 1.0)


def _unorm8(x: float) -> int:
    return int(255.0 * _saturate(x) + 0.5)


def _encode_sdf(distance: float, distance_range: float) -> int:
    return _unorm8(0.5 - distance / max(distance_range, 1e-6))


def _shape_distance(x: float, y: float, variant: int) -> float:
    variant %= 5
    if variant == 1:
        return max(abs(x) - 0.52, abs(y) - 0.52)
    if variant == 2:
        return abs(x) + abs(y) - 0.72
    if variant == 3:
        return min(
            max(abs(x) - 0.68, abs(y) - 0.15),
            max(abs(x) - 0.15, abs(y) - 0.68),
        )
    if variant == 4:
        radius = math.hypot(x, y)
        return max(radius - 0.66, 0.38 - radius)
    return math.hypot(x, y) - 0.62


def _fill_bitmap_symbol(variant: int) -> np.ndarray:
    pin_colors = (
        (239, 71, 111),
        (255, 183, 3),
        (76, 201, 240),
        (128, 255, 219),
        (201, 209, 217),
    )
    pin = pin_colors[variant % ROW_SYMBOLS]
    rgba = np.zeros((SYMBOL_PIXELS, SYMBOL_PIXELS, 4), dtype=np.uint8)
    for y in range(SYMBOL_PIXELS):
        py = (2.0 * (y + 0.5) / SYMBOL_PIXELS) - 1.0
        for x in range(SYMBOL_PIXELS):
            px = (2.0 * (x + 0.5) / SYMBOL_PIXELS) - 1.0
            edge = 0.030
            head = math.hypot(px, py + 0.28) - 0.44
            tail_width = 0.30 * (0.84 - py) / 0.94
            tail = max(abs(px) - max(tail_width, 0.0), max(-py - 0.10, py - 0.84))
            alpha = max(_saturate(0.5 - head / edge), _saturate(0.5 - tail / edge))
            center = math.hypot(px, py + 0.28) < 0.14
            rgba[y, x, :3] = (255, 255, 255) if center else pin
            rgba[y, x, 3] = _unorm8(alpha)
    return rgba


def _fill_sdf_symbol(variant: int) -> np.ndarray:
    sdf = np.zeros((SYMBOL_PIXELS, SYMBOL_PIXELS), dtype=np.uint8)
    for y in range(SYMBOL_PIXELS):
        py = (2.0 * (y + 0.5) / SYMBOL_PIXELS) - 1.0
        for x in range(SYMBOL_PIXELS):
            px = (2.0 * (x + 0.5) / SYMBOL_PIXELS) - 1.0
            sdf[y, x] = _encode_sdf(_shape_distance(px, py, variant), 0.22)
    return sdf


def _fill_msdf_symbol(variant: int) -> np.ndarray:
    msdf = np.zeros((SYMBOL_PIXELS, SYMBOL_PIXELS, 3), dtype=np.uint8)
    for y in range(SYMBOL_PIXELS):
        py = (2.0 * (y + 0.5) / SYMBOL_PIXELS) - 1.0
        for x in range(SYMBOL_PIXELS):
            px = (2.0 * (x + 0.5) / SYMBOL_PIXELS) - 1.0
            distance = _shape_distance(px, py, variant)
            msdf[y, x, 0] = _encode_sdf(distance - 0.010 * px, 0.22)
            msdf[y, x, 1] = _encode_sdf(distance, 0.22)
            msdf[y, x, 2] = _encode_sdf(distance - 0.010 * py, 0.22)
    return msdf


def _payload_ptr(array: np.ndarray):
    contiguous = np.ascontiguousarray(array)
    return contiguous, ctypes.c_void_p(contiguous.ctypes.data)


def _check_symbol(symbol_id: int) -> int:
    if symbol_id == INVALID_SYMBOL_ID:
        raise RuntimeError("symbol registration failed")
    return int(symbol_id)


def _register_builtin_symbols(symbols):
    builtins = (
        dvz.DVZ_SYMBOL_DISC,
        dvz.DVZ_SYMBOL_TARGET,
        dvz.DVZ_SYMBOL_ARROW,
        dvz.DVZ_SYMBOL_HEART,
        dvz.DVZ_SYMBOL_ROUNDED_RECT,
    )
    return [_check_symbol(dvz.dvz_symbol_builtin(symbols, builtin)) for builtin in builtins]


def _register_bitmap_symbols(symbols):
    ids = []
    for i in range(ROW_SYMBOLS):
        payload, ptr = _payload_ptr(_fill_bitmap_symbol(i))
        ids.append(
            _check_symbol(
                dvz.dvz_symbol_bitmap(
                    symbols, b"bitmap", ptr, SYMBOL_PIXELS, SYMBOL_PIXELS, None
                )
            )
        )
        del payload
    return ids


def _register_sdf_symbols(symbols):
    ids = []
    desc = dvz.dvz_symbol_image_desc()
    desc.distance_range_px = 5.0
    for i in range(ROW_SYMBOLS):
        payload, ptr = _payload_ptr(_fill_sdf_symbol(i))
        ids.append(
            _check_symbol(
                dvz.dvz_symbol_sdf(
                    symbols, b"sdf", ptr, SYMBOL_PIXELS, SYMBOL_PIXELS, ctypes.byref(desc)
                )
            )
        )
        del payload
    return ids


def _register_msdf_symbols(symbols):
    ids = []
    desc = dvz.dvz_symbol_image_desc()
    desc.distance_range_px = 5.0
    for i in range(ROW_SYMBOLS):
        payload, ptr = _payload_ptr(_fill_msdf_symbol(i))
        ids.append(
            _check_symbol(
                dvz.dvz_symbol_msdf(
                    symbols, b"msdf", ptr, SYMBOL_PIXELS, SYMBOL_PIXELS, ctypes.byref(desc)
                )
            )
        )
        del payload
    return ids


def _register_svg_symbols(symbols):
    desc = dvz.dvz_symbol_image_desc()
    desc.distance_range_px = 5.0
    paths = (
        b"M24 4 L29.9 17.5 L44.5 18.9 L33.5 28.6 L36.7 43 L24 35.6 L11.3 43 "
        b"L14.5 28.6 L3.5 18.9 L18.1 17.5 Z",
        b"M24 5 L42 24 L24 43 L6 24 Z",
        b"M24 5 C35 5 43 13 43 24 C43 35 35 43 24 43 C13 43 5 35 5 24 "
        b"C5 13 13 5 24 5 Z M24 15 C19 15 15 19 15 24 C15 29 19 33 24 33 "
        b"C29 33 33 29 33 24 C33 19 29 15 24 15 Z",
        b"M8 22 L20 22 L20 8 L28 8 L28 22 L40 22 L40 30 L28 30 L28 42 "
        b"L20 42 L20 30 L8 30 Z",
        b"M7 14 C7 8 13 5 18 8 L24 14 L30 8 C35 5 41 8 41 14 C41 23 "
        b"31 31 24 40 C17 31 7 23 7 14 Z",
    )
    ids = []
    for path in paths:
        symbol_id = dvz.dvz_symbol_svg_path(
            symbols, b"svg-path", path, SYMBOL_PIXELS, SYMBOL_PIXELS, ctypes.byref(desc)
        )
        if symbol_id == INVALID_SYMBOL_ID:
            return _register_msdf_symbols(symbols)
        ids.append(int(symbol_id))
    return ids


def _add_symbol_row(scene, panel, symbols, ids, row: int, y: float, color):
    visual = dvz.dvz_marker(scene, 0)
    if not visual:
        raise RuntimeError("dvz_marker() failed")
    if dvz.dvz_marker_set_symbols(visual, symbols) != 0:
        raise RuntimeError("dvz_marker_set_symbols() failed")

    style = dvz.dvz_marker_style()
    style.aspect = dvz.DVZ_SHAPE_ASPECT_OUTLINE if row == 0 else dvz.DVZ_SHAPE_ASPECT_FILLED
    style.edge_color = ex.TEXT
    style.edge_color.a = 220
    style.stroke_width_px = 2.0
    if dvz.dvz_marker_set_style(visual, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_marker_set_style() failed")

    positions = np.zeros((ROW_SYMBOLS, 3), dtype=np.float32)
    colors = np.zeros((ROW_SYMBOLS, 4), dtype=np.uint8)
    diameters = np.zeros(ROW_SYMBOLS, dtype=np.float32)
    angles = np.zeros(ROW_SYMBOLS, dtype=np.float32)
    symbol_ids = np.array(ids, dtype=np.uint32)

    for i in range(ROW_SYMBOLS):
        t = i / (ROW_SYMBOLS - 1)
        positions[i] = (-0.46 + 1.24 * t, y, 0.0)
        colors[i] = (color.r, color.g, color.b, 242)
        diameters[i] = 58.0 + 5.0 * ((i + row) % 2)
        angles[i] = 0.18 * (i + row)

    if dvz.dvz_visual_set_data_many(
        visual,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
            "angle": angles,
            "symbol": symbol_ids,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(marker row) failed")
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")
    ex.add_visual(panel, visual)


def _add_row_label(panel, label: bytes, y: float):
    text = dvz.dvz_text(panel, 0)
    if not text:
        raise RuntimeError("dvz_text() failed")

    style = dvz.dvz_text_style()
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.size_px = 20.0
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, ex.TEXT.a)
    if dvz.dvz_text_set_style(text, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_text_set_style() failed")

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_DATA
    placement.position[:] = (ROW_LABEL_X, y, 0.0)
    placement.text_anchor[:] = (0.0, 0.5)
    placement.has_text_anchor = True
    if dvz.dvz_text_set_placement(text, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_text_set_placement() failed")
    if dvz.dvz_text_set_string(text, label) != 0:
        raise RuntimeError("dvz_text_set_string() failed")


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    symbols = dvz.dvz_symbol_set(scene, 0)
    if not symbols:
        raise RuntimeError("dvz_symbol_set() failed")

    row_ids = (
        _register_builtin_symbols(symbols),
        _register_bitmap_symbols(symbols),
        _register_sdf_symbols(symbols),
        _register_msdf_symbols(symbols),
        _register_svg_symbols(symbols),
    )
    row_y = (+0.66, +0.33, 0.0, -0.33, -0.66)
    row_colors = (ex.CYAN, ex.TEXT, ex.GREEN, ex.YELLOW, ex.RED)
    row_labels = (b"built-in", b"bitmap pin", b"SDF", b"MSDF", b"SVG path")

    for row, (ids, y, color, label) in enumerate(zip(row_ids, row_y, row_colors, row_labels)):
        _add_symbol_row(scene, panel, symbols, ids, row, y, color)
        _add_row_label(panel, label, y)

    def configure_view(view) -> None:
        ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)

    ex.run_with_view(scene, figure, "Marker Symbols", configure_view)


if __name__ == "__main__":
    main()
