# Project Status

Datoviz v0.4 is a release candidate. Public documentation uses explicit status labels so users can
distinguish the RC surface, experimental slices, contributor surfaces, deferred work, and external
ownership.

| Status | Meaning |
| --- | --- |
| supported | Intended to be part of the v0.4 release surface. |
| experimental | Available for early testing with known gaps. |
| advanced/unstable | Useful for contributors or backend authors, but not the main user path. |
| deferred | Not part of the v0.4 release surface. |
| external/GSP | Belongs to GSP, VisPy2, or another layer outside Datoviz v0.4. |

Status applies to the named public workflow, not automatically to every backend, visual family,
query target, optional provider, or gallery route. A `supported` native API can still have an
`experimental` WebGPU route or an unavailable optional dependency.

## Current Broad Status

| Area | Status | Reference |
| --- | --- | --- |
| Native C scene/app path | supported, with feature-specific gaps | [C API](c-api/index.md), [Feature status](feature-status.md) |
| Python API: NumPy-adapted and exact raw calls | supported | [Python API](ctypes.md) |
| Retained visual families | supported/experimental by family | [Visual families](visual-families/index.md) |
| Offscreen and GLFW app presentation | supported, subject to platform/runtime availability | [Platform support](platform-support.md) |
| Raster video capture through the app API | supported, optional encoder backend | [Export videos](../how-to/video-export.md) |
| Scene compute shaders | experimental | [Compute and graphics](compute-graphics.md) |
| Qt/PyQt hosted rendering | supported from source; no packaged provider in RC2, packaged provider planned for RC3 | [Platform support](platform-support.md) |
| CPU-side nonlinear/geographic pre-projection | supported pattern | [Coordinate systems](coordinate-systems.md) |
| Scene-managed nonlinear transforms | deferred | [Feature status](feature-status.md) |
| Custom visual/render shaders | deferred | [Visual attributes](visual-attributes.md) |
| Built-in shader replacement and hot reload | deferred | [Feature status](feature-status.md) |
| C FFI helper ABI | advanced/unstable | [FFI helper API](c-api/ffi.md) |
| WebGPU/WASM path | experimental | [WebGPU subset](webgpu-subset.md) |
| DRP2 command stream and fixtures | advanced/unstable | [DRP2 command streams](../advanced/drp2-command-streams.md) |
| v0.3 visible capability disposition | fixed/experimental/deferred/external by capability | [v0.3 visible parity](v03-visible-parity.md) |
| Old Datoviz Python plotting API | external/GSP | [Feature status](feature-status.md) |

See [Feature status](feature-status.md) for the detailed table.

DRP2 and lower runtime layers are documented for backend authors and contributors under
[Runtime internals](../advanced/runtime-internals.md). Most users should start with the scene/app API
or the Python NumPy-array interface.

See [Known limitations](known-limitations.md) for the cross-cutting release-candidate, runtime, provider, scene, and browser boundaries that apply alongside this status table.
