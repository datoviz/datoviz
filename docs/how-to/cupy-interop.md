# Update Datoviz buffers and images from CUDA Python

Status: experimental and Linux/NVIDIA-only. The CuPy vertex-buffer route has physical render/readback evidence. The image-buffer route and the PyTorch and Taichi adapters are implemented but still require physical Linux/NVIDIA image evidence; none of these APIs is part of the supported portable Python or WebGPU surface.

## What is shared

Datoviz owns the Vulkan allocation. CUDA imports it through external memory, and CuPy receives a borrowed view of the mapped CUDA pointer. There are two distinct consumers:

- `session.buffer()` exposes a float scene buffer that Datoviz can read directly as vertex data after the semaphore handoff. This path has no CPU round trip and no per-frame payload copy.
- `session.image_buffer()` exposes a tightly packed `uint8[H,W,4]` pixel buffer. After CUDA writes, the normal Datoviz frame path performs one GPU-only buffer-to-texture copy and samples the resulting retained image.

Use the experimental session API rather than raw file descriptors, Vulkan handles, or the internal bridge module.

## Share a vertex buffer

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

The complete experimental vertex-buffer example is `examples/python/features/cupy_particles.py`.

## Share an image

Create a normal Datoviz image visual, then bind a distinct CUDA image buffer to its sampled-field slot. Complete the first CUDA write before requesting app resources; uninitialized external pixels are rejected.

```python
import datoviz.raw as dvz
from datoviz.experimental import cuda

scene = dvz.dvz_scene()
try:
    with cuda.scene_session(scene) as session:
        frame = session.image_buffer(
            shape=(height, width, 4),
            dtype="uint8",
            format="rgba8_unorm",
        )
        frame.bind_field(image_visual, "field")

        with frame.cupy_write() as pixels:
            pixels[...] = rgba

        app, view = session.offscreen_app(figure, 640, 480)
        try:
            dvz.dvz_view_render_once(view)
        finally:
            dvz.dvz_app_destroy(app)
finally:
    dvz.dvz_scene_destroy(scene)
```

Keep the app inside the session lifetime: destroy the app before leaving `scene_session()`, then destroy the scene. The complete source example is `examples/python/features/cupy_image.py`.

## Requirements

- Linux with Vulkan opaque-FD external-memory and external timeline-semaphore support.
- An NVIDIA CUDA device that matches the Vulkan physical device.
- A CUDA toolkit/runtime usable by the optional Datoviz bridge and a compatible CuPy installation.
- A Datoviz build with the CUDA interop path available.
- For vertex buffers, a two-dimensional `float32` shape and declared scene/runtime usage compatible with the visual consumer.
- For images, a tightly packed positive `(height, width, 4)` shape, `uint8`, and `rgba8_unorm`.

If any requirement is unavailable, the experimental module raises `CudaInteropUnavailable`. Do not silently replace this workflow with a copied NumPy upload when measuring or relying on zero-copy behavior.

## Lifetime and ordering

Create buffers inside a `scene_session()` and use them only while that session remains open. The CuPy array yielded by `cupy_write()` is borrowed; do not retain or use it after its write scope, buffer, or session has ended.

Put writes inside `cupy_write()`. Entering the scope orders CUDA after prior Datoviz use, and leaving it signals the external timeline semaphore. Vertex buffers use the ordinary Vulkan consumer wait. Image buffers invalidate the retained field and arm a one-shot wait/copy/release/signal handoff for the next Datoviz image transfer. Memory aliasing alone does not make work submitted on different CUDA or Vulkan streams safe.

The image handoff is intentionally single-flight: a second image write is rejected until Datoviz consumes the pending transfer. The current contract does not promise partial-range image writes, padded row pitch, formats other than RGBA8_UNORM, direct shared `VkImage` sampling, arbitrary CuPy allocation import, or cross-platform behavior.

Run the physical image gate on a compatible Linux/NVIDIA machine:

```sh
python tools/bindings/ctypes_cupy_smoke.py --image
```

An unavailable platform, CUDA runtime, matching device, or external-memory capability is a skip, not evidence that image interop passed.

## PyTorch

`torch_write()` is available on both vertex and image buffers. It converts the borrowed CuPy view with DLPack and verifies that PyTorch received the same CUDA pointer. It uses PyTorch's current CUDA stream by default, or an explicit `torch.cuda.Stream`, for both framework work and the external-semaphore wait/signal.

```python
with positions.torch_write() as tensor:
    tensor[:, 0] = x
    tensor[:, 1] = y
    tensor[:, 2] = z
```

The tensor is borrowed under the same lifetime rules as the CuPy array. The PyTorch stream must be on the CUDA device matched to Vulkan. The adapter and its ordering contract have local unit coverage, but physical Linux/NVIDIA image render/readback has not yet been recorded.

## Taichi compute

`taichi_write()` is available on both vertex and image buffers and yields the same zero-copy PyTorch tensor for use with a Taichi CUDA kernel. Taichi does not expose its internal CUDA stream through this adapter, so entry synchronizes the selected PyTorch stream and exit calls `ti.sync()` before Vulkan is signaled. This keeps the shared allocation zero-copy but makes the handoff CPU-blocking; the image route still performs its normal Vulkan buffer-to-texture copy.

```python
with positions.taichi_write() as tensor:
    update_positions(tensor)
```

This adapter requires both PyTorch and Taichi, and it still needs physical Linux/NVIDIA image render/readback validation. Use it only when the blocking synchronization is acceptable.

Taichi GGUI is a separate renderer, not an array adapter for Datoviz. A Taichi field rendered by GGUI is managed by Taichi's presentation path; this contract instead lets a Taichi CUDA kernel update a Datoviz-owned buffer that Datoviz renders.

## Choosing this path

Use ordinary NumPy uploads through the supported Python binding when data originates on the CPU or when portability matters. Use this path when a CUDA/CuPy workload already owns the update step and the same buffer should be rendered immediately by Datoviz.

For the low-level C/CUDA contract, including exported-handle ownership and CUDA import, see [Share Datoviz buffers with CUDA](../advanced/cuda-external-memory.md). For the complete framework matrix, see [GPU array interoperability](../reference/gpu-array-interop.md).
