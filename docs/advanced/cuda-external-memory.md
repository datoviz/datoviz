# Share Datoviz buffers with CUDA

Status: advanced and experimental. This guide documents the native Vulkan/CUDA external-memory path, not the ordinary Datoviz scene API and not a portable compute feature.

## Scope

The implemented direction is a Datoviz/Vulkan-owned buffer exported to CUDA. It is currently limited to Linux, an NVIDIA CUDA device matched to the Vulkan physical device, opaque-FD external memory, and an opaque-FD timeline semaphore. Vulkan may consume the shared buffer directly as vertex input or copy a tightly packed RGBA8 pixel buffer into a normal Datoviz texture.

```text
Datoviz/Vulkan owns buffer and allocation
  -> export opaque memory FD and timeline-semaphore FD
  -> CUDA imports and maps the allocation
  -> CUDA writes the mapped device pointer
  -> CUDA signals the timeline semaphore
  -> Vulkan waits and either reads vertex input or copies RGBA8 pixels into a texture
  -> the normal Datoviz visual renders
```

This is buffer interop. The image route shares a linear pixel buffer and performs one GPU-only buffer-to-texture copy; it does not export or import a `VkImage` for direct CUDA surface access. Arbitrary GPU objects and CUDA-owned allocations are not supported by this public path, and importing a normal CUDA allocation into Vulkan is not the preferred direction.

## C contract

Create an export-capable context with `dvz_interop_gpu_ctx()` or, when presentation extensions are required, `dvz_interop_gpu_ctx_ex()`. Create the Vulkan buffer with the usage required by its eventual consumer, configure an external timeline semaphore, then export the live `DvzBuffer` with `dvz_interop_buffer_export_from_buffer()`. The recorded native CUDA proof uses vertex usage.

`DvzInteropBufferExport` describes the full allocation size required by CUDA, the logical buffer offset and size, usage metadata, exported handles, and the Vulkan device UUID used to reject CUDA/Vulkan device mismatches. Do not read private `DvzBuffer` allocation fields in application code.

On the CUDA side, import the exported memory handle with the CUDA external-memory API and map the logical range as a device pointer. Import the external timeline semaphore, wait before CUDA accesses data still in use by Vulkan, and signal a monotonically increasing value after CUDA writes. Before direct vertex consumption, call `dvz_interop_buffer_wait_timeline()` with that value; it establishes the Vulkan-side wait and visibility transition to vertex-attribute reads.

## C image-buffer contract

The C image path keeps the external object a buffer:

1. Create a same-scene `DvzSceneBuffer` with `DVZ_SCENE_BUFFER_USAGE_COPY_SRC`, stride 4, and exact byte size `width * height * 4`.
2. Create a 2D `DvzSampledField` with `DVZ_FIELD_FORMAT_RGBA8_UNORM`, attach the borrowed buffer with `dvz_sampled_field_set_buffer()`, and bind that field to one normal image visual.
3. Export/import the underlying Vulkan buffer and timeline semaphore using the same ownership rules as the vertex route.
4. After each complete CUDA write, call `dvz_sampled_field_invalidate()` and arm `dvz_drp2_runtime_arm_external_buffer_timeline()` for the emitted external-buffer resource ID.
5. Execute the next normal scene frame. The exact copy submission waits for CUDA's value at the transfer stage, acquires the buffer for transfer reads, performs `CopyBufferToTexture`, releases it, and signals the next timeline value from the same queue submission.
6. Do not begin another external write while `dvz_drp2_runtime_external_buffer_timeline_pending()` reports a pending transfer; CUDA must wait for the Vulkan signal before reusing the buffer.

The first slice accepts only a full, tightly packed, 2D RGBA8_UNORM payload. One external buffer backs one sampled field, and that field binds one visual. Partial updates, padded rows, other formats, multiple consumers, and direct shared images remain outside the contract.

The generated [Vulkan memory and interop reference](../reference/c-api/runtime-vulkan.md#memory-and-interop) is the exact API authority for [`dvz_interop_buffer_export_from_buffer()`](../reference/c-api/runtime-vulkan.md#dvz_interop_buffer_export_from_buffer), [`dvz_interop_buffer_wait_timeline()`](../reference/c-api/runtime-vulkan.md#dvz_interop_buffer_wait_timeline), and the interop GPU-context helpers.

## Ownership and synchronization

Datoviz owns the Vulkan buffer, allocation, device, and semaphore. A successful export transfers ownership of exported operating-system handles as specified by the export descriptor; CUDA import APIs may consume a handle, while handles still owned by the caller must be closed on every error and teardown path.

Keep the Datoviz buffer alive until CUDA has destroyed its external-memory mapping and semaphore import. A shared pointer is not synchronization: CUDA and Vulkan must not access the range concurrently without the external timeline-semaphore handoff.

Consumer transitions are explicit. `dvz_interop_buffer_wait_timeline()` covers vertex input; `dvz_interop_buffer_wait_timeline_for_consumer()` declares another supported buffer consumer; the sampled-field image route uses the one-shot DRP2 transfer handoff. Do not substitute one synchronization helper for another pipeline consumer.

## Native proof

[`examples/c/advanced/cuda_external_buffer.c`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/advanced/cuda_external_buffer.c) is the end-to-end C proof. It imports the exported buffer and timeline semaphore with the CUDA Runtime API, writes a fullscreen triangle with `cudaMemcpyAsync()`, signals value 1 on the CUDA stream, renders from the same Vulkan buffer through DRP2, and verifies a red readback pixel.

```sh
just example-c advanced/cuda_external_buffer
./build/examples/c/advanced/cuda_external_buffer
```

The target is generated only when CMake finds CUDA on Linux. It uses the CUDA Runtime API from C and does not require a `.cu` translation unit or a CUDA kernel. The C image path has native semantic/backend tests but does not yet have a separate recorded CUDA image render/readback proof.

## Capability checks and failures

Treat missing extensions, inability to export opaque FDs, device UUID mismatch, CUDA import failure, and failed timeline waits as unavailable interop rather than a CPU fallback. Verify the CUDA and Vulkan physical devices before importing memory; matching device indices alone are not sufficient.

This path is unavailable on the WebGPU backend and is not a Windows support claim. Its availability also depends on the running driver and device, even on a Linux/NVIDIA system.

## Related guides

- [Update Datoviz buffers and images from CUDA Python](../how-to/cupy-interop.md) presents the experimental CuPy, PyTorch, and Taichi layer that owns these low-level details.
- [GPU array interoperability](../reference/gpu-array-interop.md) records the status of CuPy and other framework adapters.
- [Compute and graphics](../reference/compute-graphics.md) documents the separate built-in scene-compute path.
