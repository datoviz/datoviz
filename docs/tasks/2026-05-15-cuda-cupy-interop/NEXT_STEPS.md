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
   small position field, and signals the external timeline semaphore.

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
   usage flags, external semaphore handle metadata, and semaphore value.

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

The buffer-level export helper has landed. Keep the next slice focused on turning the scaffold into
a real Linux/NVIDIA smoke:

1. Connect the smoke to the DRP2 registered-buffer render path so Vulkan waits on the CUDA-ready
   timeline value, draws positions directly from the shared buffer, and verifies by readback.
2. Keep `dvz_interop_gpu_ctx()` and the CUDA bridge internal/advanced and wrap them behind the
   eventual `datoviz.cuda_array()` API.
3. Prepare the feature example around a dynamic point cloud whose positions are updated by CuPy
   kernels and rendered by Datoviz without GPU-buffer copies. Prefer a visually interesting but
   bounded effect such as a 50k-200k particle vortex/flow-field or orbital attractor: static
   colors/sizes in scene-owned buffers, zero-copy position buffer updated each frame.

The Python object should own imported CUDA external-memory/semaphore handles, the mapped pointer,
the CuPy owner object, and cleanup. Normal users should synchronize through a context manager:

```python
with shared.cuda_write():
    kernel(shared.array, t)
```

The first documented target remains Linux + NVIDIA CUDA + Vulkan opaque FD external memory +
timeline semaphore FD. Keep this path advanced/unstable and outside portable WebGPU scope.
