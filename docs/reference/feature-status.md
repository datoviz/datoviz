# Feature Status

Status labels use the meanings from [Project status](project-status.md). This page classifies
public-facing features; exact function signatures live in the generated C API reference. The v0.3
visible capability disposition is tracked in [v0.3 visible parity](v03-visible-parity.md).

| Area | Status | Reference | Notes |
| --- | --- | --- | --- |
| Native scene/app path | supported | [Objects and lifetimes](objects-and-lifetimes.md), [C API](c-api/index.md) | Core retained scene, visual, panel, figure, app, offscreen, and GLFW APIs are release-facing; optional platform/runtime availability and feature-specific gaps are classified separately. |
| Retained visual families | supported/experimental by family | [Visual families](visual-families/index.md) | Point, pixel, marker, segment, path, vector, primitive, image, text, labels, mesh, sphere, and volume are release-facing; glyph and splat are experimental. |
| Visual attributes and retained data updates | supported for documented dense paths; deferred otherwise | [Visual attributes](visual-attributes.md) | Dense per-item writes, atomic multi-attribute replacement, and range updates are active; broader constant/span/group source APIs remain deferred unless a family documents support. |
| Controllers | supported natively; experimental by browser route | [Controllers](controllers.md) | Panzoom and 3D controller APIs are active; WebGPU support follows the promoted live-route subset. |
| Queries, picking, probing, and readback | supported by documented native target/family; experimental by browser route | [Queries](queries.md) | Unified query API is active. Result richness varies; unsupported target, family, profile, or GPU format is reported explicitly. |
| DRP2/DVZR | advanced/unstable | [DRP2 command streams](../advanced/drp2-command-streams.md) | Advanced backend, fixture, packet, and replay surface for contributors and runtime authors; not the primary user API. |
| CPU-side user projection before upload | supported pattern | [Coordinate systems](coordinate-systems.md) | Users may pre-project nonlinear/geographic data on the CPU, then upload ordinary Cartesian positions. |
| Scene-managed nonlinear coordinate transforms | deferred | [Coordinate systems](coordinate-systems.md) | No v0.4 scene API for panel/domain projections or geographic transforms. |
| Custom visual/render shaders | deferred | [Visual attributes](visual-attributes.md) | Future work should arrive through custom visual families or descriptors, not built-in shader replacement. |
| Scene compute shaders | experimental | [Compute and graphics](compute-graphics.md) | Narrow compute-to-render interop path with DRP2 `ResourceBarrier`, WebGPU fixture coverage, and `examples/c/showcases/gpu_particle_smoke.c`; not a general compute framework. |
| CUDA external-memory buffers and image transfers | experimental; Linux/NVIDIA only | [GPU array interoperability](gpu-array-interop.md) | Native C and CuPy routes share Datoviz-owned Vulkan buffers through opaque FDs and timeline semaphores. RGBA8 images add one GPU-only buffer-to-texture copy. Direct shared Vulkan images are deferred; image, PyTorch, and Taichi routes await recorded physical GPU proof. |
| Raster video capture | supported, optional backend | [Export videos](../how-to/video-export.md) | The app-level CPU-readback path is release-facing; encoders are optional, while external-memory/NVENC integration is advanced and platform-specific. |
| Qt/PyQt hosted rendering | supported from source; no packaged provider in RC2 | [Platform support](platform-support.md) | Native Qt and PyQt hosting render into Qt-owned Vulkan surfaces without linking Qt into `libdatoviz`. RC2 has no packaged bridge provider; the wheels' `datoviz[qt]` extra installs PyQt6 but not `datoviz_qtbridge`. Packaging is planned for RC3. |
| Built-in shader replacement and hot reload | deferred | [Errors and logging](errors-and-logging.md) | Built-in visual shader ABI is internal in v0.4. |
| Python API | supported | [Python API](ctypes.md) | `import datoviz as dvz` adds documented NumPy adaptation; `datoviz.raw` exposes the same generated binding with exact exported `DVZ_EXPORT` ABI calls. |
| C FFI helper ABI | advanced/unstable | [FFI helper API](c-api/ffi.md) | `dvz_ffi_*` wrappers are pointer-oriented helpers for generated bindings and foreign runtimes; ordinary C/Python users should prefer the canonical `dvz_*` scene/app APIs. |
| WebGPU/WASM | experimental | [WebGPU subset](webgpu-subset.md) | Browser subset is documented and release-proofed for promoted examples; it is not Vulkan parity. |
| v0.3 visible parity | fixed/experimental/deferred/external by capability | [v0.3 visible parity](v03-visible-parity.md) | Visible v0.3-era behavior is classified as covered by v0.4, active experimental, intentionally deferred, or owned by GSP/VisPy2/Matplotlib. |
| Old Datoviz Python plotting API | external/GSP | [Project status](project-status.md) | Not part of v0.4 Datoviz docs. |
