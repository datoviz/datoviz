#!/usr/bin/env python3
"""Direct-engine Python offscreen point rendering example."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

import datoviz as dvz


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='optional .npy RGBA output path')
    args = parser.parse_args(argv)

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    app = None
    try:
        figure = dvz.dvz_figure(scene, 128, 128, 0)
        if not figure:
            raise RuntimeError('dvz_figure() failed')
        panel = dvz.dvz_panel_full(figure)
        if not panel:
            raise RuntimeError('dvz_panel_full() failed')
        dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(13, 15, 20, 255))

        points = dvz.dvz_point(scene, 0)
        if not points:
            raise RuntimeError('dvz_point() failed')

        positions = np.array(
            [[-0.55, -0.45, 0.0], [+0.55, -0.45, 0.0], [0.0, +0.50, 0.0]],
            dtype=np.float32,
        )
        colors = np.array(
            [[255, 80, 80, 255], [80, 220, 120, 255], [90, 150, 255, 255]],
            dtype=np.uint8,
        )
        diameters = np.full(3, 18.0, dtype=np.float32)

        if dvz.dvz_visual_set_data_many(
            points,
            {
                'position': positions,
                'color': colors,
                'diameter_px': diameters,
            },
        ) != 0:
            raise RuntimeError('dvz_visual_set_data_many() failed')
        if dvz.dvz_panel_add_visual(panel, points, None) != 0:
            raise RuntimeError('dvz_panel_add_visual() failed')

        app = dvz.dvz_app(scene)
        if not app:
            print('direct offscreen point: SKIP (dvz_app() failed)')
            return 0
        view = dvz.dvz_view_offscreen(app, figure, 128, 128)
        if not view:
            print('direct offscreen point: SKIP (dvz_view_offscreen() failed)')
            return 0

        if dvz.dvz_view_render_once(view) != 0:
            raise RuntimeError('dvz_view_render_once() failed')
        rgba = dvz.dvz_view_capture_rgba(view)
        if rgba.shape != (128, 128, 4) or rgba.dtype != np.uint8:
            raise RuntimeError(f'unexpected capture shape/dtype: {rgba.shape} {rgba.dtype}')
        if not np.any(rgba[:, :, :3] != np.array([13, 15, 20], dtype=np.uint8)):
            raise RuntimeError('capture did not contain non-background pixels')

        if args.output is not None:
            np.save(args.output, rgba)
        print(f'direct offscreen point: OK ({rgba.shape[1]}x{rgba.shape[0]} RGBA)')
        return 0
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == '__main__':
    raise SystemExit(main())
