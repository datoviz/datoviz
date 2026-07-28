# GPU Image Interop Implementation Plan

Status: proposed implementation plan. No GPU image interop API is implemented by this document.

This plan extends the experimental Linux/NVIDIA CUDA interop work from vertex buffers to image visuals and sampled fields without introducing a parallel renderer, frame stream, Vulkan wrapper, or backend-specific scene semantic.


## Decision Summary

Implement image support in two distinct lanes:

1. **Tensor image buffer:** CuPy, PyTorch, or Taichi writes a Datoviz-owned exportable linear buffer; the normal Datoviz runtime performs one GPU-only buffer-to-image copy before rendering.
2. **Direct shared image:** Datoviz exports a dedicated `VkImage`; CUDA imports it as a mipmapped array and exposes CUDA surface access; the normal Datoviz runtime samples the same image after an explicit ownership, layout, and semaphore handoff.

Lane 1 is the first product path because it preserves ordinary tensor semantics and reuses the existing external-buffer implementation. Lane 2 is a separately gated zero-copy path because an optimally tiled Vulkan image is not a normal linear CuPy array or PyTorch tensor.

Do not make both lanes modes of one ambiguous Python object. The first API should be `image_buffer()` or an equivalent name that promises a tensor view plus one device copy. A future direct-image object should have a distinct name and surface-oriented contract.


## Current Baseline

The repository already has:

1. Datoviz-owned exportable buffers in `include/datoviz/vk/memory_interop.h`, `src/vk/memory.c`, and `src/vklite/buffers.c`.
2. CUDA opaque-FD memory and timeline-semaphore import in `datoviz/experimental/_cuda_interop_bridge.c`.
3. Scoped CuPy, PyTorch, and Taichi buffer writers in `datoviz/experimental/cuda.py`.
4. DRP2 live-runtime external-buffer registration through `DvzDrp2ExternalBufferDesc` and `dvz_drp2_runtime_register_external_buffer()`.
5. DRP2 `CopyBufferToTexture` execution and validation.
6. Retained sampled fields and image visuals that lower through the normal frame-plan, DRP2 texture, sampler, descriptor, and render path.
7. CUDA external-image mapping precedent in the NVENC provider.

The repository does not yet have:

1. A validated real-runtime configuration layout in `datoviz/experimental/_cuda_runtime.py`; `CONFIG_FIELDS` is stale relative to `DvzInteropBufferExportConfig` and must be fixed before treating the current Python runtime as evidence.
2. A scene/frame-plan contract for copying an externally populated scene buffer into a persistent sampled texture.
3. A sampled-field mode that has external GPU backing and no retained CPU pixel payload.
4. Export-capable image creation in `dvz_allocator_image()`; unlike buffer creation, it does not append `VkExternalMemoryImageCreateInfo`.
5. A public image-export descriptor or `DvzImages` export helper.
6. CUDA bridge support for `cudaExternalMemoryGetMappedMipmappedArray()`, CUDA array levels, and CUDA surface objects.
7. Explicit Vulkan-to-CUDA image release and CUDA-to-Vulkan image acquire helpers.
8. DRP2 live-runtime external-texture registration.
9. Physical Linux/NVIDIA proof for PyTorch or Taichi image updates.


## Goals

1. Let CUDA Python frameworks generate pixels without a CPU pixel round trip.
2. Keep image rendering on the existing scene -> frame plan -> DRP2 -> vklite -> canvas/app path.
3. Preserve one canonical texture resource key and one normal image/sampled-field visual binding path.
4. Make memory ownership, operating-system handle ownership, image layout, queue-family ownership, and timeline values explicit.
5. Make unsupported platforms, formats, devices, and providers fail with actionable diagnostics instead of silently falling back to NumPy uploads.
6. Provide a tensor-friendly path first and a true zero-copy CUDA-surface path only after independent native proof.
7. Keep the base library and base Python wheel free from mandatory CUDA, CuPy, PyTorch, or Taichi dependencies.


## Non-Goals

1. Import arbitrary CUDA-owned pointers or arrays into Vulkan as the primary design.
2. Treat Taichi GGUI as a Datoviz backend or reuse its renderer.
3. Promise cross-platform external-image interop in the first slice.
4. Support images, 3D textures, cube maps, array layers, mip generation, compressed formats, multiplanar formats, depth/stencil formats, MSAA, or render-target sharing in the first slice.
5. Expose raw Vulkan handles, file descriptors, CUDA external-memory handles, or DRP2 numeric resource IDs as the normal Python workflow.
6. Add JAX, TensorFlow, Numba, or generic DLPack image support before CuPy, PyTorch, and Taichi are physically proven.
7. Count mocked tests, macOS skips, or hosted builds without suitable hardware as Linux/NVIDIA execution evidence.


## Initial Supported Slice

Lane 1 starts with:

1. Linux and NVIDIA CUDA.
2. A two-dimensional color image.
3. `shape=(height, width, 4)`.
4. `dtype="uint8"`.
5. Tightly packed C-contiguous rows.
6. `VK_FORMAT_R8G8B8A8_UNORM`.
7. One layer, one mip level, and sample count 1.
8. Full-image writes only.
9. Sampling by the existing image visual through the ordinary `"field"` slot.
10. One GPU buffer-to-image copy after each completed external write, and no copy when the resource is unchanged.

Lane 2 initially uses the same image restrictions plus:

1. Optimal tiling.
2. A dedicated Vulkan allocation.
3. `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT`.
4. CUDA import with the dedicated-memory flag.
5. CUDA mipmapped-array level 0 and surface access.
6. `VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT` for the first proof, subject to the external-image format query.
7. `cudaArraySurfaceLoadStore` on the CUDA mapping.
8. A `GENERAL` layout while CUDA owns the image and the declared sampled layout while Vulkan owns it.

`R32_SFLOAT` is the next format candidate after RGBA8 succeeds. sRGB interpretation must use the existing Datoviz color-role contract rather than silently changing the Vulkan format.


## Architecture Invariants

1. Datoviz owns the Vulkan buffer or image, allocation, view, sampler, semaphore, scene resource, and runtime lifetime.
2. CUDA imports borrowed access to that Datoviz-owned allocation and must release all CUDA objects before Datoviz destroys or recreates it.
3. A shared allocation is not synchronization.
4. Semaphore waits do not replace image layout transitions or queue-family ownership transfers.
5. The runtime must never destroy, transition, begin, end, reset, or submit a borrowed object unless its registration contract grants that responsibility.
6. External registrations are live runtime state and are not serialized portable DRP2 commands.
7. Scene semantics contain shape, format, color role, sampling, lifetime, and dirty state; they do not contain CUDA handles or Vulkan-specific synchronization details.
8. External resources use the same canonical scene resource keys, persistent emitter IDs, bind groups, samplers, draw packets, and render passes as CPU-authored resources.
9. Every generation or recreation invalidates prior CUDA imports; generation mismatch must fail before access.
10. Timeline values are monotonic and partition exclusive ownership so CUDA and Vulkan never access the resource concurrently.


## Dependency Graph

```text
Checkpoint 0: repair current CUDA Python ABI validation
    |
    +--> Lane 1 / tensor image buffer
    |      Checkpoint 1: frame-plan buffer-to-texture transfer contract
    |          |
    |      Checkpoint 2: external sampled-field backing and normal image binding
    |          |
    |      Checkpoint 3: CuPy image_buffer() API and clean-package tests
    |          |
    |      Checkpoint 4: physical CuPy render/readback proof
    |          |
    |      Checkpoint 5: PyTorch and Taichi image writers and proof
    |
    +--> Lane 2 / direct shared VkImage
           Checkpoint 6: exportable image allocation and capability contract
               |
           Checkpoint 7: native CUDA array/surface import and synchronization proof
               |
           Checkpoint 8: DRP2 external sampled-texture registration
               |
           Checkpoint 9: scene direct-image binding and optional Python surface API
               |
           Checkpoint 10: physical repeated-frame proof and documentation
```

Lane 2 may begin its allocator/capability work after Checkpoint 0, but it must not publish a Python API before Lane 1 establishes the scene resource and evidence conventions.


## Checkpoint 0: Repair The Existing CUDA Python Baseline

### Objective

Make the current buffer provider validate and exercise the actual generated binding layout before building image support on it.

### Work

1. Update `CONFIG_FIELDS` in `datoviz/experimental/_cuda_runtime.py` to match `DvzInteropBufferExportConfig`, including `struct_size`, `flags`, and `export_flags`.
2. Initialize public configuration structs through their constructor/default helper instead of relying on zeroed ABI prologues.
3. Add a test using a binding-shaped fake with the exact generated field order.
4. Add a clean-wheel import test that reaches raw-surface validation while CUDA remains unavailable.
5. Confirm the lazy bridge cache remains independent of the source checkout and base package import.

### Validation

1. Focused experimental CUDA Python tests.
2. `just ctypes`.
3. `just ctypes-check`.
4. `just ctypes-package-smoke`.
5. Linux bridge-only smoke when available.

### Commit

`fix: align experimental CUDA runtime with the generated ABI`

### Delegation

A low-reasoning subagent may update field-order tests and package smoke fixtures after the primary agent records the exact ABI decision. The primary agent owns the ABI comparison and final review.


## Lane 1: Tensor Image Buffer

## Checkpoint 1: Add A Frame-Plan Buffer-To-Texture Transfer Contract

### Objective

Represent an external scene buffer as the source of a normal persistent DRP2 texture without encoding the operation as a CPU upload.

### Work

1. Generalize `DvzFramePlanCopyDesc` or add a focused transfer descriptor so the frame plan can express buffer-to-texture as well as the existing texture-to-buffer readback direction.
2. Carry source and destination resource kinds, source byte offset, destination origin, extent, format, bytes per texel, bytes per row, and rows per image explicitly.
3. Preserve the canonical external scene-buffer resource key as the source key and the canonical visual texture resource key as the destination key.
4. Extend `src/scene/runtime/frame_plan.c` and the emitter helpers so setup creates the normal texture and frame execution emits `CopyBufferToTexture` before the render pass.
5. Require `COPY_SRC` on the registered external buffer and `COPY_DST | TEXTURE_BINDING` on the destination texture.
6. Emit the copy only when the external image-buffer generation or write revision changed.
7. Generalize the CUDA-to-Vulkan buffer wait so external writes become visible to transfer reads instead of using the existing vertex-input-only barrier.
8. Add the reverse Vulkan-to-CUDA handoff: after the copy completes, Vulkan signals the next timeline value so the next CUDA write may reuse the buffer without waiting for image sampling to finish.
9. Attach the wait and signal to ordered queue submissions; do not replace the alternating ownership protocol with a CPU wait.
10. Keep DRP2 serialization, semantic validation, recording, and WebGPU behavior honest; unsupported external runtime registration remains a capability/runtime concern, while the copy command itself remains ordinary DRP2.

### Files

Primary files include `include/datoviz/scene/frame_plan.h`, `include/datoviz/scene/enums.h`, `src/scene/frame_plan/`, `src/scene/runtime/`, `include/datoviz/drp2/stream.h`, and existing DRP2 transfer tests. Avoid a new renderer or CUDA-specific DRP2 command.

### Validation

1. Frame-plan node construction and JSON tests.
2. DRP2 semantic positive and negative fixtures for row pitch, range, usage, extent, and resource kind.
3. vklite execution test that copies a registered external buffer into a texture and reads back deterministic pixels.
4. WebGPU fixture validation for the portable copy command where applicable.
5. `just test drp2`.
6. `just test scene/frame-plan`.
7. `just spec-check`.

### Commit

`scene: add external-buffer texture transfer planning`

### Delegation

A medium-reasoning subagent may implement the DRP2 fixture and semantic-test expansion in files disjoint from the frame-plan work. Another low-reasoning subagent may update metadata registries and generated fixture expectations. The primary agent owns the descriptor shape, resource ordering, and combined runtime review.


## Checkpoint 2: Add External Sampled-Field Backing

### Objective

Let an image visual bind a sampled field whose pixels are produced in an external scene buffer while preserving the normal visual, texture key, sampler, bind-group, and render path.

### Work

1. Add an explicit sampled-field backing mode such as `OWNED_CPU_DATA` versus `EXTERNAL_BUFFER`.
2. Store the borrowed `DvzSceneBuffer`, field descriptor, color role, dimensions, format, and external write revision in the sampled field.
3. Add a public constructor or setter that binds a compatible scene buffer to a sampled field without accepting raw backend handles.
4. Reject `dvz_sampled_field_set_data()`, resize, and region-update calls while the field is externally backed, unless a future explicit API switches ownership mode.
5. Suppress CPU payload preparation and `WriteTexture` emission for external backing.
6. Append the buffer-to-texture transfer node under the canonical `visual.<index>.texture` key.
7. Preserve `DvzFramePlanVisualMeta.texture_id`, image descriptor resolution, sampler selection, picking, query semantics where data is available, and ordinary draw generation.
8. Define CPU query behavior honestly: either report unavailable external pixel data or require an explicit readback; never return stale retained CPU bytes.
9. Ensure field/visual destruction, rebind, scene destruction, and runtime reset do not destroy the externally owned scene buffer.

### Files

Primary files include `include/datoviz/scene/field.h`, scene public type/enum headers, `src/scene/domain/field*`, `src/scene/scene_emit/texture_upload.c`, `src/scene/core/resource_key.c`, `src/scene/scene_emit/metadata.c`, and image visual tests.

### Validation

1. External field emits no CPU texture upload or `WriteTexture`.
2. External field emits one correctly ordered buffer-to-texture copy after a write.
3. An unchanged field emits no redundant pixel copy.
4. Rebinding preserves canonical IDs and refreshes dependent descriptors.
5. CPU mutation and query behavior returns the declared error.
6. Scene-to-runtime render/readback proves the normal image visual samples the copied pixels.
7. Cross-scene ownership and destruction tests.
8. `just ctypes` and `just ctypes-check` after the public API change.

### Commit

`scene: bind external pixel buffers to sampled fields`

### Delegation

A medium-reasoning subagent may implement field-mode unit tests and upload-suppression tests. A low-reasoning subagent may update generated binding policy fixtures after the primary agent freezes the public structs. The primary agent owns public semantics, lifetime rules, and scene/runtime integration.


## Checkpoint 3: Expose The CuPy Image-Buffer API

### Objective

Provide the first usable Python image workflow using the proven linear external-memory representation.

### Proposed API

```python
from datoviz.experimental import cuda

with cuda.scene_session(scene, present=True) as session:
    frame = session.image_buffer(
        shape=(height, width, 4),
        dtype="uint8",
        format="rgba8_unorm",
    )
    frame.bind_field(image_visual, "field")

    with frame.cupy_write() as pixels:
        pixels[...] = rgba
```

### Work

1. Add `CudaSceneImageBuffer` and `CudaSceneSession.image_buffer()` without overloading `CudaSceneBuffer`.
2. Generalize only the lower mapped-buffer storage layer needed for `uint8` and three-dimensional Python shapes; keep existing float32 vertex-buffer behavior stable.
3. Allocate an external buffer with exact tightly packed byte size and `COPY_SRC` runtime usage.
4. Create and bind the external sampled field from Checkpoint 2.
5. Reuse scoped semaphore ordering and make scope exit advance the image write revision.
6. Keep the yielded CuPy array borrowed and invalid after the scope, image object, session, runtime generation, or scene closes.
7. Raise `CudaInteropUnavailable` for unavailable platform/provider/device capability, `ValueError` for shape/dtype/format errors, and a focused runtime exception for bridge, semaphore, copy, registration, or render failures.
8. Do not silently fall back to a NumPy upload.
9. Defer region writes until row-pitch, dirty-region, and partial-copy proof exists.

### Validation

1. Pure Python shape, dtype, format, size, lifecycle, and error tests.
2. Fake-runtime tests for buffer registration, field binding, revision updates, and no-copy-when-clean behavior.
3. Clean editable-install and wheel tests.
4. Base import with no CUDA, CuPy, PyTorch, or Taichi installed.
5. macOS and non-NVIDIA execution must skip with the expected diagnostic.

### Commit

`interop: expose CUDA image buffers to CuPy`

### Delegation

A low-reasoning subagent may implement isolated argument-validation and unavailable-provider tests. A medium-reasoning subagent may implement fake-runtime lifecycle tests. The primary agent owns lower-layer generalization, synchronization, and exception boundaries.


## Checkpoint 4: Record Physical CuPy Image Evidence

### Objective

Prove the full Lane 1 chain on physical Linux/NVIDIA hardware.

### Proof

1. CuPy kernel writes a deterministic RGBA pattern into the shared buffer.
2. CUDA signals the external timeline semaphore on the selected stream.
3. Vulkan waits, copies the buffer into the persistent image, transitions it for sampled reads, and renders the ordinary image visual.
4. Offscreen readback verifies several exact pixels or a deterministic checksum.
5. Repeat at least 100 frames with changing data and no runtime-object growth.
6. Destroy and recreate the image-buffer resource and repeat.
7. Run with Vulkan validation enabled.

### Artifacts

1. Add `examples/python/features/cupy_image.py`.
2. Extend `tools/bindings/ctypes_cupy_smoke.py` with an image-buffer gate.
3. Record GPU, driver, Vulkan, CUDA, CuPy, and Datoviz commit information with the result.
4. Treat unavailable hardware as unavailable, not passed.

### Commit

`test: prove CuPy image-buffer rendering on Linux NVIDIA`

### Delegation

A low-reasoning subagent may prepare the deterministic pixel oracle and environment-reporting code. A medium-reasoning subagent may prepare the example and smoke harness. The primary agent must run or supervise the physical proof, review validation output, and decide whether the evidence is sufficient.


## Checkpoint 5: Add PyTorch And Taichi Image Writers

### Objective

Reuse Lane 1 with framework-native tensor views after CuPy physical proof is complete.

### Work

1. Add `torch_write()` using DLPack pointer identity and the selected PyTorch CUDA stream.
2. Require the tensor and stream to use the CUDA device matched to the Vulkan UUID.
3. Require contiguous `uint8[H,W,4]` layout.
4. Add `taichi_write()` over the borrowed PyTorch tensor.
5. Retain the conservative blocking Taichi handoff with `torch_stream.synchronize()` and `ti.sync()` until a compatible Taichi CUDA stream contract is proven.
6. Keep Taichi GGUI explicitly outside this contract.

### Validation

1. Adapter and lifecycle unit tests.
2. PyTorch physical non-default-stream render/readback proof.
3. Taichi CUDA-kernel render/readback proof with blocking behavior recorded.
4. Repeated frames and teardown for each framework.

### Commit

`interop: add PyTorch and Taichi image-buffer writers`

### Delegation

A low-reasoning subagent may extend the existing framework mocks. A medium-reasoning subagent may add hardware-gated smoke entry points. The primary agent owns stream-ordering review and physical evidence.


## Lane 2: Direct Shared Vulkan Image

## Checkpoint 6: Make Datoviz Images Exportable

### Objective

Create a narrow, capability-checked Datoviz-owned `VkImage` allocation that may be exported to CUDA without exposing private allocation internals.

### Work

1. Update `dvz_allocator_image()` in `src/vk/memory.c` to copy the caller's `VkImageCreateInfo` and append `VkExternalMemoryImageCreateInfo` when the allocator has an external-memory policy.
2. Preserve existing `pNext` chains and centralize external-image chaining so Canvas and video paths do not append duplicate external-image structures.
3. Audit and update manual external-image chains in `src/canvas/canvas.c`, `src/canvas/swapchain_sink.c`, and video tests.
4. Require a dedicated allocation in the initial contract.
5. Query `VkExternalImageFormatProperties` using `VkPhysicalDeviceExternalImageFormatInfo` and `VkPhysicalDeviceImageFormatInfo2`.
6. Require the opaque-FD handle type and exportable support; respect a dedicated-only result.
7. Query external timeline-semaphore export support separately.
8. Add `DvzInteropImageExport`, `DvzInteropImageExportConfig`, a version constant, and `dvz_interop_image_export_from_images()`.
9. Include allocation size, dedicated flag, format, extent, image type, mip levels, layers, samples, tiling, usage, aspect, device UUID, memory FD, semaphore FD/type/value, and the required CUDA ownership layout.
10. Reject wrapped or borrowed images, unsupported indices, zero extents, unsupported formats, non-dedicated allocations, multiple mip levels/layers/samples, and allocators without the exact export policy.
11. Transfer each exported FD exactly once and close it on every failure path where ownership remains local.

### Validation

1. Capability-query positive and negative tests.
2. Allocator `pNext` chain tests and regression tests for Canvas/video image creation.
3. Descriptor ABI, validation, index, format, ownership, and FD cleanup tests.
4. vklite image wrapper lifetime tests.
5. `just ctypes` and `just ctypes-check`.
6. Vulkan validation smoke.

### Commit

`interop: export Datoviz-owned Vulkan images`

### Delegation

A medium-reasoning subagent may implement capability-query helpers and negative tests in isolated files. Another medium-reasoning subagent may audit Canvas/video duplicate-chain call sites. The primary agent owns the allocator chain, public descriptor, FD ownership, and graphics-safety review.


## Checkpoint 7: Prove Native CUDA Array And Surface Access

### Objective

Prove Vulkan-owned image -> CUDA import -> CUDA surface write -> Vulkan readback before adding DRP2 or scene integration.

### Work

1. Extend the CUDA bridge with a distinct image-import handle rather than overloading the mapped-buffer handle.
2. Import the memory FD with `cudaExternalMemoryHandleTypeOpaqueFd` and `cudaExternalMemoryDedicated`.
3. Map with `cudaExternalMemoryGetMappedMipmappedArray()`, obtain level 0, and create a CUDA surface object.
4. Map `VK_FORMAT_R8G8B8A8_UNORM` to the exact unsigned 8-bit four-channel CUDA descriptor.
5. Import the timeline semaphore with the existing opaque-FD timeline path.
6. Add explicit Vulkan image release-to-CUDA and acquire-from-CUDA operations using known layouts and `VK_QUEUE_FAMILY_EXTERNAL`.
7. Use a monotonic ownership protocol such as Vulkan-ready `2n+1` and CUDA-ready `2n+2`.
8. Do not use semaphore ordering as a substitute for the release/acquire barriers.
9. Destroy in this order: stop submissions, synchronize the CUDA stream, destroy the surface, free the mapped mipmapped array without separately freeing its borrowed level array, destroy external semaphore and memory, close any still-owned FDs, then destroy Vulkan view/image/allocation/semaphore/context.

### Native Proof

Add `examples/c/advanced/cuda_external_image.c`:

1. Datoviz creates and exports the image and timeline semaphore.
2. CUDA matches the Vulkan device UUID.
3. A CUDA kernel writes a deterministic RGBA pattern through the surface.
4. CUDA signals the next timeline value.
5. Vulkan waits/acquires, transitions, copies the image to a readback buffer, and verifies pixels.
6. Several handoff cycles prove both ownership directions.

### Validation

1. No-CUDA bridge/import unit tests.
2. Linux/NVIDIA hardware gate with clean skips for unavailable capability.
3. Strict Vulkan validation.
4. Repeated handoff and teardown.
5. ASAN/UBSAN where supported.

### Commit

`interop: prove CUDA surface writes to exported Vulkan images`

### Delegation

A medium-reasoning subagent may implement bridge declarations and deterministic kernel/test scaffolding after the descriptor is fixed. A low-reasoning subagent may add CMake and example-manifest gates. The primary agent owns synchronization, queue-family transfers, CUDA destruction order, and physical validation.


## Checkpoint 8: Register External Sampled Textures In DRP2

### Objective

Let the existing vklite-backed DRP2 runtime borrow an externally updated Datoviz image under a normal DRP2 texture ID.

### Work

1. Add `DvzDrp2ExternalTextureDesc`, its default constructor, validation, and `dvz_drp2_runtime_register_external_texture()`.
2. Carry format, extent, usage, current layout/access contract, and borrowed image/view information required by the first 2D sampled slice.
3. Register a semantic `DRP2_OBJECT_TEXTURE` with normal format, size, usage, and access metadata.
4. Register matching vklite runtime state without issuing `CreateTexture`.
5. Prefer borrowing a supplied image view so the runtime creates and destroys neither image nor view.
6. Add explicit borrowed-image and borrowed-view flags to runtime objects and teardown.
7. Reject `CreateTexture`, `WriteTexture`, incompatible replacement, or stream destruction that would overwrite or destroy the registered object.
8. Reuse the normal bind-group, sampler, pipeline, draw, reset, and idle-wait paths.
9. Keep synchronization outside registration: registration describes a ready sampled resource; the producer/host performs the acquire before runtime execution.

### Validation

1. Semantic registration and duplicate/invalid descriptor tests.
2. Normal texture sampling and readback through a registered external image.
3. Reset and destroy without destroying borrowed image/view handles.
4. Re-register or replace after explicit runtime reset.
5. Dependent bind-group refresh.
6. Public-header binding regeneration and checks.

### Commit

`drp2: register borrowed external sampled textures`

### Delegation

A medium-reasoning subagent may implement semantic tests while another medium-reasoning subagent implements vklite ownership tests in disjoint files. The primary agent owns the descriptor, borrowed ownership policy, teardown, and final integration.


## Checkpoint 9: Bind Direct Shared Images Through The Scene

### Objective

Use the same external sampled-field semantics and canonical texture key from Lane 1 while replacing the buffer-to-texture copy with direct runtime texture registration.

### Work

1. Add a distinct sampled-field backing mode such as `EXTERNAL_RUNTIME_TEXTURE`.
2. Preserve field descriptor, semantic, color role, geometry, sampling, dirty/revision state, and canonical texture resource key.
3. Emit no CPU upload, no `CreateTexture`, no `WriteTexture`, and no buffer-to-texture copy.
4. Add a public resource-key helper for the external field/visual so the runtime session can resolve the emitted numeric texture ID without private visual-index knowledge.
5. Register the exported image under that exact ID before executing setup.
6. Continue through the normal image visual descriptor, sampler, bind group, draw, picking, and query boundaries.
7. Define image generation/recreation and descriptor-refresh behavior before allowing resize.
8. Keep raw CUDA surface access out of the generic scene API.

### Optional Python Surface

Do not promise a CuPy ndarray or PyTorch tensor for the direct image. If a Python API is justified after native proof, use a distinct surface-oriented object such as:

```python
with session.cuda_image(shape=(height, width), format="rgba8_unorm") as image:
    with image.cuda_surface_write(stream=stream) as surface:
        launch_surface_kernel(surface, width, height)
```

The exact CUDA surface representation should remain opaque unless CuPy offers a stable, ownership-safe surface wrapper that can be validated. PyTorch and Taichi should continue using Lane 1.

### Validation

1. External field emits the normal texture metadata and draw without create/write/copy commands.
2. Canonical label resolves to the registered texture ID.
3. CUDA surface writes render through the ordinary image visual.
4. Repeated frame handoffs, reset, destroy, and recreation are safe.
5. CPU query behavior follows the declared unavailable/readback contract.

### Commit

`scene: bind direct CUDA-shared images to sampled fields`

### Delegation

A medium-reasoning subagent may add emission-shape and resource-key tests. A low-reasoning subagent may add Python argument and unavailable-provider tests if the optional API is approved. The primary agent owns the public distinction between tensor-buffer and CUDA-surface objects.


## Checkpoint 10: Evidence, Documentation, And Promotion Decision

### Objective

Decide whether Lane 2 remains an internal native technique or becomes an experimental user-facing API.

### Required Evidence

1. At least 100 alternating CUDA/Vulkan ownership cycles.
2. Vulkan validation with no ownership, layout, lifetime, or descriptor errors.
3. Exact sampled-image render/readback.
4. Resource destroy/recreate proof.
5. Unsupported format and capability diagnostics.
6. Device UUID mismatch diagnostics.
7. Driver and toolkit matrix covering at least the intended hosted Linux/NVIDIA lane.
8. A performance comparison between Lane 1's GPU copy and Lane 2's direct image for representative image sizes.

### Documentation

1. Add a public CUDA image how-to only for the lane whose evidence is complete.
2. Extend the GPU-array interoperability matrix with separate tensor-buffer and direct-image rows.
3. Document memory ownership, borrowed lifetimes, copies, stream behavior, blocking Taichi behavior, supported formats, and platform exclusions.
4. Keep Lane 2 labeled research or advanced/native-only if no stable Python surface exists.

### Commits

Use separate commits for evidence fixtures and public documentation so evidence status can be reviewed independently from prose.

### Delegation

A low-reasoning subagent may update documentation matrices and example manifests from an approved evidence record. A medium-reasoning subagent may build the benchmark harness. The primary agent owns status classification and any promotion decision.


## Delegation Matrix

| Work packet | Reasoning | May run in parallel with | Must not decide |
| --- | --- | --- | --- |
| Source inventory, symbol lists, and file maps | Low | Any read-only audit | Architecture or ownership |
| Argument validation and unavailable-provider Python tests | Low | C/runtime implementation | Public semantics |
| Example manifest, CMake gate, and generated navigation updates | Low | Example implementation after IDs freeze | Platform support claims |
| Deterministic pixel oracle and environment report | Low | Hardware harness construction | Whether hardware evidence passes |
| DRP2 fixtures and negative semantic tests | Medium | Frame-plan implementation in disjoint files | Descriptor shape |
| Scene field-mode and upload-suppression tests | Medium | Runtime registration work | Ownership-mode semantics |
| CUDA bridge declarations and mock tests | Medium | Native Vulkan export work | Destruction and synchronization order |
| Capability-query helpers and negative tests | Medium | Descriptor/API work in disjoint files | Supported format policy |
| vklite borrowed-object teardown tests | Medium | DRP2 semantic tests | Borrowed ownership contract |
| Example and smoke harness implementation | Medium | Documentation drafting | Physical pass/fail classification |
| Performance benchmark harness | Medium | Final documentation | Promotion decision |
| Public C/Python API contracts | Primary agent | None until frozen | Not delegated |
| Vulkan layout and queue-family synchronization | Primary agent | Reviewed after subagent audit | Not delegated |
| FD, CUDA object, and Vulkan object ownership | Primary agent | Reviewed after subagent audit | Not delegated |
| Cross-checkpoint integration and commits | Primary agent | After child work lands | Not delegated |
| Physical validation interpretation and feature status | Primary agent | Hardware harness may be delegated | Not delegated |

Subagents must receive exact file boundaries. Do not assign concurrent writers to the same header, generated binding, runtime state file, or test registry. The primary agent integrates and commits only after inspecting the combined diff.


## Checkpoint Commit Discipline

1. One logical checkpoint per commit after its focused checks pass.
2. Public-header checkpoints include regenerated bindings in the same commit only when the generated output is expected and reviewed.
3. Physical-evidence commits must not claim unavailable machines as passes.
4. Do not combine Lane 1 product work and Lane 2 research-spike work in one commit.
5. Do not stage `data`, runtime libraries, generated binary payloads, or unrelated user changes.
6. Before each commit run `git diff --check`, inspect `git status --short`, and inspect `git diff --cached --stat`.
7. Use isolated staging if unrelated work exists in shared files.


## Validation Ladder

Every checkpoint runs the narrowest relevant subset, then the broader gate required by its touched boundary.

### Python-only changes

```sh
python3 -m pytest -q testing/test_experimental_cuda*.py
just ctypes-package-smoke
git diff --check
```

### Public headers or bindings

```sh
just ctypes
just ctypes-check
git diff --check
```

### DRP2 and scene

```sh
just test drp2
just test scene/frame-plan
just spec-check
git diff --check
```

### Vulkan, vklite, and synchronization

```sh
just build
direnv exec . just test vk
direnv exec . just test vklite
git diff --check
```

### Documentation

```sh
just docs-build-check
just docs-status-check
git diff --check
```

### Linux/NVIDIA evidence

Run the hardware-gated native C example, CuPy smoke, PyTorch smoke, and Taichi smoke with Vulkan validation enabled. Record skips as unavailable and include the exact GPU, driver, CUDA runtime/toolkit, framework versions, and commit SHA.


## Stop Conditions

Stop and revise the plan before continuing if:

1. External image format queries do not report the narrow RGBA8 profile as exportable.
2. CUDA requires an image representation incompatible with the selected Vulkan format, extent, usage, or dedicated policy.
3. Validation reports unresolved external queue-family ownership or layout errors.
4. Runtime external-texture registration would require a parallel renderer or backend-shaped scene handle.
5. PyTorch or Taichi would need to pretend an optimally tiled CUDA array is a linear tensor.
6. Resource recreation cannot invalidate outstanding CUDA imports deterministically.
7. Lane 1's GPU copy cost is material enough to justify changing its default contract before Lane 2 has proof.
8. Required physical Linux/NVIDIA hardware remains unavailable when a checkpoint's exit criterion requires it.


## Definition Of Done

Lane 1 is complete when:

1. CuPy writes a borrowed `uint8[H,W,4]` view backed by a Datoviz-owned exportable buffer.
2. No CPU pixel copy occurs.
3. Exactly one GPU buffer-to-image copy occurs after a completed write and none when unchanged.
4. The normal image visual renders and readback verifies the pixels.
5. Repeated frames, non-default streams, teardown, and recreation pass on physical Linux/NVIDIA hardware.
6. PyTorch and Taichi status accurately reflects their own physical evidence.
7. Clean packages import without CUDA dependencies.

Lane 2 is complete when:

1. Datoviz exports a capability-checked dedicated Vulkan image.
2. CUDA imports it as a mipmapped array and writes through a surface.
3. Explicit bidirectional ownership, layout, memory, and semaphore handoffs pass Vulkan validation.
4. DRP2 borrows the texture without destroying it and the scene binds it through the canonical sampled-field path.
5. Repeated frames, teardown, recreation, and physical render/readback pass.
6. The public surface does not misrepresent the resource as a general CuPy/PyTorch tensor.


## Approval Boundary

Approval of this plan should authorize Checkpoint 0 and Lane 1 only unless the user explicitly approves the direct shared-image Lane 2 research work. Lane 2 changes allocator image creation, cross-API image ownership, and DRP2 borrowed texture lifetime, so it should receive a separate go/no-go review after Lane 1 physical CuPy evidence and performance measurements are available.
