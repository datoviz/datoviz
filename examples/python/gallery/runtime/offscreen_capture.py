#!/usr/bin/env python3
"""Render a retained point scene offscreen and optionally write a PNG."""

from __future__ import annotations

import argparse
import ctypes
import os
from pathlib import Path

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


WIDTH = 1920
HEIGHT = 1080
POINT_COUNT = 4


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [-0.55, -0.35, 0.0],
            [-0.18, +0.35, 0.0],
            [+0.18, -0.20, 0.0],
            [+0.55, +0.25, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.CYAN, ex.BLUE, ex.YELLOW, ex.TEXT)
    diameters = np.array([38.0, 54.0, 44.0, 62.0], dtype=np.float32)

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
    ex.add_visual(panel, point)


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, ex.BG)
    _add_points(scene, panel)
    return scene, figure


def _framebuffer_size(view) -> tuple[int, int]:
    width = ctypes.c_uint32()
    height = ctypes.c_uint32()
    dvz.dvz_view_framebuffer_size(view, ctypes.byref(width), ctypes.byref(height))
    return int(width.value), int(height.value)


def _render_capture(output: Path | None = None):
    scene, figure = _build_scene()
    app = None
    try:
        app = dvz.dvz_app(scene)
        if not app:
            raise RuntimeError("dvz_app() failed")
        view = dvz.dvz_view_offscreen(app, figure, WIDTH, HEIGHT)
        if not view:
            raise RuntimeError("dvz_view_offscreen() failed")

        framebuffer_width, framebuffer_height = _framebuffer_size(view)
        if (framebuffer_width, framebuffer_height) != (WIDTH, HEIGHT):
            raise RuntimeError(
                f"unexpected framebuffer size: {framebuffer_width}x{framebuffer_height}"
            )

        if dvz.dvz_view_render_once(view) != dvz.DVZ_CANVAS_FRAME_READY:
            raise RuntimeError("dvz_view_render_once() failed")
        rgba = dvz.dvz_view_capture_rgba(view)
        if ex.SMOKE_MODE and not np.any(rgba[..., :3] != rgba[0, 0, :3]):
            raise RuntimeError("offscreen capture smoke is blank")

        if output is not None:
            output.parent.mkdir(parents=True, exist_ok=True)
            if dvz.dvz_view_capture_png(view, str(output).encode()) != 0:
                raise RuntimeError("dvz_view_capture_png() failed")
        return rgba
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    smoke_capture = os.environ.get("DVZ_PYTHON_GALLERY_CAPTURE", "") if ex.SMOKE_MODE else ""
    default_output = Path(smoke_capture) if smoke_capture else None
    parser.add_argument("--output", type=Path, default=default_output)
    args = parser.parse_args(argv)

    rgba = _render_capture(args.output)
    action = f"wrote {args.output}" if args.output is not None else "rendered without writing"
    print(f"offscreen_capture: {action} ({rgba.shape[1]}x{rgba.shape[0]} exact pixels)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
