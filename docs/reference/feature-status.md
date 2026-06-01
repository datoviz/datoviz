# Feature Status

Draft v0.4 public status table. Status labels use the meanings from
[Project status](../start/project-status.md).

| Area | Status | Notes |
| --- | --- | --- |
| Native scene/app path | supported/experimental | Feature-specific status still being inventoried. |
| Retained visual families | supported/experimental | See visual family reference. |
| DRP2/DVZR | advanced/unstable | Backend and replay surface for contributors. |
| CPU-side user projection before upload | supported pattern | Users may pre-project nonlinear/geographic data on the CPU, then upload ordinary Cartesian positions. |
| Scene-managed nonlinear coordinate transforms | deferred | No v0.4 scene API for panel/domain projections or geographic transforms. |
| Custom visual/render shaders | deferred | Future work should arrive through custom visual families, not built-in shader replacement. |
| Scene compute shaders | advanced/unstable | Narrow compute-to-render interop path; not the general custom render-shader API. |
| Built-in shader replacement and hot reload | deferred | Built-in visual shader ABI is internal in v0.4. |
| Raw `ctypes` | experimental | Low-level binding path only. |
| WebGPU/WASM | experimental | Supported subset and diagnostics pending release proof. |
| Old Datoviz Python plotting API | external/GSP | Not part of v0.4 Datoviz docs. |
