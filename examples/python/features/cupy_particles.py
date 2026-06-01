#!/usr/bin/env python3
"""Live CuPy + Datoviz shared-buffer particle example.

Datoviz owns a renderable position buffer and exposes it as a CuPy array. CuPy updates the
positions every frame, and Datoviz renders points from the same GPU memory.
"""

from __future__ import annotations

import argparse
import ctypes
import math
import time

import datoviz.raw as dvz
from datoviz.experimental import cuda as dvz_cuda


WIDTH = 1600
HEIGHT = 1200
DEFAULT_PARTICLES = int(2**16)
GOLDEN_ANGLE = 2.39996322972865332
TAU = 6.283185307179586


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def _skip(reason: str) -> int:
    print(f'cupy particles: SKIP ({reason})')
    return 0


def _make_style(count: int):
    colors = (dvz.DvzColor * count)()
    sizes = (ctypes.c_float * count)()

    for i in range(count):
        u = (i + 0.5) / count
        r = math.sqrt(u)
        core = (1.0 - r) ** 1.6
        edge = min(1.0, max(0.0, (1.0 - r) / 0.18))
        edge = edge * edge * (3.0 - 2.0 * edge)
        hue = (i * GOLDEN_ANGLE / TAU) % 1.0
        cool = 0.5 + 0.5 * math.sin(TAU * hue)
        colors[i] = dvz.DvzColor(
            int(45 + 160 * core),
            int(110 + 95 * cool),
            int(190 + 55 * (1.0 - core)),
            max(1, int((100 + 100 * core) * edge)),
        )
        sizes[i] = 2.0 + 1.0 * core
    return colors, sizes


def _set_style(visual, colors, sizes, count: int) -> None:
    if dvz.dvz_visual_set_data(visual, b'color', _void_p(colors), count) != 0:
        raise RuntimeError('dvz_visual_set_data(color) failed')
    if dvz.dvz_visual_set_data(visual, b'size', _void_p(sizes), count) != 0:
        raise RuntimeError('dvz_visual_set_data(size) failed')


def _build_scene(scene, positions: dvz_cuda.CudaSceneBuffer, count: int):
    figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    panel = dvz.dvz_panel_full(figure)
    visual = dvz.dvz_point(scene, 0)
    if not figure or not panel or not visual:
        raise RuntimeError('scene setup failed')

    dvz.dvz_panel_set_background_color(panel, 0.005, 0.007, 0.014, 1.0)
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DvzAlphaMode.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError('dvz_visual_set_alpha_mode() failed')
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError('dvz_visual_set_depth_test() failed')

    positions.bind_attr(visual, 'position')
    colors, sizes = _make_style(count)
    _set_style(visual, colors, sizes, count)
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError('dvz_panel_add_visual() failed')

    controller = dvz.dvz_panzoom(scene, None)
    if not controller:
        raise RuntimeError('dvz_panzoom() failed')
    if dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY) != 0:
        raise RuntimeError('dvz_panel_bind_controller() failed')
    return figure, panel, visual, colors, sizes


class ParticleStepper:
    def __init__(self, positions: dvz_cuda.CudaSceneBuffer):
        self.cp = positions.cupy
        self.positions = positions
        self.start = time.perf_counter()
        cp = self.cp
        idx = cp.arange(positions.count, dtype=cp.float32)
        u = (idx + cp.float32(0.5)) / cp.float32(positions.count)
        jitter = cp.float32(0.96) + cp.float32(0.08) * self._rand(idx + cp.float32(31.0))
        self.radius = cp.sqrt(u) * jitter * cp.float32(0.78)
        self.theta0 = idx * cp.float32(GOLDEN_ANGLE)
        self.phase = self._rand(idx) * cp.float32(TAU)
        self.speed = cp.float32(0.08) + cp.float32(0.08) * self._rand(idx + cp.float32(79.0))

    def _rand(self, idx):
        cp = self.cp
        value = cp.sin((idx + cp.float32(17.0)) * cp.float32(12.9898)) * cp.float32(43758.5453)
        return value - cp.floor(value)

    def update(self) -> None:
        cp = self.cp
        t = cp.float32(time.perf_counter() - self.start)
        with self.positions.cupy_write() as pos:
            theta = self.theta0 + self.speed * t
            breath = cp.float32(1.0) + cp.float32(0.025) * cp.sin(cp.float32(0.6) * t + self.phase)
            pos[:, 0] = self.radius * breath * cp.cos(theta)
            pos[:, 1] = self.radius * breath * cp.sin(theta)
            pos[:, 2] = cp.float32(0.03) * cp.sin(cp.float32(2.0) * theta + self.phase)


def _connect_input(panel, view, app) -> None:
    router = dvz.dvz_view_input(view)
    if not router or dvz.dvz_panel_connect_input(panel, router) != 0:
        raise dvz_cuda.CudaInteropUnavailable('input setup failed')


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--particles', type=int, default=DEFAULT_PARTICLES)
    args = parser.parse_args(argv)
    if args.particles <= 0:
        parser.error('--particles must be positive')

    app = None
    scene = None
    try:
        scene = dvz.dvz_scene()
        if not scene:
            raise RuntimeError('dvz_scene() failed')

        with dvz_cuda.scene_session(scene, present=True) as cuda:
            positions = cuda.buffer(
                shape=(args.particles, 3),
                dtype='float32',
                usage=('vertex', 'storage'),
            )
            figure, panel, visual, colors, sizes = _build_scene(scene, positions, args.particles)
            refresh_style = lambda: _set_style(visual, colors, sizes, args.particles)
            app, view = cuda.live_app(
                figure, WIDTH, HEIGHT, b'cupy_particles', after_resources=refresh_style
            )
            try:
                _connect_input(panel, view, app)
                stepper = ParticleStepper(positions)
                stepper.update()

                def on_frame(view, user_data) -> None:
                    del view
                    del user_data
                    stepper.update()

                dvz.dvz_view_set_frame_callback(view, on_frame, None)
                dvz.dvz_app_run(app, 0)
                print(f'cupy particles: OK ({args.particles} particles, zero-copy positions)')
            finally:
                if app:
                    dvz.dvz_app_destroy(app)
                    app = None
    except dvz_cuda.CudaInteropUnavailable as exc:
        return _skip(str(exc))
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        if scene:
            dvz.dvz_scene_destroy(scene)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
