# GPU array interoperability

Status: the resource and synchronization model is experimental. Framework rows distinguish implementation from physical GPU evidence and are not a promise of stable public APIs.

## Contract

The current interop direction is Datoviz/Vulkan-owned buffers exported to an external CUDA consumer. Datoviz retains allocation and rendering ownership; the external framework receives a borrowed device-memory view and must participate in explicit CUDA/Vulkan synchronization.

The proven surface is Linux, NVIDIA CUDA, opaque-FD external memory, opaque-FD timeline semaphores, whole-buffer writes, and Vulkan vertex-buffer consumption. It is unrelated to the portable WebGPU backend and does not support arbitrary CUDA-owned allocation import as its primary workflow.

## Framework status

| Consumer | Status | Intended route | Notes |
| --- | --- | --- | --- |
| CuPy | implemented; physical smoke available | `datoviz.experimental.cuda` session buffer and `cupy_write()` | Wraps CUDA-mapped Datoviz memory and performs the current external-semaphore handoff. |
| PyTorch | adapter implemented; physical proof pending | `torch_write()` converts the CuPy view through DLPack and shares the selected PyTorch CUDA stream | Pointer identity, same-device selection, lifetime, and ordering have unit coverage; Linux/NVIDIA Vulkan render/readback remains required. |
| Taichi compute kernels | adapter implemented; physical proof pending | `taichi_write()` lends the zero-copy PyTorch tensor to a Taichi CUDA kernel | Uses blocking PyTorch-stream synchronization and `ti.sync()` because a compatible Taichi stream is not exposed; Linux/NVIDIA Vulkan render/readback remains required. |
| Numba CUDA | candidate | CUDA-array-interface or mapped-pointer adapter | Requires device, lifetime, and external-semaphore validation. |
| Taichi GGUI | separate renderer | Not a Datoviz GPU-array consumer | GGUI manages rendering of Taichi fields; direct Datoviz buffer sharing is not planned by this contract. |
| JAX | deferred | Potential future DLPack investigation | Mutation, ownership, and stream semantics are not yet suitable for this workflow. |
| TensorFlow | deferred | Potential future DLPack investigation | Mutation and lifetime semantics are not yet established for Datoviz render buffers. |

No row is a stable support guarantee. CuPy has the strongest evidence; PyTorch and Taichi remain experimental implementations until they prove CUDA/Vulkan device matching, real stream ordering, repeated frames, teardown, and a Vulkan render/readback result on physical Linux/NVIDIA hardware.

## Guide selection

- [Share Datoviz buffers with CUDA](../advanced/cuda-external-memory.md) covers the advanced C/Vulkan/CUDA external-object contract.
- [Update a Datoviz buffer from CUDA Python](../how-to/cupy-interop.md) covers the experimental CuPy, PyTorch, and Taichi workflows.
- [Compute and graphics](compute-graphics.md) covers Datoviz's distinct built-in scene-compute path.
