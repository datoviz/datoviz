#!/usr/bin/env python3
"""Experimental CuPy + Datoviz shared-scene particle example.

Datoviz owns a renderable position buffer and exposes it as a CuPy array. CuPy updates the
shared scene attribute directly with array operations, and Datoviz renders points from the same GPU
memory. There is no per-frame CPU upload for particle positions.

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


def _render_particles(
    app, cp, positions: ci.SharedSceneCudaArray, frames: int, fps: float
) -> None:
    idx = cp.arange(positions.count, dtype=cp.float32)
    u = (idx + cp.float32(0.5)) / cp.float32(positions.count)
    r0 = cp.sqrt(u)
    theta0 = idx * cp.float32(GOLDEN_ANGLE)
    dt = 1.0 / fps
    for frame in range(frames):
        t = cp.float32(frame * dt)
        with positions.cuda_write() as pos:
            arm = cp.float32(6.0) * r0 + cp.float32(0.65) * cp.sin(
                cp.float32(0.7) * t + cp.float32(17.0) * u
            )
            spin = t * (cp.float32(0.25) + cp.float32(1.45) * (cp.float32(1.0) - r0))
            wave = cp.float32(0.045) * cp.sin(
                cp.float32(10.0) * r0
                - cp.float32(2.2) * t
                + cp.float32(3.0) * cp.sin(theta0)
            )
            pulse = cp.float32(0.035) * cp.sin(cp.float32(3.0) * t + cp.float32(25.0) * u)
            radius = cp.float32(0.88) * r0 + wave + pulse
            theta = theta0 + arm + spin

            pos[:, 0] = radius * cp.cos(theta)
            pos[:, 1] = radius * cp.sin(theta)
            pos[:, 2] = (
                cp.float32(0.16)
                * cp.sin(cp.float32(2.0) * theta - cp.float32(1.7) * t)
                * (cp.float32(1.0) - r0)
            )
        positions.wait_for_cuda_writes()
        if dvz.dvz_app_render_once(app) != 0:
            raise RuntimeError('dvz_app_render_once() failed')


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--particles', type=int, default=DEFAULT_PARTICLES)
    parser.add_argument('--frames', type=int, default=DEFAULT_FRAMES)
    parser.add_argument('--fps', type=float, default=60.0)
    parser.add_argument('--output', type=Path, help='PNG output path for the last rendered frame')
    args = parser.parse_args(argv)

    try:
        ci.require_linux()
        dvz_raw = ci.require_raw_surface()
        cp = ci.require_cupy()
        bridge = ci.load_bridge()
    except ci.InteropSkip as exc:
        return _skip(str(exc))

    output = args.output
    tempdir = None
    if output is None:
        tempdir = tempfile.TemporaryDirectory(prefix='datoviz-cupy-particles-')
        output = Path(tempdir.name) / 'cupy_particles.png'

    app = None
    scene = None
    try:
        scene = dvz.dvz_scene()
        if not scene:
            raise RuntimeError('dvz_scene() failed')
        with ci.SharedSceneCudaArray(
            dvz_raw, cp, bridge, scene, count=args.particles
        ) as positions:
            try:
                figure, visual, colors, sizes = _build_scene(scene, positions, args.particles)

                def refresh_static_attrs() -> None:
                    _set_static_attrs(visual, colors, sizes, args.particles)

                app, view = positions.create_offscreen_app(
                    scene,
                    figure,
                    WIDTH,
                    HEIGHT,
                    refresh_after_resource_resolution=refresh_static_attrs,
                )

                _render_particles(app, cp, positions, args.frames, args.fps)

                if dvz.dvz_view_capture_png(view, str(output).encode()) != 0:
                    raise RuntimeError('dvz_view_capture_png() failed')
                if not output.exists() or output.stat().st_size == 0:
                    raise RuntimeError('PNG capture was not written')
                print(
                    f'cupy particles: OK ({args.particles} particles, {args.frames} frames, '
                    f'zero-copy positions, output={output})'
                )
            finally:
                if app:
                    dvz.dvz_app_destroy(app)
                    app = None
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
