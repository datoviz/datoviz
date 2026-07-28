# Update a Datoviz buffer from CUDA Python

Status: experimental. CuPy is the exercised Python route. PyTorch and Taichi adapters are implemented but still require physical Linux/NVIDIA render/readback validation; none of these APIs is part of the supported portable Python or WebGPU surface.

## What is shared

Datoviz owns the Vulkan allocation and creates a renderable scene buffer. CUDA imports that allocation through external memory, and CuPy receives a view of the mapped CUDA pointer. CuPy writes are therefore visible to Datoviz without a CPU round trip or a per-frame GPU copy.

Use the experimental session API rather than raw file descriptors, Vulkan handles, or the internal bridge module. The current public shape is `scene_session()`, `buffer()`, and `cupy_write()`.

```python
import datoviz.raw as dvz
from datoviz.experimental import cuda

scene = dvz.dvz_scene()
try:
    with cuda.scene_session(scene, present=True) as session:
        positions = session.buffer(
            shape=(particle_count, 3),
            dtype="float32",
            usage=("vertex", "storage"),
        )

        positions.bind_attr(points, "position")

        with positions.cupy_write() as array:
            array[:, 0] = x
            array[:, 1] = y
            array[:, 2] = z

        # Render through the normal Datoviz app/view path.
finally:
    dvz.dvz_scene_destroy(scene)
```

The complete experimental CuPy source example is `examples/python/features/cupy_particles.py`. It is not a normal gallery example because the required CUDA hardware path has separate validation and capture requirements.

## Requirements

- Linux with Vulkan opaque-FD external-memory and external timeline-semaphore support.
- An NVIDIA CUDA device that matches the Vulkan physical device.
- A CUDA toolkit/runtime usable by the optional Datoviz bridge and a compatible CuPy installation.
- A Datoviz build with the CUDA interop path available.
- A two-dimensional `float32` buffer shape and `vertex` plus `storage` usage for the currently proven rendering route.

If any requirement is unavailable, the experimental module raises `CudaInteropUnavailable`. Do not silently replace this workflow with a copied NumPy upload when measuring or relying on zero-copy behavior.

## Lifetime and ordering

Create buffers inside a `scene_session()` and use them only while that session remains open. The CuPy array yielded by `cupy_write()` is borrowed; do not retain or use it after its write scope, buffer, or session has ended.

Put writes inside `cupy_write()`. Entering the scope orders CUDA after prior Datoviz use, and leaving it signals the external timeline semaphore so Vulkan can wait before vertex consumption. Memory aliasing alone does not make work submitted on different CUDA or Vulkan streams safe.

The current contract is whole-buffer writes followed by vertex reads. It does not promise partial-range tracking, generic storage or index consumption, textures/images, arbitrary CuPy allocation import, or cross-platform behavior.

## PyTorch

`torch_write()` converts the borrowed CuPy view with DLPack and verifies that PyTorch received the same CUDA pointer. It uses PyTorch's current CUDA stream by default, or an explicit `torch.cuda.Stream`, for both framework work and the external-semaphore wait/signal.

```python
with positions.torch_write() as tensor:
    tensor[:, 0] = x
    tensor[:, 1] = y
    tensor[:, 2] = z
```

The tensor is borrowed under the same lifetime rules as the CuPy array. The PyTorch stream must be on the CUDA device matched to Vulkan. The adapter and its ordering contract have local unit coverage, but a physical Linux/NVIDIA Vulkan render/readback run has not yet been recorded.

## Taichi compute

`taichi_write()` yields the same zero-copy PyTorch tensor for use with a Taichi CUDA kernel. Taichi does not expose its internal CUDA stream through this adapter, so entry synchronizes the selected PyTorch stream and exit calls `ti.sync()` before Vulkan is signaled. This keeps memory zero-copy but makes the handoff CPU-blocking.

```python
with positions.taichi_write() as tensor:
    update_positions(tensor)
```

This adapter requires both PyTorch and Taichi, and it still needs physical Linux/NVIDIA render/readback validation. Use it only when the blocking synchronization is acceptable.

Taichi GGUI is a separate renderer, not an array adapter for Datoviz. A Taichi field rendered by GGUI is managed by Taichi's presentation path; this contract instead lets a Taichi CUDA kernel update a Datoviz-owned buffer that Datoviz renders.

## Choosing this path

Use ordinary NumPy uploads through the supported Python binding when data originates on the CPU or when portability matters. Use this path when a CUDA/CuPy workload already owns the update step and the same buffer should be rendered immediately by Datoviz.

For the low-level C/CUDA contract, including exported-handle ownership and CUDA import, see [Share Datoviz buffers with CUDA](../advanced/cuda-external-memory.md). For the complete framework matrix, see [GPU array interoperability](../reference/gpu-array-interop.md).
