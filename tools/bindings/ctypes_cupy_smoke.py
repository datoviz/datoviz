#!/usr/bin/env python3
"""Advanced Linux/NVIDIA CuPy interop smoke scaffold.

This is intentionally not a public example. It records the first Python-side smoke target for the
Vulkan-owned buffer -> CUDA/CuPy import route. It validates the raw advanced interop ctypes
surface before gating on local CuPy/CUDA availability. ``--image`` additionally proves the
CUDA-written RGBA8 image-buffer -> Datoviz texture transfer path with two offscreen captures.
"""

import argparse
import ctypes

import numpy as np

from datoviz._array_facade import dvz_view_capture_rgba, dvz_visual_set_data
from datoviz.experimental import _cuda_runtime as ci
from datoviz.experimental import cuda as dvz_cuda

DVZ_DRP2_BUFFER_USAGE_COPY_DST = 0x0002
DVZ_DRP2_BUFFER_USAGE_MAP_READ = 0x0004
DVZ_DRP2_TEXTURE_USAGE_COPY_SRC = 0x0001
DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT = 0x0010
DVZ_DRP2_VALIDATION_OK = 0
DVZ_DRP2_VERTEX_STEP_MODE_VERTEX = 0
VK_FORMAT_R32G32_SFLOAT = 103
VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
PARTICLE_COUNT = 1024
IMAGE_STRESS_FRAMES = 100


def _check_drp2(ok: bool, label: str) -> None:
    if not ok:
        raise RuntimeError(f'DRP2 command append failed: {label}')


def _render_drp2_external_buffer(
    dvz, shared: ci.CudaMappedDatovizBuffer
) -> tuple[int, int, int, int]:
    cfg = dvz.dvz_drp2_runtime_vklite_config(shared.device, shared.allocator)
    runtime = dvz.dvz_drp2_runtime_vklite(ctypes.byref(cfg))
    if not runtime:
        raise RuntimeError('dvz_drp2_runtime_vklite() failed')

    stream = None
    try:
        shared.register_external_buffer(runtime, 1)

        binding_stride = (ctypes.c_uint32 * 1)(ci.POSITION_COMPONENTS * ci.POSITION_DTYPE_SIZE)
        binding_step = (ctypes.c_uint32 * 1)(DVZ_DRP2_VERTEX_STEP_MODE_VERTEX)
        attr_binding = (ctypes.c_uint32 * 1)(0)
        attr_location = (ctypes.c_uint32 * 1)(0)
        attr_format = (ctypes.c_int * 1)(VK_FORMAT_R32G32_SFLOAT)
        attr_offset = (ctypes.c_uint32 * 1)(0)

        stream = dvz.dvz_drp2_stream()
        if not stream:
            raise RuntimeError('dvz_drp2_stream() failed')
        _check_drp2(dvz.dvz_drp2_stream_hello_renderer(stream, b'cupy-smoke'), 'hello')
        _check_drp2(dvz.dvz_drp2_stream_renderer_hello_reply(stream, b'datoviz'), 'hello reply')
        _check_drp2(
            dvz.dvz_drp2_stream_create_shader_module_format(
                stream,
                2,
                b'VERTEX',
                b'glsl',
                b'#version 450\nlayout(location=0)in vec2 pos;'
                b'void main(){gl_Position=vec4(pos,0,1);}',
            ),
            'vertex shader',
        )
        _check_drp2(
            dvz.dvz_drp2_stream_create_shader_module_format(
                stream,
                3,
                b'FRAGMENT',
                b'glsl',
                b'#version 450\nlayout(location=0)out vec4 color;'
                b'void main(){color=vec4(1,0,0,1);}',
            ),
            'fragment shader',
        )
        pipeline = dvz.dvz_drp2_render_pipeline_desc()
        pipeline.id = 4
        pipeline.vertex_shader_module_id = 2
        pipeline.fragment_shader_module_id = 3
        pipeline.vertex_buffer_slots = 1
        pipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        pipeline.binding_count = 1
        pipeline.binding_strides = binding_stride
        pipeline.binding_step_modes = binding_step
        pipeline.attr_count = 1
        pipeline.attr_bindings = attr_binding
        pipeline.attr_locations = attr_location
        pipeline.attr_formats = attr_format
        pipeline.attr_offsets = attr_offset
        _check_drp2(
            dvz.dvz_drp2_stream_create_render_pipeline(stream, ctypes.byref(pipeline)),
            'render pipeline',
        )
        _check_drp2(
            dvz.dvz_drp2_stream_create_texture_2d_usage(
                stream,
                5,
                2,
                2,
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC,
            ),
            'render target',
        )
        _check_drp2(
            dvz.dvz_drp2_stream_create_buffer(
                stream, 6, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ
            ),
            'readback buffer',
        )
        _check_drp2(dvz.dvz_drp2_stream_begin_command_encoder(stream, 10), 'encoder')
        _check_drp2(
            dvz.dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 5, 0, 0, 0, 1),
            'render pass',
        )
        _check_drp2(dvz.dvz_drp2_stream_set_pipeline(stream, 11, 4), 'set pipeline')
        _check_drp2(dvz.dvz_drp2_stream_set_vertex_buffer(stream, 11, 0, 1, 0), 'vertex buffer')
        _check_drp2(dvz.dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0), 'draw')
        _check_drp2(dvz.dvz_drp2_stream_end_render_pass(stream, 11), 'end render pass')
        _check_drp2(
            dvz.dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 5, 6, 0, 1, 1, 4, 1),
            'copy readback',
        )
        _check_drp2(dvz.dvz_drp2_stream_finish_command_encoder(stream, 10, 12), 'finish')
        _check_drp2(dvz.dvz_drp2_stream_queue_submit(stream, 12, 13), 'submit')

        result = dvz.dvz_drp2_runtime_execute(runtime, stream)
        if not result.ok or result.code != DVZ_DRP2_VALIDATION_OK:
            raise RuntimeError(
                f'DRP2 execution failed: code={result.code}, command={result.command_index}'
            )

        pixel = (ctypes.c_uint8 * 4)()
        if not dvz.dvz_drp2_runtime_download_buffer(runtime, 6, 0, 4, pixel):
            raise RuntimeError('DRP2 readback failed')
        rgba = tuple(int(x) for x in pixel)
        if rgba != (255, 0, 0, 255):
            raise RuntimeError(f'DRP2 readback pixel mismatch: {rgba!r}')
        return rgba
    finally:
        if stream is not None:
            dvz.dvz_drp2_stream_destroy(stream)
        dvz.dvz_drp2_runtime_destroy(runtime)


def _run_cupy_write_smoke(dvz, cp, bridge) -> tuple[int, int, int, int]:
    with ci.CudaMappedDatovizBuffer(dvz, cp, bridge, count=PARTICLE_COUNT) as shared:
        stream = cp.cuda.get_current_stream()
        with shared.cupy_write(stream) as array:
            t = cp.linspace(-1.0, 1.0, shared.count, dtype=cp.float32)
            array[:, 0] = t
            array[:, 1] = cp.sin(t * cp.float32(6.283185307179586))
            array[:, 2] = 0
            array[:3, :] = cp.asarray(
                [[-0.8, -0.8, 0.0], [0.8, -0.8, 0.0], [0.0, 0.8, 0.0]],
                dtype=cp.float32,
            )
        shared.wait_for_cuda_writes()
        return _render_drp2_external_buffer(dvz, shared)


def _run_torch_write_smoke(dvz) -> tuple[int, int, int, int]:
    torch = dvz_cuda._require_torch()
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')
    try:
        with dvz_cuda.scene_buffer(scene, shape=(PARTICLE_COUNT, 3)) as shared:
            with shared.torch_write() as tensor:
                if int(tensor.data_ptr()) != int(shared.array.data.ptr):
                    raise RuntimeError('PyTorch tensor does not share the Datoviz CUDA pointer')
                tensor[:3] = torch.tensor(
                    [[-0.8, -0.8, 0.0], [0.8, -0.8, 0.0], [0.0, 0.8, 0.0]],
                    device=tensor.device,
                    dtype=tensor.dtype,
                )
            return _render_drp2_external_buffer(dvz, shared._shared.shared)
    finally:
        dvz.dvz_scene_destroy(scene)


def _run_taichi_write_smoke(dvz) -> tuple[int, int, int, int]:
    try:
        import taichi as ti  # noqa: PLC0415
    except Exception as exc:
        raise dvz_cuda.CudaInteropUnavailable(f'Taichi unavailable: {exc}') from exc

    ti.init(arch=ti.cuda)

    @ti.kernel
    def write_triangle(array: ti.types.ndarray(dtype=ti.f32, ndim=2)):
        array[0, 0] = -0.8
        array[0, 1] = -0.8
        array[0, 2] = 0.0
        array[1, 0] = 0.8
        array[1, 1] = -0.8
        array[1, 2] = 0.0
        array[2, 0] = 0.0
        array[2, 1] = 0.8
        array[2, 2] = 0.0

    scene = dvz.dvz_scene()
    if not scene:
        ti.reset()
        raise RuntimeError('dvz_scene() failed')
    try:
        with dvz_cuda.scene_buffer(scene, shape=(PARTICLE_COUNT, 3)) as shared:
            with shared.taichi_write() as tensor:
                write_triangle(tensor)
            return _render_drp2_external_buffer(dvz, shared._shared.shared)
    finally:
        dvz.dvz_scene_destroy(scene)
        ti.reset()


def _validate_image_capture(
    before: np.ndarray, after: np.ndarray
) -> tuple[tuple[int, ...], tuple[int, ...]]:
    """Assert that the image center changed from red to green after the second CUDA write."""

    if before.shape != after.shape or before.ndim != 3 or before.shape[2] != 4:
        raise RuntimeError('offscreen image captures have incompatible RGBA shapes')

    height, width, _ = before.shape
    y0, y1 = height * 3 // 8, height * 5 // 8
    x0, x1 = width * 3 // 8, width * 5 // 8
    red = np.median(before[y0:y1, x0:x1, :3], axis=(0, 1))
    green = np.median(after[y0:y1, x0:x1, :3], axis=(0, 1))
    difference = float(
        np.mean(
            np.abs(
                after[y0:y1, x0:x1, :3].astype(np.int16)
                - before[y0:y1, x0:x1, :3].astype(np.int16)
            )
        )
    )
    if red[0] < 80 or red[0] < max(red[1], red[2]) + 40:
        raise RuntimeError(
            f'first image capture is not red-dominant: {tuple(int(v) for v in red)}'
        )
    if green[1] < 80 or green[1] < max(green[0], green[2]) + 40:
        raise RuntimeError(
            f'second image capture is not green-dominant: {tuple(int(v) for v in green)}'
        )
    if difference < 40:
        raise RuntimeError(
            f'image captures did not change enough: mean RGB delta={difference:.1f}'
        )
    return tuple(int(v) for v in red), tuple(int(v) for v in green)


def _run_cupy_image_smoke(
    dvz, frame_count: int = IMAGE_STRESS_FRAMES, framework: str = 'cupy'
) -> tuple[tuple[int, ...], tuple[int, ...]]:
    """Render repeated CUDA-written frames through the external buffer-to-texture path."""

    if frame_count < 2:
        raise ValueError('image smoke requires at least two frames')
    if framework not in ('cupy', 'torch', 'taichi'):
        raise ValueError(f'unknown image writer framework {framework!r}')

    taichi = None
    taichi_write_color = None
    if framework == 'taichi':
        try:
            import taichi as ti  # noqa: PLC0415
        except Exception as exc:
            raise dvz_cuda.CudaInteropUnavailable(f'Taichi unavailable: {exc}') from exc
        taichi = ti
        taichi.init(arch=taichi.cuda)

        @taichi.kernel
        def write_color(
            array: taichi.types.ndarray(dtype=taichi.u8, ndim=3),
            red: taichi.u8,
            green: taichi.u8,
            blue: taichi.u8,
            alpha: taichi.u8,
        ):
            for y, x in taichi.ndrange(array.shape[0], array.shape[1]):
                array[y, x, 0] = red
                array[y, x, 1] = green
                array[y, x, 2] = blue
                array[y, x, 3] = alpha

        taichi_write_color = write_color

    def write_image(image, session, color, stream) -> None:
        if framework == 'cupy':
            with stream:
                with image.cupy_write(stream) as pixels:
                    pixels[...] = session.cupy.asarray(color, dtype=session.cupy.uint8)
        elif framework == 'torch':
            torch = dvz_cuda._require_torch()
            with image.torch_write(stream) as pixels:
                pixels[...] = torch.tensor(color, device=pixels.device, dtype=pixels.dtype)
        else:
            assert taichi_write_color is not None
            with image.taichi_write(stream) as pixels:
                taichi_write_color(pixels, *color)

    scene = dvz.dvz_scene()
    if not scene:
        if taichi is not None:
            taichi.reset()
        raise RuntimeError('dvz_scene() failed')

    try:
        figure = dvz.dvz_figure(scene, 64, 64, 0)
        if not figure:
            raise RuntimeError('dvz_figure() failed')
        panel = dvz.dvz_panel_full(figure)
        if not panel:
            raise RuntimeError('dvz_panel_full() failed')
        visual = dvz.dvz_image(scene, 0)
        if not visual:
            raise RuntimeError('dvz_image() failed')
        positions = np.asarray(
            [[-0.88, -0.88, 0.0], [-0.88, 0.88, 0.0], [0.88, -0.88, 0.0], [0.88, 0.88, 0.0]],
            dtype=np.float32,
        )
        texcoords = np.asarray([[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]], dtype=np.float32)
        if dvz_visual_set_data(visual, b'position', positions) != 0:
            raise RuntimeError('dvz_visual_set_data(position) failed')
        if dvz_visual_set_data(visual, b'texcoords', texcoords) != 0:
            raise RuntimeError('dvz_visual_set_data(texcoords) failed')
        if dvz.dvz_visual_set_depth_test(visual, False) != 0:
            raise RuntimeError('dvz_visual_set_depth_test() failed')
        if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
            raise RuntimeError('dvz_panel_add_visual() failed')

        with dvz_cuda.scene_session(scene) as session:
            app = None
            try:
                if framework == 'cupy':
                    writer_stream = session.cupy.cuda.Stream(non_blocking=True)
                else:
                    writer_stream = dvz_cuda._require_torch().cuda.Stream()
                image = session.image_buffer(shape=(8, 8, 4), dtype='uint8')
                write_image(image, session, (255, 0, 0, 255), writer_stream)
                image.bind_field(visual)

                app, view = session.offscreen_app(figure, 64, 64)
                if dvz.dvz_view_render_once(view) < 0:
                    raise RuntimeError('first dvz_view_render_once() failed')
                before = dvz_view_capture_rgba(view)

                after = None
                for frame_idx in range(1, frame_count):
                    color = (0, 255, 0, 255) if frame_idx % 2 else (255, 0, 0, 255)
                    write_image(image, session, color, writer_stream)
                    if dvz.dvz_view_render_once(view) < 0:
                        raise RuntimeError(
                            f'image dvz_view_render_once() failed at frame {frame_idx}'
                        )
                    if frame_idx == 1:
                        after = dvz_view_capture_rgba(view)
                if after is None:
                    raise RuntimeError('image smoke did not capture its second frame')
                return _validate_image_capture(before, after)
            finally:
                if app:
                    dvz.dvz_app_destroy(app)
    finally:
        dvz.dvz_scene_destroy(scene)
        if taichi is not None:
            taichi.reset()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--bridge-only', action='store_true', help='build and load the optional CUDA bridge only'
    )
    parser.add_argument(
        '--ctx-only',
        action='store_true',
        help='create and destroy an exportable Datoviz GPU context',
    )
    parser.add_argument(
        '--export-only', action='store_true', help='create and export a Datoviz buffer only'
    )
    parser.add_argument(
        '--torch', action='store_true', help='run the optional zero-copy PyTorch write smoke'
    )
    parser.add_argument(
        '--taichi', action='store_true', help='run the optional zero-copy Taichi write smoke'
    )
    parser.add_argument(
        '--image',
        action='store_true',
        help='run the CUDA RGBA8 image-buffer to Datoviz texture smoke',
    )
    args = parser.parse_args()
    if args.torch and args.taichi:
        parser.error('--torch and --taichi are mutually exclusive')

    ci.require_linux()
    bridge = ci.load_bridge() if args.bridge_only else None
    if args.bridge_only:
        assert bridge is not None
        print('ctypes CuPy interop bridge: OK')
        return 0

    dvz = ci.require_raw_surface()
    if args.ctx_only:
        ci.probe_interop_context(dvz)
        print('ctypes CuPy interop context: OK')
        return 0
    if args.export_only:
        with ci.ExportedDatovizBuffer(dvz, count=PARTICLE_COUNT) as exported:
            print(
                'ctypes CuPy interop export: OK '
                f'(size={exported.desc.size}, memory_fd={exported.desc.memory_handle}, '
                f'semaphore_fd={exported.desc.semaphore_handle})'
            )
        return 0

    if args.image:
        framework = 'taichi' if args.taichi else 'torch' if args.torch else 'cupy'
        first_red, first_green = _run_cupy_image_smoke(dvz, framework=framework)
        second_red, second_green = _run_cupy_image_smoke(dvz, framework=framework)
        cp = ci.require_cupy()
        device = cp.cuda.runtime.getDeviceProperties(cp.cuda.runtime.getDevice())
        device_name = device['name']
        if isinstance(device_name, bytes):
            device_name = device_name.decode(errors='replace')
        datoviz_version = dvz.dvz_version().decode()
        framework_name = {'cupy': 'CuPy', 'torch': 'PyTorch', 'taichi': 'Taichi'}[framework]
        print(
            f'ctypes {framework_name} image interop smoke: READY '
            f'(runs=2, frames_per_run={IMAGE_STRESS_FRAMES}, '
            f'red={first_red}/{second_red}, green={first_green}/{second_green}, '
            f'GPU={device_name}, CUDA driver={cp.cuda.runtime.driverGetVersion()}, '
            f'CUDA runtime={cp.cuda.runtime.runtimeGetVersion()}, CuPy={cp.__version__}, '
            f'Datoviz={datoviz_version})'
        )
        return 0

    if args.torch:
        rgba = _run_torch_write_smoke(dvz)
        print(f'ctypes PyTorch interop smoke: READY (zero-copy write+render, pixel={rgba})')
        return 0

    if args.taichi:
        rgba = _run_taichi_write_smoke(dvz)
        print(f'ctypes Taichi interop smoke: READY (zero-copy write+render, pixel={rgba})')
        return 0

    cp = ci.require_cupy()
    bridge = ci.load_bridge()
    assert bridge is not None
    rgba = _run_cupy_write_smoke(dvz, cp, bridge)
    print(
        f'ctypes CuPy interop smoke: READY '
        f'(CuPy {cp.__version__}, zero-copy write+render, pixel={rgba})'
    )
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ci.InteropSkip, dvz_cuda.CudaInteropUnavailable) as exc:
        print(f'ctypes CuPy interop smoke: SKIP ({exc})')
        raise SystemExit(0)
