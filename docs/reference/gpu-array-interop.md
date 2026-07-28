# GPU array interoperability

Status: the resource and synchronization model is experimental. Framework rows distinguish implementation from physical GPU evidence and are not a promise of stable public APIs.

## Contract

The current interop direction is Datoviz/Vulkan-owned buffers exported to an external CUDA consumer. Datoviz retains allocation and rendering ownership; the external framework receives a borrowed device-memory view and must participate in explicit CUDA/Vulkan synchronization. Vertex buffers are consumed directly. Tensor-shaped image buffers are copied on the GPU into a normal retained Datoviz texture before sampling.

The implemented surface is Linux, NVIDIA CUDA, opaque-FD external memory, opaque-FD timeline semaphores, and whole-buffer writes. Physical evidence currently covers Vulkan vertex-buffer consumption. The RGBA8 image transfer path has unit/native validation and a hardware-gated smoke but still awaits a recorded Linux/NVIDIA run. This work is unrelated to the portable WebGPU backend and does not support arbitrary CUDA-owned allocation import as its primary workflow.

## Framework status

| Consumer | Status | Intended route | Notes |
| --- | --- | --- | --- |
| CuPy | vertex buffers implemented and physically exercised; images implemented with physical proof pending | `session.buffer()` or `session.image_buffer()` with `cupy_write()` | Images use a linear RGBA8 tensor plus a GPU-only buffer-to-texture copy; they are not direct shared Vulkan images. |
| PyTorch | vertex and image adapters implemented; physical image proof pending | `torch_write()` converts the CuPy view through DLPack and shares the selected PyTorch CUDA stream | Pointer identity, same-device selection, lifetime, and ordering have unit coverage; Linux/NVIDIA image render/readback remains required. |
| Taichi compute kernels | vertex and image adapters implemented; physical image proof pending | `taichi_write()` lends the zero-copy PyTorch tensor to a Taichi CUDA kernel | Uses blocking PyTorch-stream synchronization and `ti.sync()` because a compatible Taichi stream is not exposed; Linux/NVIDIA image render/readback remains required. |
| Numba CUDA | candidate | CUDA-array-interface or mapped-pointer adapter | Requires device, lifetime, and external-semaphore validation. |
| Taichi GGUI | separate renderer | Not a Datoviz GPU-array consumer | GGUI manages rendering of Taichi fields; direct Datoviz buffer sharing is not planned by this contract. |
| JAX | deferred | Potential future DLPack investigation | Mutation, ownership, and stream semantics are not yet suitable for this workflow. |
| TensorFlow | deferred | Potential future DLPack investigation | Mutation and lifetime semantics are not yet established for Datoviz render buffers. |

No row is a stable support guarantee. Direct shared `VkImage` import/sampling is a separate future lane because an optimally tiled Vulkan image cannot be exposed as an ordinary CuPy or PyTorch tensor. CuPy has the strongest buffer evidence; every image/framework combination remains experimental until it proves CUDA/Vulkan device matching, real stream ordering, repeated frames, teardown, and Vulkan render/readback on physical Linux/NVIDIA hardware.

## Guide selection

- [Share Datoviz buffers with CUDA](../advanced/cuda-external-memory.md) covers the advanced C/Vulkan/CUDA external-object contract.
- [Update Datoviz buffers and images from CUDA Python](../how-to/cupy-interop.md) covers the experimental CuPy, PyTorch, and Taichi workflows.
- [Compute and graphics](compute-graphics.md) covers Datoviz's distinct built-in scene-compute path.
