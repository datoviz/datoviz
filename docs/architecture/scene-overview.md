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

1. scene and app are active default-build modules with several retained implementation slices,
2. the current implementation covers scene/figure/panel lifecycle, point/primitive/mesh/path/image
   visuals, sampled fields, scale/image colormap binding, capability snapshots, diagnostics, frame
   plans, DRP2 emission, interaction bookkeeping, point/image request readbacks, and offscreen/GLFW
   app paths,
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
2. `src/scene/scene.c` for scene graph lifecycle, retained objects, and visual families,
3. `src/scene/frame_plan.c` for frame-plan construction and JSON/debug serialization,
4. `src/scene/converter.c` for frame-plan to DRP2 emission,
5. `src/scene/pick_probe.c` for the first point-pick/image-probe request execution path,
6. `src/app/app.c`, `src/app/status.c`, and `src/app/trace.c` for the small presentation layer,
7. focused tests in `src/scene/tests/` and `src/app/tests/test_app.c`,
8. C examples `visuals/point.c`, `visuals/point.c`, `visuals/primitive.c`, `visuals/mesh.c`,
   `visuals/mesh.c`, `visuals/path.c`, `visuals/image.c`, `visuals/image.c`, and
   `techniques/pick_hover.c`, plus `techniques/image_probe.c` for live image probes and
   `visuals/image.c` for live sampled-field subregion updates, and
   `techniques/multi_panel.c` and `techniques/linked_panels.c` for live multi-panel
   controller/routing and linked-panzoom smoke.
9. [manual_scene_smoke.md](manual_scene_smoke.md) for the current manual scene/app smoke matrix.


## Sequencing

The correct near-term order is:

1. harden the active retained scene path with manual app examples covering point/image request
   handling, multi-panel rendering, depth, resizing, and controller state,
2. keep the DRP2 contract and runtime boundary as the source of truth for scene dependencies,
3. use `visuals/mesh.c` as the native 3D pressure example for mesh/depth/arcball before adding
   broad scene features,
4. run a narrow WebGPU feasibility spike against the existing DRP2 subset before the visual surface
   becomes much larger,
5. continue refining scene semantics in `spec/scene/` as implementation reveals concrete needs.

Avoid broad scene API growth before each new visual family has tests, an example, and a clear DRP2
emission/runtime story.
