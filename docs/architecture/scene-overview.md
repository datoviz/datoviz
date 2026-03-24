# Scene Overview

The future scene layer should be a pure high-level consumer of DRP2.

It should own user-facing visualization state and convert that state into a frame plan and DRP2
commands, without knowing how Vulkan or WebGPU implement those commands internally.


## Status

This document is only a short architectural overview.

The current source of truth for scene design is
[spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md) and the documents it
indexes.

Current branch status:

1. scene remains planning-only,
2. the scene spec is now materially broader than this overview alone,
3. implementation should still follow the DRP2/runtime boundary rather than the current low-level
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


## Sequencing

The correct order is:

1. finish stabilizing the current low-level layers enough to understand the durable backend boundary,
2. keep the DRP2 contract and runtime boundary as the source of truth for scene dependencies,
3. continue refining scene semantics in `spec/scene/`,
4. only then start bringing up the real scene implementation against the stabilized DRP2/runtime
   surface.

Trying to implement the scene layer before the DRP2 boundary is stable would very likely lock the
project into the wrong abstractions.
