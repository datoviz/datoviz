# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** give future agents the practical next steps after the first scene -> DRP2 ->
>   vklite/canvas slice.


## Current Position

The low-level stack is the foundation:

1. `vk` owns low-level Vulkan instance/device/queue/memory primitives.
2. `vklite` owns higher-level Vulkan wrappers.
3. `canvas` owns frame acquisition, borrowed frame command buffers, swapchain/offscreen targets,
   and stream submission.
4. `stream` and sinks route frames to swapchain, offscreen, live image, and video consumers.

The active higher layer exists:

1. `drp2` owns backend-agnostic command streams, JSON/debug serialization, validation, and the
   native vklite runtime.
2. `scene` owns early scene graph objects, capability snapshots, diagnostic reports, frame plans,
   DRP2 emission, and a minimal app/offscreen path.
3. Built-in visual families currently implemented are `point`, `primitive`, and `image`.
4. Panel controllers are live: panzoom and arcball feed per-panel transforms.
5. Per-panel viewport/scissor and offscreen multi-panel preservation work through the emitted DRP2
   path.

Focused validation recorded on `2026-05-06`:

1. `just spec-check`: `119/119` DRP2 fixtures passed; `52` fixture-runner tests passed.
2. `just test drp2`: `73/73` tests passed.
3. `just test scene`: `52/52` tests passed.
4. `git diff --check`: passed for the last documentation-only refresh.


## Immediate Task

The next work should draft concrete public scene headers rather than add more narrative design
notes.

Read in this order:

1. [../../spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md)
2. [../../spec/scene/api/API_SURFACE.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/api/API_SURFACE.md)
3. [../../spec/scene/decisions/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/README.md)
4. [SCENE_PUBLIC_API_HEADER_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PUBLIC_API_HEADER_PLAN.md)

Deliver the next public API draft in this order:

1. Draft interaction objects and selection/link/probe result types.
2. Draft scale, colormap, and colorbar handles/descriptors.
3. Draft text, font, annotation, and pinned readout handles/descriptors.
4. Add one implementation-facing scratch header or update
   [../../spec/scene/headers/scene_api.h](/home/cyrille/GIT/Viz/datoviz/spec/scene/headers/scene_api.h).
5. Write three tiny API-shape examples before implementation:
   mesh face/object selection with a link channel, image probe with pinned readout, and scale +
   colormap + colorbar + annotation label.


## Scope Guardrails

For the immediate API pass:

1. Do not implement runtime behavior yet.
2. Do not broaden visual-family work beyond what the header examples need.
3. Keep retained scene objects opaque.
4. Keep request descriptors and result payloads public and value-like.
5. Keep scene semantics in `spec/scene/`, not in new `agents/now/` design notes.


## Roadmap After The Header Pass

After the public API surface has one review pass, proceed in this order unless the user redirects:

1. Interaction core: pick requests, hover state, selection objects, link channels, probe result
   plumbing, and pinned readout state.
2. Scale/colormap/colorbar core: scene-owned scale and colormap objects, panel-attached colorbars,
   formatting descriptors, and tests.
3. Text/annotation retained objects: font handles, text style/placement descriptors, labels,
   scale bars, dimensions, and pinned readouts.
4. Native 3D baseline: minimal mesh visual, depth attachment wiring, viewport UBO use, arcball
   validation, and mesh examples.
5. Browser/WebGPU feasibility: replay a narrow DRP2 subset for point, primitive, image, and minimal
   mesh/depth scenes.
6. Transparency architecture: explicit WBOIT-style scene mode through frame plan, DRP2, runtime, and
   capability fallback.
7. Broader figure features: axes, lines/segments, richer annotations, picking refinements, and
   additional visual families.


## Validation Defaults

For documentation-only passes:

1. run `git diff --check`,
2. inspect `git status --short`,
3. do not run the graphics suite unless code or generated fixtures changed.

For scene/DRP2 code changes:

1. run `just build`,
2. run the narrowest relevant `just test <filter>`,
3. use Vulkan validation smoke tests for changes touching `vk`, `vklite`, `canvas`, `scene`,
   `drp2`, command buffers, frame lifetimes, render targets, swapchains, or synchronization.


## Completed Context

Completed implementation records:

1. [../done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
2. [../done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
3. [../done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)

Backlog context:

1. [../later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
2. [../later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
