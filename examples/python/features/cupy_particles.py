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
        band = math.sin(7.0 * math.pi * u) ** 2
        glow = math.sin(math.pi * r) ** 2
        red = int(80 + 130 * (1.0 - r) + 35 * band)
        green = int(95 + 105 * glow + 35 * (1.0 - band))
        blue = int(155 + 85 * r)
        alpha = int(135 + 90 * (1.0 - r))
        colors[i] = dvz.DvzColor(red, green, blue, alpha)
        sizes[i] = 0.85 + 2.35 * (1.0 - r) ** 1.4
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


class MouseState:
    def __init__(self):
        self.valid = False
        self.down = False
        self.x = WIDTH * 0.5
        self.y = HEIGHT * 0.5

    def set_position(self, x: float, y: float) -> None:
        self.valid = True
        self.x = x
        self.y = y

    def scene_position(self) -> tuple[float, float]:
        return 2.0 * self.x / WIDTH - 1.0, 1.0 - 2.0 * self.y / HEIGHT

    def strength(self) -> float:
        return 3.4 if self.down else 1.0


class ParticleStepper:
    def __init__(self, cp, positions: ci.SharedSceneCudaArray, fps: float):
        self.cp = cp
        self.positions = positions
        self.dt = 1.0 / fps
        self.frame = 0
        self.initialized = False
        idx = cp.arange(positions.count, dtype=cp.float32)
        self.idx = idx
        self.seed_a = self._rand(cp.float32(3.1))
        self.seed_b = self._rand(cp.float32(17.7))
        self.seed_c = self._rand(cp.float32(41.3))
        self.seed_d = self._rand(cp.float32(73.9))
        self.life = cp.float32(2.8) + cp.float32(4.8) * self.seed_c
        self.age = self.life * self.seed_d
        self.vel = cp.zeros((positions.count, 3), dtype=cp.float32)

    def _rand(self, salt):
        cp = self.cp
        value = cp.sin((self.idx + salt) * cp.float32(12.9898)) * cp.float32(43758.5453)
        return value - cp.floor(value)

    def _source(self, t, salt):
        cp = self.cp
        jitter = self._rand(salt)
        theta = self.idx * cp.float32(GOLDEN_ANGLE) + cp.float32(1.7) * t + jitter * cp.float32(
            6.2831853
        )
        radius = cp.float32(0.035) + cp.float32(0.27) * cp.sqrt(self._rand(salt + cp.float32(9.0)))
        wobble = cp.float32(0.75) + cp.float32(0.35) * cp.sin(
            cp.float32(3.0) * t + self.seed_b * cp.float32(6.2831853)
        )
        x = radius * wobble * cp.cos(theta)
        y = radius * cp.sin(theta)
        z = cp.float32(0.08) * cp.sin(theta * cp.float32(2.0) + t)
        return x, y, z

    def update(self, mouse: MouseState | None = None) -> None:
        cp = self.cp
        t = cp.float32(self.frame * self.dt)
        dt = cp.float32(min(self.dt, 1.0 / 30.0))
        with self.positions.cuda_write() as pos:
            if not self.initialized:
                sx, sy, sz = self._source(t, cp.float32(101.0))
                pos[:, 0] = sx
                pos[:, 1] = sy
                pos[:, 2] = sz
                self.initialized = True

            x = pos[:, 0]
            y = pos[:, 1]
            z = pos[:, 2]

            field_x = (
                -cp.float32(0.22) * x
                + cp.float32(0.55)
                * cp.sin(cp.float32(3.8) * y + cp.float32(1.15) * t + self.seed_a)
            )
            field_y = (
                -cp.float32(0.22) * y
                + cp.float32(0.55)
                * cp.cos(cp.float32(3.6) * x - cp.float32(0.9) * t + self.seed_b)
            )
            field_z = (
                -cp.float32(0.35) * z
                + cp.float32(0.18)
                * cp.sin(cp.float32(5.5) * x + cp.float32(4.0) * y + cp.float32(1.6) * t)
            )

            c1x = cp.float32(0.44) * cp.sin(cp.float32(0.43) * t)
            c1y = cp.float32(0.38) * cp.cos(cp.float32(0.31) * t)
            d1x = x - c1x
            d1y = y - c1y
            i1 = cp.exp(-(d1x * d1x + d1y * d1y) * cp.float32(5.0))
            field_x += -d1y * i1 * cp.float32(2.6)
            field_y += d1x * i1 * cp.float32(2.6)

            c2x = cp.float32(0.52) * cp.sin(cp.float32(-0.29) * t + cp.float32(1.6))
            c2y = cp.float32(0.45) * cp.sin(cp.float32(0.37) * t + cp.float32(0.4))
            d2x = x - c2x
            d2y = y - c2y
            i2 = cp.exp(-(d2x * d2x + d2y * d2y) * cp.float32(4.0))
            field_x += d2y * i2 * cp.float32(2.1)
            field_y += -d2x * i2 * cp.float32(2.1)

            if mouse is not None and mouse.valid:
                mx, my = mouse.scene_position()
                dx = x - cp.float32(mx)
                dy = y - cp.float32(my)
                d2 = dx * dx + dy * dy + cp.float32(0.0015)
                influence = cp.exp(-d2 * cp.float32(18.0)) * cp.float32(mouse.strength())
                attract = cp.float32(0.55 if mouse.down else 0.15)
                field_x += (-dy * cp.float32(4.2) - dx * attract) * influence
                field_y += (dx * cp.float32(4.2) - dy * attract) * influence
                field_z += cp.float32(0.35) * influence * cp.sin(cp.float32(7.0) * t + self.seed_c)

            self.vel[:, 0] = self.vel[:, 0] * cp.float32(0.965) + field_x * dt
            self.vel[:, 1] = self.vel[:, 1] * cp.float32(0.965) + field_y * dt
            self.vel[:, 2] = self.vel[:, 2] * cp.float32(0.94) + field_z * dt

            speed = cp.sqrt(
                self.vel[:, 0] * self.vel[:, 0]
                + self.vel[:, 1] * self.vel[:, 1]
                + self.vel[:, 2] * self.vel[:, 2]
            )
            scale = cp.minimum(cp.float32(1.0), cp.float32(1.75) / (speed + cp.float32(1e-4)))
            self.vel *= scale[:, None]

            next_x = x + self.vel[:, 0] * dt
            next_y = y + self.vel[:, 1] * dt
            next_z = z + self.vel[:, 2] * dt
            self.age += dt * (cp.float32(0.85) + cp.float32(0.45) * self.seed_d)
            dead = (
                (self.age > self.life)
                | (next_x * next_x + next_y * next_y > cp.float32(1.55 * 1.55))
                | (cp.abs(next_z) > cp.float32(0.55))
            )
            sx, sy, sz = self._source(t, cp.float32(self.frame) + cp.float32(211.0))
            pos[:, 0] = cp.where(dead, sx, next_x)
            pos[:, 1] = cp.where(dead, sy, next_y)
            pos[:, 2] = cp.where(dead, sz, next_z)

            spawn_speed = cp.float32(0.25) + cp.float32(0.35) * self._rand(
                cp.float32(self.frame) + cp.float32(19.0)
            )
            spawn_theta = self.idx * cp.float32(GOLDEN_ANGLE) + t
            self.vel[:, 0] = cp.where(dead, spawn_speed * cp.cos(spawn_theta), self.vel[:, 0])
            self.vel[:, 1] = cp.where(dead, spawn_speed * cp.sin(spawn_theta), self.vel[:, 1])
            self.vel[:, 2] = cp.where(dead, cp.float32(0.0), self.vel[:, 2])
            self.age = cp.where(dead, cp.float32(0.0), self.age)
            self.life = cp.where(
                dead,
                cp.float32(2.8)
                + cp.float32(4.8) * self._rand(cp.float32(self.frame) + cp.float32(37.0)),
                self.life,
            )
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
    mouse = MouseState()
    stepper = ParticleStepper(cp, positions, fps)
    stepper.update(mouse)

    def on_pointer(_router, event_ptr, _user_data) -> None:
        event = event_ptr.contents
        event_type = event.type
        press = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_PRESS)
        move = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE)
        drag_start = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_START)
        drag = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG)
        release = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_RELEASE)
        drag_stop = int(dvz.DvzPointerEventType.DVZ_POINTER_EVENT_DRAG_STOP)
        if event_type in (
            move,
            press,
            drag_start,
            drag,
        ):
            mouse.set_position(float(event.pos[0]), float(event.pos[1]))
        if event_type in (
            press,
            drag_start,
            drag,
        ):
            left = int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_LEFT)
            none = int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE)
            mouse.down = event.button in (
                none,
                left,
            )
        elif event_type in (
            release,
            drag_stop,
        ):
            mouse.set_position(float(event.pos[0]), float(event.pos[1]))
            mouse.down = False

    router = dvz.dvz_view_input(view)
    if router:
        dvz.dvz_input_subscribe_pointer(router, on_pointer, None)

    def on_frame(view, user_data) -> None:
        del view
        del user_data
        stepper.update(mouse)

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
