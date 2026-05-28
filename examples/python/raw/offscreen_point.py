#!/usr/bin/env python3
"""Raw ctypes offscreen point rendering example."""

from __future__ import annotations

import argparse
import ctypes
import tempfile
from pathlib import Path

import datoviz.raw as dvz


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='PNG output path')
    args = parser.parse_args(argv)

    output = args.output
    tempdir = None
    if output is None:
        tempdir = tempfile.TemporaryDirectory(prefix='datoviz-ctypes-render-')
        output = Path(tempdir.name) / 'raw_offscreen_point.png'

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
        dvz.dvz_panel_set_background_color(panel, 0.05, 0.06, 0.08, 1.0)

        visual = dvz.dvz_point(scene, 0)
        if not visual:
            raise RuntimeError('dvz_point() failed')

        positions = (ctypes.c_float * 9)(
            -0.55,
            -0.45,
            0.0,
            +0.55,
            -0.45,
            0.0,
            0.0,
            +0.50,
            0.0,
        )
        colors = (dvz.DvzColor * 3)(
            dvz.DvzColor(255, 80, 80, 255),
            dvz.DvzColor(80, 220, 120, 255),
            dvz.DvzColor(90, 150, 255, 255),
        )
        diameters = (ctypes.c_float * 3)(18.0, 18.0, 18.0)

        if dvz.dvz_visual_set_data(visual, b'position', _void_p(positions), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(position) failed')
        if dvz.dvz_visual_set_data(visual, b'color', _void_p(colors), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(color) failed')
        if dvz.dvz_visual_set_data(visual, b'diameter', _void_p(diameters), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(diameter) failed')
        if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
            raise RuntimeError('dvz_panel_add_visual() failed')

        app = dvz.dvz_app(scene)
        if not app:
            print('raw offscreen point: SKIP (dvz_app() failed)')
            return 0
        view = dvz.dvz_view_offscreen(app, figure, 128, 128)
        if not view:
            print('raw offscreen point: SKIP (dvz_view_offscreen() failed)')
            return 0

        if dvz.dvz_app_render_once(app) != 0:
            raise RuntimeError('dvz_app_render_once() failed')
        if dvz.dvz_view_capture_png(view, str(output).encode()) != 0:
            raise RuntimeError('dvz_view_capture_png() failed')
        if not output.exists() or output.stat().st_size == 0:
            raise RuntimeError('PNG capture was not written')

        print(f'raw offscreen point: OK ({output})')
        return 0
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
        if tempdir is not None:
            tempdir.cleanup()


if __name__ == '__main__':
    raise SystemExit(main())
