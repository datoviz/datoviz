# GPU Array Interop

Status: experimental v0.4 design target.

This note defines the intended public direction for GPU array interop in the scene runtime. It is
not a promise that the first Python module names are stable.


## Goal

Let external GPU compute code update Datoviz scene resources without CPU round trips.

The first supported direction is:

```text
Datoviz-owned renderable GPU buffer -> external CUDA/CuPy view -> explicit sync -> scene visual
```

The reverse direction, importing arbitrary CUDA-owned pointers into Vulkan, is not the primary
design target for v0.4 because it is less reliable across the current platform slice.


## Public Shape

The user-facing API should be resource-oriented:

```python
from datoviz.experimental import cuda

positions = cuda.scene_array(
    scene,
    shape=(particle_count, 3),
    dtype="float32",
    usage=("vertex", "storage"),
    present=not offscreen,
)

positions.bind_attr(points, "position")

with positions.write_cupy() as pos:
    pos[:, 0] = ...
    pos[:, 1] = ...
    pos[:, 2] = ...
```

The API exposes scene resources and typed array views, not Vulkan handles, file descriptors,
semaphore handles, DRP2 buffer ids, or frame-plan setup details.


## Stable Concepts

1. Datoviz owns the GPU allocation and render lifetime.
2. The exported resource has explicit shape, dtype, stride, byte size, and usage.
3. Visual attributes bind to the resource through the scene resource model.
4. External writers use scoped write access. Entering the scope waits for the previous Datoviz use;
   leaving the scope signals external writes.
5. Datoviz waits for the external write before reading the resource in a frame.
6. Capability failures are reported as diagnostics or clear Python exceptions.


## Initial Python Scope

The first Python implementation may live under `datoviz.experimental.cuda` and support only:

1. Linux;
2. NVIDIA CUDA;
3. CuPy arrays;
4. float32 2D arrays;
5. Datoviz/Vulkan-owned buffers exported with opaque-FD external memory;
6. timeline-semaphore synchronization around whole-buffer writes.

This is enough for feature examples and smoke tests. It deliberately does not freeze a general
DLPack, PyTorch, Numba, or cross-platform API.


## Non-Goals

1. Hide all synchronization behind implicit magic.
2. Treat `tools/bindings/cupy_interop_runtime.py` or `SharedSceneCudaArray` as public API.
3. Expose byte offsets, raw external handles, or DRP2 registration as the main user workflow.
4. Support CUDA-owned pointer import as the preferred path.
5. Promise v0.4 stable names for the experimental Python namespace.


## Open Design Work

1. Decide whether the final module belongs under `datoviz.gpu`, `datoviz.interop`, or a higher-level
   scene package once the Python scene API settles.
2. Add non-CuPy adapters only after the resource/sync contract survives the first gallery examples.
3. Add partial-range write scopes when scene dirty ranges and runtime barriers can preserve that
   information end to end.
4. Decide how this API reports backend capabilities alongside WebGPU and non-CUDA native runtimes.
