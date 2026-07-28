#!/usr/bin/env python3
"""
Live CuPy + Datoviz RGBA image-buffer example.

CuPy writes a Datoviz-owned tightly packed RGBA8 buffer every frame. Datoviz copies that buffer
to its ordinary sampled texture on the GPU, then renders it through the normal image visual path.
This experimental path requires Linux, an NVIDIA CUDA device, CuPy, and Vulkan external-memory
and timeline-semaphore support.
"""

from __future__ import annotations

import argparse
import ctypes
import math
import time

import datoviz.raw as dvz
from datoviz.experimental import cuda as dvz_cuda

WIDTH = 1280
HEIGHT = 720
IMAGE_WIDTH = 768
IMAGE_HEIGHT = 512
TAU = 2.0 * math.pi


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def _skip(reason: str) -> int:
    print(f'cupy image: SKIP ({reason})')
    return 0


def _build_scene(scene, image: dvz_cuda.CudaSceneImageBuffer):
    figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    panel = dvz.dvz_panel_full(figure)
    visual = dvz.dvz_image(scene, 0)
    if not figure or not panel or not visual:
        raise RuntimeError('scene setup failed')

    positions = (ctypes.c_float * 12)(
        -0.92,
        -0.72,
        0.0,
        -0.92,
        0.72,
        0.0,
        0.92,
        -0.72,
        0.0,
        0.92,
        0.72,
        0.0,
    )
    texcoords = (ctypes.c_float * 8)(0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0)
    if dvz.dvz_visual_set_data(visual, b'position', _void_p(positions), 4) != 0:
        raise RuntimeError('dvz_visual_set_data(position) failed')
    if dvz.dvz_visual_set_data(visual, b'texcoords', _void_p(texcoords), 4) != 0:
        raise RuntimeError('dvz_visual_set_data(texcoords) failed')
    image.bind_field(visual)
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DvzAlphaMode.DVZ_ALPHA_OPAQUE) != 0:
        raise RuntimeError('dvz_visual_set_alpha_mode() failed')
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError('dvz_visual_set_depth_test() failed')
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError('dvz_panel_add_visual() failed')
    return figure, panel, visual, positions, texcoords


class ImageStepper:
    """Generate a deterministic animated RGBA pattern directly in the shared CuPy array."""

    def __init__(self, image: dvz_cuda.CudaSceneImageBuffer):
        """Precompute the normalized image coordinate grid."""
        self.image = image
        self.cp = image.cupy
        self.start = time.perf_counter()
        cp = self.cp
        x = cp.arange(image.width, dtype=cp.float32) / cp.float32(max(1, image.width - 1))
        y = cp.arange(image.height, dtype=cp.float32) / cp.float32(max(1, image.height - 1))
        self.u, self.v = cp.meshgrid(x, y)

    def update(self) -> None:
        """Write the next pattern frame and hand it off to Datoviz."""
        cp = self.cp
        t = cp.float32(time.perf_counter() - self.start)
        wave = cp.float32(0.5) + cp.float32(0.5) * cp.sin(
            cp.float32(TAU) * (cp.float32(3.0) * self.u + cp.float32(2.0) * self.v)
            + cp.float32(1.4) * t
        )
        ring = cp.float32(0.5) + cp.float32(0.5) * cp.cos(
            cp.float32(TAU)
            * (cp.hypot(self.u - cp.float32(0.5), self.v - cp.float32(0.5)) * cp.float32(8.0))
            - t
        )
        with self.image.cupy_write() as rgba:
            rgba[..., 0] = (
                cp.float32(255.0) * (cp.float32(0.14) + cp.float32(0.76) * wave)
            ).astype(cp.uint8)
            rgba[..., 1] = (
                cp.float32(255.0)
                * (cp.float32(0.12) + cp.float32(0.70) * (cp.float32(1.0) - ring))
            ).astype(cp.uint8)
            rgba[..., 2] = (
                cp.float32(255.0) * (cp.float32(0.28) + cp.float32(0.62) * ring)
            ).astype(cp.uint8)
            rgba[..., 3] = cp.uint8(255)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--width', type=int, default=IMAGE_WIDTH)
    parser.add_argument('--height', type=int, default=IMAGE_HEIGHT)
    args = parser.parse_args(argv)
    if args.width <= 0 or args.height <= 0:
        parser.error('--width and --height must be positive')

    app = None
    scene = None
    try:
        scene = dvz.dvz_scene()
        if not scene:
            raise RuntimeError('dvz_scene() failed')

        with dvz_cuda.scene_session(scene, present=True) as cuda:
            image = cuda.image_buffer(
                shape=(args.height, args.width, 4), dtype='uint8', format='rgba8_unorm'
            )
            figure, _panel, _visual, _positions, _texcoords = _build_scene(scene, image)
            stepper = ImageStepper(image)
            stepper.update()
            app, view = cuda.live_app(figure, WIDTH, HEIGHT, b'cupy_image')
            try:

                def on_frame(_view, _user_data) -> None:
                    stepper.update()

                if dvz.dvz_view_set_frame_callback(view, on_frame, None) != 0:
                    raise RuntimeError('dvz_view_set_frame_callback() failed')
                dvz.dvz_app_run(app, 0)
                print(
                    f'cupy image: OK ({args.width}x{args.height} RGBA8, '
                    'GPU buffer-to-texture copy)'
                )
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
