# Project Status

Datoviz v0.4 is under active development. The current release surface is being classified with
explicit status labels:

| Status | Meaning |
| --- | --- |
| supported | Intended to be part of the v0.4 release surface. |
| experimental | Available for early testing with known gaps. |
| advanced/unstable | Useful for contributors or backend authors, but not the main user path. |
| deferred | Not part of the v0.4 release surface. |
| external/GSP | Belongs to GSP, VisPy2, or another layer outside Datoviz v0.4. |


## Current Broad Status

| Area | Status |
| --- | --- |
| Native C scene/app path | supported, with feature-specific gaps |
| Retained visual families | supported/experimental by visual |
| Offscreen and GLFW app presentation | supported/experimental |
| DRP2 command stream and fixtures | advanced/unstable |
| Scene compute shaders | experimental |
| Qt/PyQt hosted rendering | supported, optional provider |
| CPU-side nonlinear/geographic pre-projection | supported pattern |
| Scene-managed nonlinear transforms | deferred |
| Custom visual/render shaders | deferred |
| Built-in shader replacement and hot reload | deferred |
| Python raw `ctypes` binding | supported |
| WebGPU/WASM path | experimental |
| Old Datoviz Python plotting API | external/GSP |

See [Feature status](../reference/feature-status.md) for the detailed table.
