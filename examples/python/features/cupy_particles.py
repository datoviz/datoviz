#!/usr/bin/env python3
"""Minimal CuPy + Datoviz shared-scene particle example.

Datoviz owns a renderable position buffer and exposes it as a CuPy array. CuPy updates the
shared scene attribute directly with array operations, and Datoviz renders points from the same GPU
memory. There is no per-frame CPU upload for particle positions.

The default mode opens a live GLFW window and runs until the window is closed. Use --offscreen for
bounded PNG capture.

Current platform target: Linux + NVIDIA CUDA + CuPy + Vulkan opaque-FD external memory.
"""

from __future__ import annotations

import argparse
import ctypes
import math
import sys
import tempfile
from pathlib import Path

import datoviz.raw as dvz


ROOT_DIR = Path(__file__).resolve().parents[3]
TOOLS_DIR = ROOT_DIR / 'tools' / 'bindings'
sys.path.insert(0, str(TOOLS_DIR))

import cupy_interop_runtime as ci  # noqa: E402


WIDTH = 1024
HEIGHT = 768
DEFAULT_PARTICLES = 65536
DEFAULT_FRAMES = 180
DEFAULT_FPS = 60.0
GOLDEN_ANGLE = 2.39996322972865332
TAU = 6.283185307179586


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def _skip(reason: str) -> int:
    print(f'cupy particles: SKIP ({reason})')
    return 0


def _make_static_attrs(count: int):
    colors = (dvz.DvzColor * count)()
    sizes = (ctypes.c_float * count)()
    for i in range(count):
        u = (i + 0.5) / count
        r = math.sqrt(u)
        core = (1.0 - r) ** 1.4
        red = int(28 + 60 * core)
        green = int(125 + 85 * core)
        blue = int(205 + 35 * core)
        alpha = int(58 + 54 * core)
        colors[i] = dvz.DvzColor(red, green, blue, alpha)
        sizes[i] = 0.85 + 1.45 * core
    return colors, sizes


def _set_static_attrs(visual, colors, sizes, count: int) -> None:
    if dvz.dvz_visual_set_data(visual, b'color', _void_p(colors), count) != 0:
        raise RuntimeError('dvz_visual_set_data(color) failed')
    if dvz.dvz_visual_set_data(visual, b'size', _void_p(sizes), count) != 0:
        raise RuntimeError('dvz_visual_set_data(size) failed')


def _build_scene(scene, positions: ci.SharedSceneCudaArray, count: int):
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

    positions.bind_attr(visual, b'position')
    colors, sizes = _make_static_attrs(count)
    _set_static_attrs(visual, colors, sizes, count)
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError('dvz_panel_add_visual() failed')
    return figure, visual, colors, sizes


class ParticleStepper:
    def __init__(self, cp, positions: ci.SharedSceneCudaArray, fps: float):
        self.cp = cp
        self.positions = positions
        self.dt = 1.0 / fps
        self.frame = 0
        idx = cp.arange(positions.count, dtype=cp.float32)
        self.idx = idx
        u = (idx + cp.float32(0.5)) / cp.float32(positions.count)
        self.radius = cp.sqrt(u) * cp.float32(0.78)
        self.theta0 = idx * cp.float32(GOLDEN_ANGLE)
        self.phase = self._rand(cp.float32(17.0)) * cp.float32(TAU)

    def _rand(self, salt):
        cp = self.cp
        value = cp.sin((self.idx + salt) * cp.float32(12.9898)) * cp.float32(43758.5453)
        return value - cp.floor(value)

    def update(self) -> None:
        cp = self.cp
        t = cp.float32(self.frame * self.dt)
        with self.positions.cuda_write() as pos:
            theta = self.theta0 + cp.float32(0.55) * t
            wave = cp.float32(1.0) + cp.float32(0.08) * cp.sin(
                cp.float32(1.3) * t + self.phase
            )
            pos[:, 0] = self.radius * wave * cp.cos(theta)
            pos[:, 1] = self.radius * wave * cp.sin(theta)
            pos[:, 2] = cp.float32(0.12) * cp.sin(cp.float32(2.0) * theta + self.phase)
        self.positions.wait_for_cuda_writes()
        self.frame += 1


def _render_particles(app, stepper: ParticleStepper, frames: int) -> None:
    for _frame in range(frames):
        stepper.update()
        if dvz.dvz_app_render_once(app) != 0:
            raise RuntimeError('dvz_app_render_once() failed')


def _borrowed_app_config(schedule_mode=None):
    app_config = dvz.dvz_app_config()
    app_config.instance_extension_count = 0
    app_config.instance_extensions = None
    app_config.enable_canvas_extensions = False
    app_config.enable_glfw_extensions = False
    if schedule_mode is not None:
        app_config.schedule_mode = schedule_mode
    return app_config


def _create_live_app(scene, figure, positions: ci.SharedSceneCudaArray, refresh_static_attrs):
    resources = positions.create_app_resources(figure)
    refresh_static_attrs()
    app_config = _borrowed_app_config(dvz.DvzAppScheduleMode.DVZ_APP_SCHEDULE_CONTINUOUS)
    app = dvz.dvz_app_with_resources(scene, ctypes.byref(app_config), ctypes.byref(resources))
    if not app:
        raise ci.InteropSkip('dvz_app_with_resources() failed')
    view = dvz.dvz_view_glfw(app, figure, WIDTH, HEIGHT, b'cupy_particles')
    if not view:
        dvz.dvz_app_destroy(app)
        raise ci.InteropSkip('dvz_view_glfw() failed')
    return app, view


def _run_live(
    cp, scene, figure, positions, refresh_static_attrs, particles: int, frames: int, fps: float
):
    app, view = _create_live_app(scene, figure, positions, refresh_static_attrs)
    stepper = ParticleStepper(cp, positions, fps)
    stepper.update()

    def on_frame(view, user_data) -> None:
        del view
        del user_data
        stepper.update()

    dvz.dvz_view_set_frame_callback(view, on_frame, None)
    dvz.dvz_app_run(app, frames)
    print(
        f'cupy particles: OK ({particles} particles, '
        f'{"live" if frames == 0 else f"{frames} live frames"}, zero-copy positions)'
    )
    return app


def _run_offscreen(
    cp,
    scene,
    figure,
    positions,
    refresh_static_attrs,
    particles: int,
    frames: int,
    fps: float,
    output: Path,
):
    app, view = positions.create_offscreen_app(
        scene,
        figure,
        WIDTH,
        HEIGHT,
        refresh_after_resource_resolution=refresh_static_attrs,
    )
    stepper = ParticleStepper(cp, positions, fps)
    _render_particles(app, stepper, frames)

    if dvz.dvz_view_capture_png(view, str(output).encode()) != 0:
        raise RuntimeError('dvz_view_capture_png() failed')
    if not output.exists() or output.stat().st_size == 0:
        raise RuntimeError('PNG capture was not written')
    print(
        f'cupy particles: OK ({particles} particles, {frames} offscreen frames, '
        f'zero-copy positions, output={output})'
    )
    return app


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--particles', type=int, default=DEFAULT_PARTICLES)
    parser.add_argument(
        '--frames',
        type=int,
        help='live frames to run (0 = until window close); offscreen default is 180',
    )
    parser.add_argument('--fps', type=float, default=DEFAULT_FPS)
    parser.add_argument('--offscreen', action='store_true', help='render bounded frames to a PNG')
    parser.add_argument('--output', type=Path, help='offscreen PNG path for the last rendered frame')
    args = parser.parse_args(argv)
    if args.output is not None and not args.offscreen:
        parser.error('--output requires --offscreen')

    try:
        ci.require_linux()
        dvz_raw = ci.require_raw_surface()
        cp = ci.require_cupy()
        bridge = ci.load_bridge()
    except ci.InteropSkip as exc:
        return _skip(str(exc))

    frames = args.frames if args.frames is not None else (DEFAULT_FRAMES if args.offscreen else 0)
    output = args.output
    tempdir = None
    if args.offscreen and output is None:
        tempdir = tempfile.TemporaryDirectory(prefix='datoviz-cupy-particles-')
        output = Path(tempdir.name) / 'cupy_particles.png'

    app = None
    scene = None
    try:
        scene = dvz.dvz_scene()
        if not scene:
            raise RuntimeError('dvz_scene() failed')
        with ci.SharedSceneCudaArray(
            dvz_raw, cp, bridge, scene, count=args.particles, present=not args.offscreen
        ) as positions:
            try:
                figure, visual, colors, sizes = _build_scene(scene, positions, args.particles)

                def refresh_static_attrs() -> None:
                    _set_static_attrs(visual, colors, sizes, args.particles)

                if args.offscreen:
                    if output is None:
                        raise RuntimeError('offscreen output path was not initialized')
                    app = _run_offscreen(
                        cp,
                        scene,
                        figure,
                        positions,
                        refresh_static_attrs,
                        args.particles,
                        frames,
                        args.fps,
                        output,
                    )
                else:
                    app = _run_live(
                        cp,
                        scene,
                        figure,
                        positions,
                        refresh_static_attrs,
                        args.particles,
                        frames,
                        args.fps,
                    )
            finally:
                if app:
                    dvz.dvz_app_destroy(app)
                    app = None
    except ci.InteropSkip as exc:
        return _skip(str(exc))
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        if scene:
            dvz.dvz_scene_destroy(scene)
        if tempdir is not None:
            tempdir.cleanup()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
