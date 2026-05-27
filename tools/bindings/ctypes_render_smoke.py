#!/usr/bin/env python3
"""Smoke-test a tiny raw ctypes offscreen render."""

from __future__ import annotations

import ctypes
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]


def _run_view_post_smoke() -> None:
    import datoviz as dvz  # noqa: PLC0415

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    app = None
    try:
        figure = dvz.dvz_figure(scene, 32, 32, 0)
        if not figure:
            raise RuntimeError('dvz_figure() failed')
        app = dvz.dvz_app(scene)
        if not app:
            print('raw view post: SKIP (dvz_app() failed)')
            return
        view = dvz.dvz_view_offscreen(app, figure, 32, 32)
        if not view:
            print('raw view post: SKIP (dvz_view_offscreen() failed)')
            return
        dvz.dvz_view_set_render_enabled(view, False)

        calls: list[int | None] = []

        def posted(_view, user_data):
            calls.append(user_data)

        user_data = ctypes.c_void_p(5678)
        if dvz.dvz_view_post(view, posted, user_data) != 0:
            raise RuntimeError('dvz_view_post() failed')
        rc = dvz.dvz_view_render_once(view)
        if rc < 0:
            raise RuntimeError('dvz_view_render_once() failed')
        if calls != [5678]:
            raise RuntimeError('posted callback did not run on render_once')
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))
    from examples.python.raw.offscreen_point import main as run_example  # noqa: PLC0415

    _run_view_post_smoke()
    return run_example([])


if __name__ == '__main__':
    raise SystemExit(main())
