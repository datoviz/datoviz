# CUDA/CuPy External-Memory Interop

## Status

The active route is Vulkan-owned allocation exported outward to CUDA/CuPy. Do not design new
architecture around importing CUDA/CuPy-owned device pointers into Vulkan.

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

Add a buffer-level export helper before writing public examples. The preferred API is an advanced
interop helper in `datoviz/vk/memory_interop.h`, not an accessor exposing `DvzBuffer::alloc`:

```c
DVZ_EXPORT int dvz_interop_buffer_export_from_buffer(
    DvzBuffer* buffer,
    const DvzInteropBufferExportConfig* config,
    DvzInteropBufferExport* out);
```

This helper should validate the live buffer and exportable allocator, resolve the logical byte
range, export the memory handle, package optional timeline semaphore metadata, and report both
Vulkan usage and DRP2 usage. Consider extending `DvzInteropBufferExport` with `version`,
`vk_usage`, `drp2_usage`, export flags, and Vulkan device UUID fields before exposing the descriptor
to Python.

Then add a tiny Python/CuPy smoke:

1. Datoviz creates a Vulkan-owned exportable vertex/storage buffer.
2. The export descriptor is imported by a small CUDA bridge.
3. The bridge maps a CUDA pointer and wraps it as `cupy.cuda.UnownedMemory` plus a CuPy ndarray.
4. A CuPy kernel writes point positions.
5. CUDA signals an external timeline semaphore.
6. The DRP2 registered-buffer render path waits, draws, and verifies by readback.

The Python object should own imported CUDA external-memory/semaphore handles, the mapped pointer,
the CuPy owner object, and cleanup. Normal users should synchronize through a context manager:

```python
with shared.cuda_write():
    kernel(shared.array, t)
```

The first documented target remains Linux + NVIDIA CUDA + Vulkan opaque FD external memory +
timeline semaphore FD. Keep this path advanced/unstable and outside portable WebGPU scope.
