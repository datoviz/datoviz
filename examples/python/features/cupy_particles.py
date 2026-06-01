#!/usr/bin/env python3
"""Experimental CuPy + Datoviz shared-scene particle example.

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
        hot = int(90 + 150 * (1.0 - r))
        blue = int(120 + 120 * r)
        green = int(60 + 120 * math.sin(math.pi * r) ** 2)
        colors[i] = dvz.DvzColor(hot, green, blue, 210)
        sizes[i] = 1.2 + 2.4 * (1.0 - r)
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
        self.u = (idx + cp.float32(0.5)) / cp.float32(positions.count)
        self.r0 = cp.sqrt(self.u)
        self.theta0 = idx * cp.float32(GOLDEN_ANGLE)

    def update(self) -> None:
        cp = self.cp
        t = cp.float32(self.frame * self.dt)
        with self.positions.cuda_write() as pos:
            arm = cp.float32(6.0) * self.r0 + cp.float32(0.65) * cp.sin(
                cp.float32(0.7) * t + cp.float32(17.0) * self.u
            )
            spin = t * (
                cp.float32(0.25) + cp.float32(1.45) * (cp.float32(1.0) - self.r0)
            )
            wave = cp.float32(0.045) * cp.sin(
                cp.float32(10.0) * self.r0
                - cp.float32(2.2) * t
                + cp.float32(3.0) * cp.sin(self.theta0)
            )
            pulse = cp.float32(0.035) * cp.sin(cp.float32(3.0) * t + cp.float32(25.0) * self.u)
            radius = cp.float32(0.88) * self.r0 + wave + pulse
            theta = self.theta0 + arm + spin

            pos[:, 0] = radius * cp.cos(theta)
            pos[:, 1] = radius * cp.sin(theta)
            pos[:, 2] = (
                cp.float32(0.16)
                * cp.sin(cp.float32(2.0) * theta - cp.float32(1.7) * t)
                * (cp.float32(1.0) - self.r0)
            )
        self.positions.wait_for_cuda_writes()
        self.frame += 1


def _render_particles(app, stepper: ParticleStepper, frames: int) -> None:
    for _frame in range(frames):
        stepper.update()
        if dvz.dvz_app_render_once(app) != 0:
            raise RuntimeError('dvz_app_render_once() failed')


def _borrowed_app_config():
    app_config = dvz.dvz_app_config()
    app_config.instance_extension_count = 0
    app_config.instance_extensions = None
    app_config.enable_canvas_extensions = False
    app_config.enable_glfw_extensions = False
    return app_config


def _create_live_app(scene, figure, positions: ci.SharedSceneCudaArray, refresh_static_attrs):
    resources = positions.create_app_resources(figure)
    refresh_static_attrs()
    app_config = _borrowed_app_config()
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
        del user_data
        stepper.update()
        dvz.dvz_view_request_frame(view)

    dvz.dvz_view_set_frame_callback(view, on_frame, None)
    dvz.dvz_view_request_frame(view)
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
