#!/usr/bin/env python3
"""Raw ctypes asyncio click-handler example."""

from __future__ import annotations

import argparse
import asyncio
import ctypes
import tempfile
from pathlib import Path

import datoviz as dvz


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def _diameter_from_click(x: float) -> float:
    return 16.0 + abs(float(x)) * 0.05


async def _run(output: Path) -> int:
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    app = None
    wrapped_view = None
    try:
        figure = dvz.dvz_figure(scene, 128, 128, 0)
        if not figure:
            raise RuntimeError('dvz_figure() failed')
        panel = dvz.dvz_panel_full(figure)
        if not panel:
            raise RuntimeError('dvz_panel_full() failed')
        dvz.dvz_panel_set_background_color(panel, 0.04, 0.05, 0.07, 1.0)

        visual = dvz.dvz_point(scene, 0)
        if not visual:
            raise RuntimeError('dvz_point() failed')

        positions = (ctypes.c_float * 9)(
            -0.5,
            -0.35,
            0.0,
            +0.5,
            -0.35,
            0.0,
            0.0,
            +0.45,
            0.0,
        )
        colors = (dvz.DvzColor * 3)(
            dvz.DvzColor(255, 96, 96, 255),
            dvz.DvzColor(80, 220, 150, 255),
            dvz.DvzColor(96, 160, 255, 255),
        )
        diameters = (ctypes.c_float * 3)(16.0, 16.0, 16.0)

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
            print('raw async click: SKIP (dvz_app() failed)')
            return 0
        raw_view = dvz.dvz_view_offscreen(app, figure, 128, 128)
        if not raw_view:
            print('raw async click: SKIP (dvz_view_offscreen() failed)')
            return 0

        wrapped_view = dvz.View(raw_view)
        wrapped_view.bind_loop()
        clicked = asyncio.Event()

        @wrapped_view.on('click')
        async def click(ev):
            diameter = await dvz.run_thread(_diameter_from_click, ev.x)
            diameters[0] = diameter
            if dvz.dvz_visual_set_data(visual, b'diameter', _void_p(diameters), 3) != 0:
                raise RuntimeError('dvz_visual_set_data(diameter) failed')
            wrapped_view.request_frame()
            clicked.set()

        app_loop = dvz.AppLoop(app, views=[raw_view], fps=30.0)
        task = asyncio.create_task(app_loop.run_async())
        try:
            await asyncio.sleep(0)
            router = dvz.dvz_view_input(raw_view)
            if not router:
                raise RuntimeError('dvz_view_input() failed')
            dvz.dvz_pointer_emit_position(
                router,
                dvz.DvzPointerEventType.DVZ_POINTER_EVENT_CLICK,
                32.0,
                64.0,
                128.0,
                128.0,
                dvz.DvzPointerButton.DVZ_POINTER_BUTTON_LEFT,
                0,
                1.0,
                1,
                None,
            )
            await asyncio.wait_for(clicked.wait(), timeout=2.0)
            app_loop.stop()
            await task
        finally:
            app_loop.stop()
            if not task.done():
                await task

        if dvz.dvz_app_render_once(app) != 0:
            raise RuntimeError('dvz_app_render_once() failed')
        if dvz.dvz_view_capture_png(raw_view, str(output).encode()) != 0:
            raise RuntimeError('dvz_view_capture_png() failed')
        if not output.exists() or output.stat().st_size == 0:
            raise RuntimeError('PNG capture was not written')

        print(f'raw async click: OK ({output})')
        return 0
    finally:
        if wrapped_view is not None:
            wrapped_view.close()
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='PNG output path')
    args = parser.parse_args(argv)

    output = args.output
    tempdir = None
    if output is None:
        tempdir = tempfile.TemporaryDirectory(prefix='datoviz-ctypes-async-')
        output = Path(tempdir.name) / 'raw_async_click.png'

    try:
        return asyncio.run(_run(output))
    finally:
        if tempdir is not None:
            tempdir.cleanup()


if __name__ == '__main__':
    raise SystemExit(main())
