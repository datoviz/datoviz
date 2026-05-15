# CuPy/CUDA Interop Design

This document records the recommended design direction for zero-copy Python/CuPy integration with
Datoviz.

The focus is real-time visualization of CUDA-produced data through the active scene -> DRP2 ->
vklite/canvas path, without CPU readback and without per-frame GPU copies.


## Goals

1. Allow Python users to run CuPy kernels on GPU data and visualize the result in Datoviz.
2. Keep the Vulkan/Datoviz runtime as the owner of graphics resources and presentation state.
3. Avoid per-frame CPU uploads for frequently updated visual attributes.
4. Preserve scene-level semantics: users describe visual attributes, not Vulkan binding slots.
5. Make synchronization explicit and reliable across CUDA and Vulkan.


## Non-Goals

1. Do not require the scene layer to understand CUDA or CuPy directly.
2. Do not make ordinary `cupy.empty()` allocations a first stable import path.
3. Do not expose raw Vulkan handles through the normal scene API.
4. Do not encode process-local file descriptors or OS handles in normal scene/DRP2 JSON fixtures.
5. Do not add a parallel renderer or bypass the active DRP2/vklite runtime.


## Recommended First Direction

Start with Datoviz/Vulkan-owned exportable buffers that are wrapped as CuPy arrays.

This gives the user the important behavior:

1. Datoviz creates a GPU buffer suitable for rendering.
2. Datoviz exports the allocation through Vulkan external memory.
3. A CUDA-side interop helper imports that memory and maps a CUDA device pointer.
4. Python wraps the pointer as a CuPy array.
5. CuPy kernels update the buffer.
6. Datoviz renders from the same GPU memory.

This is preferable to starting from a normal CuPy allocation because the graphics side can choose the
required Vulkan usage flags, memory requirements, allocation size, alignment, dedicated allocation
policy, and vertex/storage binding layout.


## First User-Facing Shape

The first Python-facing API should look like a high-level shared array, not a Vulkan handle API:

```python
shared = datoviz.cuda_array(
    shape=(n, 3),
    dtype=cp.float32,
    usage="vertex",
    semantic="position",
)

pos = shared.cupy_array

while app.running:
    with shared.cuda_write():
        kernel(pos, t)
    app.render()
```

The exact names can change, but the ownership should remain:

1. Datoviz owns the underlying Vulkan allocation.
2. CUDA/CuPy owns a mapped view while the shared object is alive.
3. Python keeps the Datoviz shared object alive for as long as the CuPy array exists.


## Low-Level Runtime Shape

The low-level implementation should build on `include/datoviz/vk/memory_interop.h` and the vklite
buffer wrapper path.

Required pieces:

1. create exportable `DvzBuffer` objects with explicit Vulkan usage,
2. export allocation handles using platform-appropriate handle types,
3. import those handles into CUDA,
4. map CUDA device pointers,
5. wrap mapped pointers in CuPy with an owner object that preserves lifetime,
6. import or share synchronization primitives with CUDA,
7. register the runtime buffer with DRP2 object ids used by scene emission.

The runtime should expose a registration path such as:

```c
int dvz_drp2_runtime_register_external_buffer(
    DvzDrp2Runtime* runtime,
    uint64_t buffer_id,
    const DvzExternalBufferDesc* desc);
```

The exact public C shape is not final. The important point is that external handles are registered
with a live runtime, not serialized as portable scene data.


## Current Low-Level Status

The repository already contains two low-level CUDA/Vulkan memory tests in
`src/vk/tests/test_memory.c`:

1. `test_memory_cuda_1` is the preferred Vulkan-owned direction. It creates an exportable Vulkan
   buffer, exports the allocation FD with `dvz_allocator_export()`, imports that FD into CUDA with
   `cudaImportExternalMemory()`, maps a CUDA pointer with
   `cudaExternalMemoryGetMappedBuffer()`, imports a Vulkan-owned timeline semaphore into CUDA,
   launches a tiny CUDA kernel, signals from CUDA, waits from Vulkan before copying the buffer, and
   verifies the data from the Vulkan allocation. This is the canonical registered CUDA interop smoke
   in `src/vk/tests/test_vk.c`.
2. `test_memory_cuda_2` exercises the opposite CUDA-owned allocation -> Vulkan import direction with
   CUDA Driver virtual-memory APIs, `dvz_allocator_import_buffer()`, and an exported Vulkan timeline
   semaphore. This direction remains later work because it has proved less reliable as a primary
   architecture, so the test is kept available but is no longer the default registered CUDA interop
   smoke.

The video/NVENC path also imports Vulkan-owned external image memory into CUDA in
`src/video/encoder_backend_nvenc.c`, mapping it as a CUDA mipmapped array before conversion and
encoding. That code is useful evidence for image import mechanics, but the CuPy shared-array route
should still start with buffers because they map directly to CUDA device pointers and CuPy arrays.

NVIDIA CIG (`VK_NV_external_compute_queue` / CUDA-in-Graphics contexts) is not used by these paths
and should remain optional NVIDIA-specific scheduling work. It is not required for Vulkan-owned
external memory imported into CUDA/CuPy.


## Scene Attribute Integration

The scene layer should treat CUDA/CuPy-backed data as an attribute source with external storage.

For the first useful slice, support point positions:

```text
point.position -> external shared buffer, frequent update
point.color    -> regular scene-owned buffer
point.size     -> regular scene-owned buffer
```

The current point pipeline already maps these attributes to separate vertex buffers:

```text
location 0 position <- binding 0
location 1 color    <- binding 1
location 2 size     <- binding 2
```

That means the first interop slice can avoid interleaving changes. The scene emitter only needs to
know that the `position` resource already exists in the runtime and must not emit a CPU
`WRITE_BUFFER` for it.

Later, the scene API should allow layout hints as described in
`../pipeline/ATTRIBUTE_SOURCES.md#attribute-layout-hints`. Frequently updated or externally produced
attributes should be able to remain independent buffers, while static attributes can be interleaved
when that improves cache behavior or reduces binding overhead.


## Synchronization Model

Use external semaphores for CUDA/Vulkan ordering. Avoid `cudaDeviceSynchronize()` in the normal live
loop.

The preferred steady-state loop is:

1. Vulkan signals that buffer slot `i` is no longer being read.
2. CUDA waits on that signal before writing slot `i`.
3. CuPy kernels write the shared buffer on a known CUDA stream.
4. CUDA signals that writes are complete.
5. Vulkan waits on that signal before binding the buffer for draw.
6. Vulkan signals completion after the render work that used the buffer.

Use double or triple buffering for high-rate updates so CUDA and Vulkan do not serialize more than
necessary.

Timeline semaphores are the preferred abstraction where supported. Binary semaphores can be a
fallback if timeline import/export support is unavailable, but they complicate reuse and should not
be the main design target.


## Attribute Layout Policy

The API should let users provide semantic hints, not backend layout instructions.

Useful hints:

1. `external`: the attribute is backed by externally shared GPU memory,
2. `frequent_update`: the attribute changes most frames,
3. `static`: the attribute rarely changes,
4. `prefer_separate`: avoid packing this attribute with unrelated attributes,
5. `prefer_interleaved`: this attribute may be packed with compatible static attributes.

Example policy:

| Attribute | Hint | Runtime choice |
|---|---|---|
| `position` | `external`, `frequent_update` | independent shared vertex buffer |
| `color` | `static` | scene-owned static buffer, possibly interleaved |
| `size` | `static` | scene-owned static buffer, possibly interleaved with `color` |

Pipeline cache keys must include the concrete vertex layout. A point pipeline using three planar
buffers is not interchangeable with a point pipeline using `position` plus interleaved
`color + size`.


## Device Matching

CUDA and Vulkan must operate on the same physical GPU.

The interop setup should validate this before creating shared resources:

1. query the Vulkan physical-device UUID or LUID,
2. query CUDA device UUID or LUID,
3. select or validate the CUDA device that matches the Vulkan device,
4. fail clearly when no matching CUDA device exists.

This validation belongs below the scene layer. Python should receive a clear capability error rather
than undefined behavior or silent fallback to copies.


## Ownership And Lifetime

Ownership rules must be explicit:

1. Datoviz owns Vulkan buffers, Vulkan allocations, Vulkan semaphores, and runtime DRP2 ids.
2. CUDA owns imported CUDA external-memory objects and mapped CUDA pointers.
3. CuPy owns only an ndarray view over a mapped pointer.
4. The Python shared-array object keeps all lower-level resources alive.
5. Destroying the shared object must wait for outstanding CUDA/Vulkan work or defer destruction
   through the existing runtime retirement path.

On Linux, file-descriptor ownership changes across import/export calls must be documented per API.
The implementation should duplicate handles when needed so cleanup paths remain deterministic.


## Why Normal CuPy-Owned Buffers Are Later Work

Importing a normal CuPy allocation into Vulkan should not be the first stable path.

A CuPy array exposes a CUDA device pointer, but that is not enough for Vulkan external memory.
Vulkan needs an importable OS handle and a compatible allocation shape. Ordinary CuPy memory-pool
allocations are not guaranteed to be created as exportable external-memory allocations.

A later CUDA-owned path should use CUDA Driver virtual memory APIs to create exportable allocations,
wrap those allocations in CuPy, and import the exported handle into Vulkan. That path is useful, but
it should wait until the Vulkan-owned export path, external synchronization, and scene attribute
binding contract are stable.


## Validation Plan

Start with narrow tests before adding Python examples:

1. add a device-local exportable buffer variant with staging/readback verification so the test
   reflects real vertex/storage-buffer use rather than only host-mappable memory,
2. add runtime registration for a pre-existing shared buffer consumed by DRP2/vklite without a
   `WRITE_BUFFER` upload,
3. add a point visual with external `position` and regular scene-owned `color`/`size`,
4. add double-buffered CUDA write and Vulkan draw coverage with external semaphore waits/signals,
5. add the Python CuPy wrapper lifetime test,
6. add a live example that updates positions at a fixed rate without CPU upload.

The existing CUDA import/export tests in `src/vk/tests/test_memory.c` are the right low-level
starting point, but the public example should only be added after the synchronization path is tested
with Vulkan validation enabled.


## Example Target

The first public example should visualize a dynamic point cloud:

1. create `N` positions as a Datoviz shared CUDA array,
2. create static scene-owned colors and sizes,
3. run a CuPy kernel each frame that updates positions,
4. render the point visual directly from the shared position buffer,
5. report frame rate and update rate separately.

This example demonstrates the real value of the integration while keeping the first visual contract
small and testable.
