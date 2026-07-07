#!/usr/bin/env python3
"""Shared sampled field with subregion updates."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_WIDTH = 96
FIELD_HEIGHT = 72
PATCH_SIZE = 12
TAU = 2.0 * np.pi


def _background_values(width: int = FIELD_WIDTH, height: int = FIELD_HEIGHT):
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    values = 0.16 + 0.20 * u + 0.10 * v
    values += 0.12 * np.sin(TAU * (1.8 * u + 0.35 * v))
    values += 0.08 * np.cos(TAU * (0.45 * u - 2.4 * v))
    values += 0.22 * np.exp(-10.0 * ((u - 0.66) ** 2 + (v - 0.35) ** 2))
    return np.clip(values, 0.0, 1.0).astype(np.float32)


def _patch_position(index: int):
    path = (
        (10, 8),
        (42, 8),
        (74, 10),
        (72, 30),
        (58, 52),
        (30, 50),
        (12, 36),
        (24, 20),
    )
    return path[index % len(path)]


def _highlight_patch(phase: float):
    coords = np.linspace(-1.0, 1.0, PATCH_SIZE, dtype=np.float32)
    ux, vy = np.meshgrid(coords, coords)
    pulse = 0.82 + 0.12 * np.sin(phase)
    patch = 0.42 + pulse * np.exp(-2.4 * (ux * ux + vy * vy))
    return np.clip(patch, 0.0, 1.0).astype(np.float32)


def _scale(scene, name: bytes, colors):
    color_array = (dvz.DvzColor * len(colors))(*colors)
    colormap = dvz.dvz_colormap_custom(scene, name, color_array, len(colors))
    if not colormap:
        raise RuntimeError("dvz_colormap_custom() failed")

    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    dvz.dvz_scale_set_domain(scale, 0.0, 1.0)
    dvz.dvz_scale_set_colormap(scale, colormap)
    return scale


def main() -> None:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    grid = dvz.dvz_figure_grid(figure, 1, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")
    if dvz.dvz_grid_set_gutter(grid, 34.0, 0.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    panels = [dvz.dvz_grid_panel(grid, 0, 0), dvz.dvz_grid_panel(grid, 0, 1)]
    if not panels[0] or not panels[1]:
        raise RuntimeError("dvz_grid_panel() failed")
    for panel in panels:
        dvz.dvz_panel_set_background_color(panel, ex.BG)

    values = _background_values()
    x0, y0 = _patch_position(0)
    values[y0 : y0 + PATCH_SIZE, x0 : x0 + PATCH_SIZE] = _highlight_patch(0.0)

    field = dvz.dvz_sampled_field_from_array(scene, values)
    restore = _background_values()[y0 : y0 + PATCH_SIZE, x0 : x0 + PATCH_SIZE]
    dvz.dvz_sampled_field_update_from_array(field, restore, offset=(x0, y0))
    x1, y1 = _patch_position(3)
    dvz.dvz_sampled_field_update_from_array(field, _highlight_patch(1.2), offset=(x1, y1))

    left_scale = _scale(
        scene,
        b"python_sampled_update_cyan",
        [
            dvz.DvzColor(14, 17, 23, 255),
            dvz.DvzColor(25, 79, 118, 255),
            ex.CYAN,
            ex.GREEN,
            ex.YELLOW,
        ],
    )
    right_scale = _scale(
        scene,
        b"python_sampled_update_warm",
        [
            dvz.DvzColor(14, 17, 23, 255),
            dvz.DvzColor(49, 60, 91, 255),
            dvz.DvzColor(128, 91, 153, 255),
            dvz.DvzColor(229, 117, 94, 255),
            dvz.DvzColor(255, 214, 102, 255),
        ],
    )

    ex.add_image(scene, panels[0], field, scale=left_scale)
    ex.add_image(scene, panels[1], field, scale=right_scale)

    ex.run(scene, figure, "Sampled Field Update")


if __name__ == "__main__":
    main()
