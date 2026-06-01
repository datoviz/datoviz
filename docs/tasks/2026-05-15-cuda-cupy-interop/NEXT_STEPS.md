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
   skips until CuPy and the generated advanced raw ctypes symbols are available.

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

1. Generate or hand-write the advanced raw ctypes surface for `datoviz/vk/memory_interop.h`
   without exposing it through high-level Python APIs.
2. Datoviz creates a Vulkan-owned exportable vertex/storage buffer.
3. The export descriptor is imported by a small CUDA bridge.
4. The bridge maps a CUDA pointer and wraps it as `cupy.cuda.UnownedMemory` plus a CuPy ndarray.
5. A CuPy kernel writes point positions.
6. CUDA signals an external timeline semaphore.
7. The DRP2 registered-buffer render path waits, draws, and verifies by readback.

The Python object should own imported CUDA external-memory/semaphore handles, the mapped pointer,
the CuPy owner object, and cleanup. Normal users should synchronize through a context manager:

```python
with shared.cuda_write():
    kernel(shared.array, t)
```

The first documented target remains Linux + NVIDIA CUDA + Vulkan opaque FD external memory +
timeline semaphore FD. Keep this path advanced/unstable and outside portable WebGPU scope.
