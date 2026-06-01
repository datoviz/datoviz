#!/usr/bin/env python3
"""Advanced Linux/NVIDIA CuPy interop smoke scaffold.

This is intentionally not a public example. It records the first Python-side smoke target for the
Vulkan-owned buffer -> CUDA/CuPy import route. It validates the raw advanced interop ctypes
surface before gating on local CuPy/CUDA availability.
"""

from __future__ import annotations

import argparse
import ctypes

import cupy_interop_runtime as ci


DVZ_DRP2_BUFFER_USAGE_COPY_DST = 0x0002
DVZ_DRP2_BUFFER_USAGE_MAP_READ = 0x0004
DVZ_DRP2_TEXTURE_USAGE_COPY_SRC = 0x0001
DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT = 0x0010
DVZ_DRP2_VALIDATION_OK = 0
DVZ_DRP2_VERTEX_STEP_MODE_VERTEX = 0
VK_FORMAT_R32G32_SFLOAT = 103
VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
PARTICLE_COUNT = 1024


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
        attr_format = (ctypes.c_uint32 * 1)(VK_FORMAT_R32G32_SFLOAT)
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
        _check_drp2(
            dvz.dvz_drp2_stream_create_render_pipeline_ex2(
                stream,
                4,
                2,
                3,
                1,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                1,
                binding_stride,
                binding_step,
                1,
                attr_binding,
                attr_location,
                attr_format,
                attr_offset,
            ),
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
    args = parser.parse_args()

    ci.require_linux()
    bridge = ci.load_bridge() if args.bridge_only else None
    if args.bridge_only:
        assert bridge is not None
        print(f'ctypes CuPy interop bridge: OK ({ci.BRIDGE_LIBRARY})')
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
    except ci.InteropSkip as exc:
        print(f'ctypes CuPy interop smoke: SKIP ({exc})')
        raise SystemExit(0)
