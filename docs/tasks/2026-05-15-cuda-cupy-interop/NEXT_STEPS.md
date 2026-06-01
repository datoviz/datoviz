# CUDA/CuPy External-Memory Interop

## Status

The active route is Vulkan-owned allocation exported outward to CUDA/CuPy. Do not design new
architecture around importing CUDA/CuPy-owned device pointers into Vulkan.

Current slice update, 2026-06-01:

1. `dvz_interop_buffer_export_from_buffer()` is now the public buffer-level helper in
   `datoviz/vk/memory_interop.h`; examples and bindings no longer need private `DvzBuffer::alloc`
   access.
2. `DvzInteropBufferExport` now carries a descriptor version, Vulkan usage, DRP2 usage, flags,
   and a Vulkan device UUID validity bit plus UUID bytes for CUDA/Vulkan device matching.
3. `tools/bindings/ctypes_cupy_smoke.py` is the first Linux/NVIDIA-only Python smoke scaffold; it
   now validates that CuPy and the generated advanced raw ctypes symbols are available.
4. `spec/bindings/ctypes.yml` includes the memory interop header, forced descriptor layouts, and
   the buffer-level export helper in the raw smoke symbol set.
5. `tools/bindings/cuda_interop_bridge.c` is the optional Linux CUDA Runtime bridge used by the
   smoke. It imports opaque-FD external memory and timeline semaphore FDs, maps a device pointer,
   exposes wait/signal calls, and owns cleanup.
6. `dvz_interop_gpu_ctx()` creates the advanced exportable GPU context used by raw Python smoke
   setup. This remains binding substrate, not the final high-level Python API.
7. `tools/bindings/ctypes_cupy_smoke.py --export-only` now creates a Vulkan-owned
   vertex/storage buffer, exports memory and timeline semaphore FDs, and closes them without
   touching private `DvzBuffer` internals. When CuPy is installed, the full smoke imports the
   descriptor with the CUDA bridge, wraps the pointer with `cupy.cuda.UnownedMemory`, writes a
   small position field, signals the external timeline semaphore, waits on that semaphore from
   Vulkan, renders the shared buffer through DRP2, and verifies a readback pixel.
8. `dvz_interop_buffer_wait_timeline()` is the public advanced Vulkan-side wait helper used by the
   Python smoke before vertex reads from CUDA/CuPy-written shared buffers.
9. Scene external point-position buffers now have a stable registration route: use
   `dvz_scene_buffer_resource_key()` to get the retained buffer key and
   `dvz_drp2_stream_label_id()` on the emitted stream to resolve the runtime DRP2 id, then register
   the live `DvzBuffer` with `dvz_drp2_runtime_register_external_buffer()`.
10. `tools/bindings/cupy_interop_runtime.py` now centralizes the internal Python owner used by the
    smoke: Datoviz exportable buffer lifetime, CUDA bridge import lifetime, CuPy `UnownedMemory`,
    timeline values, `cuda_write()` context manager, Vulkan wait, and DRP2/scene registration
    helpers.
11. `examples/python/features/cupy_particles.py` is the first experimental visual example: CuPy
    runs a CUDA kernel that writes particle positions into a Datoviz/Vulkan-owned zero-copy buffer,
    while Datoviz renders a point visual with static color/size scene attributes.

Current proof points:

1. `src/vk/tests/test_memory.c:test_memory_cuda_1` creates a Vulkan-owned exportable buffer,
   imports it into CUDA, synchronizes cross-API access with an external timeline semaphore, and
   verifies CUDA writes from Vulkan and Vulkan writes from CUDA.
2. `src/drp2/tests/test_drp2.c:test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer`
   registers a CUDA-filled Vulkan-owned external vertex buffer through
   `dvz_drp2_runtime_register_external_buffer()`, renders through the DRP2 vklite runtime, and
   verifies the rendered color by readback.
3. `src/vk/tests/test_memory.c:test_memory_interop_buffer_export` verifies the low-level
   `DvzInteropBufferExport` package: exported memory handle, allocation size, logical byte range,
   usage flags, external semaphore handle metadata, semaphore value, and the Vulkan-side timeline
   wait/barrier helper.

`test_memory_cuda_2` remains useful for later CUDA-owned allocation experiments, but it is not the
primary v0.4 route.

## Contract

The next Python/CuPy-facing contract should expose a small export package rather than a generic
scene binding API:

1. memory handle: Unix FD for the current Linux path; later Win32 handle support can mirror the
   existing allocator platform split,
2. allocation size in bytes,
3. byte offset and logical byte size for the buffer view,
4. DRP2/Vulkan usage flags required by the registered buffer,
5. external timeline semaphore handle,
6. current semaphore value or the next value expected by the consumer,
7. ownership/lifetime rules.

Ownership rules:

1. Datoviz owns the Vulkan allocation, buffer, and semaphore lifecycle.
2. Python/CuPy imports a view and must not free or close Datoviz-owned objects except for handles
   whose ownership is explicitly transferred by the export call.
3. Cross-API access must be ordered with the exported semaphore; CPU sleeps or implicit CUDA stream
   ordering are not a substitute.
4. DRP2 registration should continue to use `dvz_drp2_runtime_register_external_buffer()` for the
   runtime object table, keeping stream data portable.

## Implemented C Descriptor Slice

The low-level C descriptor now lives in `datoviz/vk/memory_interop.h` as
`DvzInteropBufferExport`, with `dvz_interop_buffer_export()` packaging metadata from a
Vulkan-owned `DvzAllocation` and explicit logical buffer view parameters.

`dvz_interop_buffer_export()` transfers ownership of the exported memory handle to the caller on
success. The semaphore handle is copied as metadata only; its ownership remains defined by the code
that exported it.

## Next Implementation Slice

The raw smoke now reaches the DRP2 registered-buffer render/readback path when CuPy is available.
Keep the next slice focused on turning this substrate into the first feature-quality example:

1. Run and tune `examples/python/features/cupy_particles.py` on a Linux/NVIDIA/CuPy workstation:
   verify the rendered PNG/video, tune particle count/colors/animation, and decide whether to keep
   it as an experimental feature or move it behind the future public `datoviz.cuda_array()` API.
2. Keep `dvz_interop_gpu_ctx()`, `dvz_interop_buffer_wait_timeline()`, and the CUDA bridge
   internal/advanced until the public `datoviz.cuda_array()` API is ready.
3. Prepare the feature example around a dynamic point cloud whose positions are updated by CuPy
   kernels and rendered by Datoviz without GPU-buffer copies. Prefer a visually interesting but
   bounded effect such as a 50k-200k particle vortex/flow-field or orbital attractor: static
   colors/sizes in scene-owned buffers, zero-copy position buffer updated each frame.
4. Add a Linux/NVIDIA CI/manual gate that runs the full CuPy smoke on a machine with CuPy installed;
   local validation currently reaches context/export/bridge and skips only at CuPy import.

The Python object should own imported CUDA external-memory/semaphore handles, the mapped pointer,
the CuPy owner object, and cleanup. Normal users should synchronize through a context manager:

```python
with shared.cuda_write():
    kernel(shared.array, t)
```

The first documented target remains Linux + NVIDIA CUDA + Vulkan opaque FD external memory +
timeline semaphore FD. Keep this path advanced/unstable and outside portable WebGPU scope.
