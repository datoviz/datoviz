# Feature Status

Status labels use the meanings from [Project status](project-status.md). This page classifies
public-facing features; exact function signatures live in the generated C API reference. The v0.3
visible capability disposition is tracked in [v0.3 visible parity](v03-visible-parity.md).

| Area | Status | Reference | Notes |
| --- | --- | --- | --- |
| Native scene/app path | supported/experimental | [Objects and lifetimes](objects-and-lifetimes.md), [C API](c-api/index.md) | Core retained scene, visual, panel, figure, app, offscreen, and GLFW paths are active, with feature-specific gaps tracked below and in examples. |
| Retained visual families | supported/experimental by family | [Visual families](visual-families/index.md) | Point, pixel, marker, segment, path, vector, primitive, image, text, labels, mesh, sphere, and volume are release-facing; glyph and splat are experimental. |
| Visual attributes and retained data updates | supported/experimental by attribute path | [Visual attributes](visual-attributes.md) | Dense per-item writes and range updates are active; broader constant/span/group source APIs remain directional unless a family documents support. |
| Controllers | supported/experimental by controller/backend | [Controllers](controllers.md) | Panzoom and 3D controller examples are active; WebGPU support follows the promoted live-route subset. |
| Queries, picking, probing, and readback | supported/experimental by target/backend | [Queries](queries.md) | Unified query API is active. Result richness and family/backend parity vary; unsupported capabilities must be explicit. |
| DRP2/DVZR | advanced/unstable | [DRP2](drp2/index.md) | Backend, fixture, packet, and replay surface for contributors and runtime authors. |
| CPU-side user projection before upload | supported pattern | [Coordinate systems](coordinate-systems.md) | Users may pre-project nonlinear/geographic data on the CPU, then upload ordinary Cartesian positions. |
| Scene-managed nonlinear coordinate transforms | deferred | [Coordinate systems](coordinate-systems.md) | No v0.4 scene API for panel/domain projections or geographic transforms. |
| Custom visual/render shaders | deferred | [Visual attributes](visual-attributes.md) | Future work should arrive through custom visual families or descriptors, not built-in shader replacement. |
| Scene compute shaders | experimental | [Compute and graphics](compute-graphics.md) | Narrow compute-to-render interop path with DRP2 `ResourceBarrier`, WebGPU fixture coverage, and `examples/c/showcases/gpu_particle_smoke.c`; not a general compute framework. |
| Qt/PyQt hosted rendering | supported, optional provider | [Platform support](platform-support.md) | Native Qt and PyQt hosting render into Qt-owned Vulkan surfaces without linking Qt into `libdatoviz`; PyQt requires the optional bridge and compatible Vulkan binding surface. |
| Built-in shader replacement and hot reload | deferred | [Errors and logging](errors-and-logging.md) | Built-in visual shader ABI is internal in v0.4. |
| Python raw `ctypes` binding | supported | [Python raw ctypes](ctypes.md) | Low-level Python binding path for exact C-shaped access. |
| WebGPU/WASM | experimental | [WebGPU subset](webgpu-subset.md) | Browser subset is documented and release-proofed for promoted examples; it is not Vulkan parity. |
| v0.3 visible parity | fixed/experimental/deferred/external by capability | [v0.3 visible parity](v03-visible-parity.md) | Visible v0.3-era behavior is classified as covered by v0.4, active experimental, intentionally deferred, or owned by GSP/VisPy2/Matplotlib. |
| Old Datoviz Python plotting API | external/GSP | [Project status](project-status.md) | Not part of v0.4 Datoviz docs. |
