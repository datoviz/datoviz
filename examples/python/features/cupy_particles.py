#!/usr/bin/env python3
"""Experimental CuPy compute + Datoviz render particle example.

This feature example mirrors the C GPU-particle compute-shader idea, but moves the compute logic to
Python/CuPy. Datoviz owns the Vulkan position buffer and renders points from it; CuPy imports that
same buffer and updates positions with a CUDA kernel. There is no per-frame CPU upload for particle
positions.

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


DVZ_SCENE_BUFFER_USAGE_VERTEX = 0x0001
DVZ_SCENE_BUFFER_USAGE_STORAGE = 0x0008
WIDTH = 1024
HEIGHT = 768
DEFAULT_PARTICLES = 65536
DEFAULT_FRAMES = 180


VORTEX_KERNEL = r'''
extern "C" __global__ void vortex_positions(float* pos, int n, float t)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= n) return;

    const float golden = 2.39996322972865332f;
    float u = ((float)i + 0.5f) / (float)n;
    float r0 = sqrtf(u);
    float theta0 = (float)i * golden;

    float arm = 6.0f * r0 + 0.65f * sinf(0.7f * t + 17.0f * u);
    float spin = t * (0.25f + 1.45f * (1.0f - r0));
    float wave = 0.045f * sinf(10.0f * r0 - 2.2f * t + 3.0f * sinf(theta0));
    float pulse = 0.035f * sinf(3.0f * t + 25.0f * u);
    float r = 0.88f * r0 + wave + pulse;
    float theta = theta0 + arm + spin;

    float x = r * cosf(theta);
    float y = r * sinf(theta);
    float z = 0.16f * sinf(2.0f * theta - 1.7f * t) * (1.0f - r0);

    pos[3 * i + 0] = x;
    pos[3 * i + 1] = y;
    pos[3 * i + 2] = z;
}
'''


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


def _resolve_scene_buffer_id(figure, scene_buffer) -> int:
    caps = dvz.DvzCapabilitySnapshot()
    dvz.dvz_capability_snapshot_default(ctypes.byref(caps))
    report = dvz.DvzDiagnosticReport()
    dvz.dvz_diagnostic_report_init(ctypes.byref(report))
    stream = dvz.dvz_figure_emit(figure, ctypes.byref(caps), ctypes.byref(report))
    if dvz.dvz_diagnostic_report_count(ctypes.byref(report)) != 0 or not stream:
        raise RuntimeError('dvz_figure_emit() failed while resolving external buffer id')
    try:
        key = ctypes.create_string_buffer(128)
        if not dvz.dvz_scene_buffer_resource_key(scene_buffer, key, len(key)):
            raise RuntimeError('dvz_scene_buffer_resource_key() failed')
        buffer_id = int(dvz.dvz_drp2_stream_label_id(stream, key.value))
        if buffer_id == 0:
            raise RuntimeError(f'emitted stream has no id for scene buffer {key.value!r}')
        return buffer_id
    finally:
        dvz.dvz_drp2_stream_destroy(stream)


def _build_scene(shared: ci.SharedDatovizCudaArray, count: int):
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')
    figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    panel = dvz.dvz_panel_full(figure)
    visual = dvz.dvz_point(scene, 0)
    if not figure or not panel or not visual:
        raise RuntimeError('scene setup failed')
    dvz.dvz_panel_set_background_color(panel, 0.005, 0.007, 0.014, 1.0)

    position_desc = dvz.DvzSceneBufferDesc()
    position_desc.usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE
    position_desc.stride = ci.POSITION_COMPONENTS * ci.POSITION_DTYPE_SIZE
    position_desc.byte_size = shared.size
    position_buffer = dvz.dvz_scene_buffer(scene, ctypes.byref(position_desc))
    if not position_buffer:
        raise RuntimeError('dvz_scene_buffer(position) failed')
    if not dvz.dvz_visual_set_attr_buffer(visual, b'position', position_buffer, 0, count):
        raise RuntimeError('dvz_visual_set_attr_buffer(position) failed')

    colors, sizes = _make_static_attrs(count)
    _set_static_attrs(visual, colors, sizes, count)
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError('dvz_panel_add_visual() failed')
    return scene, figure, visual, position_buffer, colors, sizes


def _render_particles(app, cp, shared: ci.SharedDatovizCudaArray, frames: int, fps: float) -> None:
    kernel = cp.RawKernel(VORTEX_KERNEL, 'vortex_positions')
    threads = 256
    blocks = (shared.count + threads - 1) // threads
    dt = 1.0 / fps
    for frame in range(frames):
        t = frame * dt
        with shared.cuda_write() as pos:
            kernel((blocks,), (threads,), (pos, shared.count, cp.float32(t)))
        shared.wait_for_cuda_writes()
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

    runtime = None
    app = None
    scene = None
    try:
        with ci.SharedDatovizCudaArray(dvz_raw, cp, bridge, count=args.particles) as shared:
            scene, figure, visual, position_buffer, colors, sizes = _build_scene(shared, args.particles)

            # Resolve before app creation, then re-mark static attrs dirty so the app's first render
            # uploads color/size buffers into the borrowed runtime.
            position_id = _resolve_scene_buffer_id(figure, position_buffer)
            _set_static_attrs(visual, colors, sizes, args.particles)

            cfg = dvz.dvz_drp2_runtime_vklite_config(shared.device, shared.allocator)
            runtime = dvz.dvz_drp2_runtime_vklite(ctypes.byref(cfg))
            if not runtime:
                raise RuntimeError('dvz_drp2_runtime_vklite() failed')
            shared.register_external_buffer(runtime, position_id)

            resources = dvz.DvzAppResources()
            resources.gpu_ctx = shared.exported.ctx
            resources.runtime = runtime
            app_config = dvz.dvz_app_config()
            app = dvz.dvz_app_with_resources(scene, ctypes.byref(app_config), ctypes.byref(resources))
            if not app:
                return _skip('dvz_app_with_resources() failed')
            view = dvz.dvz_view_offscreen(app, figure, WIDTH, HEIGHT)
            if not view:
                return _skip('dvz_view_offscreen() failed')

            _render_particles(app, cp, shared, args.frames, args.fps)

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
        if runtime:
            dvz.dvz_drp2_runtime_destroy(runtime)
        if scene:
            dvz.dvz_scene_destroy(scene)
        if tempdir is not None:
            tempdir.cleanup()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
