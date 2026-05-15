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

## Next Implementation Slice

1. Add a low-level C descriptor, tentatively `DvzInteropBufferExport`, in
   `datoviz/vk/memory_interop.h`.
2. Add a helper that fills the descriptor from a Vulkan-owned `DvzAllocation` plus explicit logical
   buffer view metadata.
3. Add focused C coverage that exports the descriptor, verifies handle/size/value fields, closes the
   transferred FDs, and does not require Python.
4. After that, add a tiny Python/CuPy smoke that imports the exported FD and writes a vertex buffer,
   matching the existing C CUDA/DRP2 render smoke.
