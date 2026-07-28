# GPU Array Interop

Status: experimental v0.4 implementation contract.

This note defines the implemented public direction for GPU array interop in the scene runtime. The resource and synchronization concepts are authoritative, but names under `datoviz.experimental` are not a stable compatibility promise.


## Goal

Let external GPU compute code update Datoviz scene resources without CPU round trips.

The first supported direction is:

```text
Datoviz-owned renderable GPU buffer -> external CUDA/CuPy view -> explicit sync -> scene visual
```

The reverse direction, importing arbitrary CUDA-owned pointers into Vulkan, is not the primary design target for v0.4 because it is less reliable across the current platform slice.


## Public Shape

The user-facing API is resource-oriented and keeps one interop context around all buffers used by a scene:

```python
from datoviz.experimental import cuda

with cuda.scene_session(scene, present=not offscreen) as session:
    positions = session.buffer(
        shape=(particle_count, 3),
        dtype="float32",
        usage=("vertex", "storage"),
    )

    positions.bind_attr(points, "position")

    with positions.cupy_write() as pos:
        pos[:, 0] = ...
        pos[:, 1] = ...
        pos[:, 2] = ...
```

Equivalent experimental scopes are available as `torch_write()` and `taichi_write()`. The PyTorch adapter uses DLPack without changing the pointer and keeps external-semaphore work on the selected PyTorch CUDA stream. The Taichi adapter lends that tensor to a Taichi CUDA kernel but blocks on stream synchronization and `ti.sync()` because Taichi does not expose a compatible stream through this contract.

The API exposes scene resources and typed array views, not Vulkan handles, file descriptors, semaphore handles, DRP2 buffer ids, or frame-plan setup details.


## Stable Concepts

1. Datoviz owns the GPU allocation and render lifetime.
2. The exported resource has explicit shape, dtype, stride, byte size, and usage.
3. Visual attributes bind to the resource through the scene resource model.
4. External writers use scoped write access. Entering the scope waits for the previous Datoviz use; leaving the scope signals external writes.
5. Datoviz waits for the external write before reading the resource in a frame.
6. Capability failures are reported as diagnostics or clear Python exceptions.


## Initial Python Scope

The first Python implementation lives under `datoviz.experimental.cuda` and supports only:

1. Linux;
2. NVIDIA CUDA;
3. CuPy arrays, with implemented experimental PyTorch and Taichi adapters;
4. float32 2D arrays;
5. Datoviz/Vulkan-owned buffers exported with opaque-FD external memory;
6. timeline-semaphore synchronization around whole-buffer writes.

This is enough for feature examples and smoke tests. It deliberately does not freeze a general DLPack, PyTorch, Numba, or cross-platform API. CuPy has physical smoke evidence; PyTorch and Taichi still require physical Linux/NVIDIA Vulkan render/readback validation.


## Non-Goals

1. Hide all synchronization behind implicit magic.
2. Treat `datoviz.experimental._cuda_runtime` or its CUDA bridge as public API.
3. Expose byte offsets, raw external handles, or DRP2 registration as the main user workflow.
4. Support CUDA-owned pointer import as the preferred path.
5. Promise v0.4 stable names for the experimental Python namespace.


## Open Design Work

1. Decide whether the final module belongs under `datoviz.gpu`, `datoviz.interop`, or a higher-level scene package once the Python scene API settles.
2. Promote non-CuPy adapters only after each adapter proves memory identity, same-device execution, stream ordering, Vulkan render/readback, repeated frames, and teardown.
3. Add partial-range write scopes when scene dirty ranges and runtime barriers can preserve that information end to end.
4. Decide how this API reports backend capabilities alongside WebGPU and non-CUDA native runtimes.
