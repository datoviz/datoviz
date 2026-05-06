# Scene Overview

The scene layer should remain a pure high-level consumer of DRP2.

It should own user-facing visualization state and convert that state into a frame plan and DRP2
commands, without knowing how Vulkan or WebGPU implement those commands internally.


## Status

This document is only a short architectural overview.

The current source of truth for scene design is
[spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md) and the documents it
indexes.

Current branch status:

1. scene is now an active default-build module with a first implementation slice,
2. the current implementation covers scene/figure/panel/point visual basics, capability snapshots,
   diagnostics, frame plans, DRP2 emission, and an app/offscreen path,
3. implementation should continue to follow the DRP2/runtime boundary rather than the low-level
   Vulkan stack directly.


## What Scene Is

Scene should own:

1. panels and layout,
2. cameras and controllers,
3. visuals and materials,
4. CPU-side resources and dirty tracking,
5. picking interpretation,
6. annotations, legends, and colorbars,
7. validation and capability adaptation policy,
8. animations,
9. frame planning.


## What Scene Is Not

Scene should not own:

1. Vulkan handles,
2. swapchain logic,
3. memory allocation internals,
4. command-buffer recording internals,
5. backend synchronization primitives.


## Current Shape

The current scene spec now covers at least:

1. object model,
2. visual contracts and visual families,
3. scene resource model,
4. transform pipeline,
5. frame-plan structure and lifecycle,
6. invalidation and caching,
7. controllers and picking,
8. annotations,
9. legends and colorbars,
10. scene validation,
11. capability adaptation,
12. worked examples.

This means the scene layer is no longer just:

1. panels plus visuals plus a render graph sketch.

It is now a broader producer-side semantic layer with explicit planning, validation, and fallback
policy.

In source, the current implementation is intentionally much smaller than the full spec. It includes:

1. `include/datoviz/scene.h` and `include/datoviz/scene/*` public draft headers,
2. `src/scene/scene.c` for scene graph lifecycle and the point visual,
3. `src/scene/frame_plan.c` for frame-plan construction and JSON/debug serialization,
4. `src/scene/converter.c` for frame-plan to DRP2 emission,
5. `src/scene/app.c` for the early scene/app/offscreen convenience path,
6. focused tests in `src/scene/tests/test_scene.c`,
7. C examples `hello_point.c` and `hello_scatter.c`.


## Sequencing

The correct near-term order is:

1. harden the existing point-only scene path across repeated updates, multiple panels, multiple visuals,
   diagnostics, and offscreen capture,
2. keep the DRP2 contract and runtime boundary as the source of truth for scene dependencies,
3. add the next minimal visual family, likely triangle/mesh, with tests and a C example,
4. add a minimal image/texture visual after mesh to pressure-test texture upload, samplers, views, and
   bind groups,
5. continue refining scene semantics in `spec/scene/` as implementation reveals concrete needs.

Avoid broad scene API growth before each new visual family has tests, an example, and a clear DRP2
emission/runtime story.
