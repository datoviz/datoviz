# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-12`
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
3. Built-in visual families currently implemented are `point`, `primitive`, `mesh`, path-as-line/strip,
   and `image`.
4. Panel controllers are live: panzoom and arcball feed per-panel transforms.
5. Per-panel viewport/scissor and offscreen multi-panel preservation work through the emitted DRP2
   path.
6. Retained sampled fields, scales, scene buffers, and primitive/mesh shading uniforms now share the
   scene -> frame-plan -> DRP2 binding path.
7. Public headers now include first-draft interaction/text/annotation APIs, but those groups are not
   implemented in `src/scene` yet.

Focused validation recorded on `2026-05-12`:

1. `just spec-check`: last recorded pass remained `119/119` DRP2 fixtures; `52` fixture-runner
   tests passed.
2. `just test drp2`: last recorded pass remained `73/73`.
3. `just test scene`: passed `127/127` after finishing the current request-resolution cleanup
   pass. Point pick and image probe now resolve through GPU-backed auxiliary DRP2/readback
   execution only; request freshness is explicit and persistent per panel/request-kind scope, and
   image probes now use the same explicit recentering rule as point picking.
4. `git diff --check`: passed on the latest scene slices.


## Immediate Task

The next work should stay implementation-focused and build on the current retained scene/runtime
path.

Read in this order:

1. this file for current ordering,
2. [../../spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md) for scene
   semantics,
3. [../done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
   for the active vertical-slice history,
4. [../done/SCENE_PICK_PROBE_EXECUTION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION.md)
   for the current shipped request-resolution behavior and caveats,
5. [SCENE_PICK_PROBE_EXECUTION_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PICK_PROBE_EXECUTION_PLAN.md)
   for the original planned shape,
6. the current `scene` and `drp2` tests before broadening any API.

Deliver the next implementation slices in this order:

1. Text and annotation retained-object bookkeeping for the already-declared `text.h` and
   `annotation.h` APIs, initially without glyph rendering if necessary.
2. Rendered colorbar/text/annotation realization, reusing the current scene -> DRP2 path.
3. Depth attachment wiring and validation for mesh scenes, especially under arcball-driven views.
4. Picking payload widening after the first hardened slice: richer ids, mesh targets, and less
   ad-hoc payload encoding.


## Scope Guardrails

For the immediate implementation pass:

1. Do not add a generic public binding API yet; keep public setters typed.
2. Do not invent a second mesh renderer path; reuse the current scene -> DRP2 -> runtime flow.
3. Prefer scene-owned reusable resources over visual-private upload helpers.
4. Keep examples and focused tests in lockstep with each retained slice.
5. Treat declared-but-unimplemented public functions as a priority: either implement them narrowly
   or mark/document the gap before depending on them.


## Roadmap After The Immediate Pass

After the immediate interaction/text/annotation passes, proceed in this order unless the user redirects:

1. Depth/depth-state mesh runtime work and fixture pressure.
2. Browser/WebGPU feasibility: replay a narrow DRP2 subset for point, primitive, image, and minimal
   mesh/depth scenes.
3. Transparency architecture: explicit WBOIT-style scene mode through frame plan, DRP2, runtime, and
   capability fallback.
4. Broader figure features: axes, lines/segments, richer annotations, picking refinements, and
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


## Request Slice Status

The first end-to-end request path is now in better shape than the original plan snapshot:

1. image probe no longer falls back to CPU-side texture sampling; misses now stay misses,
2. auxiliary readback execution now resets DRP2 runtime state before each synthetic request stream,
   which avoids `HELLO/REPLY` semantic-state collisions across multiple requests on one runtime,
3. request freshness now stays explicit after polling, so late stale GPU results cannot reappear
   once a newer panel-local request scope has already been claimed,
4. image probes now use the same explicit request recentering rule as point picking instead of an
   implicit fixed-pixel assumption,
5. focused scene coverage now includes:
   - successful combined pick+probe resolution,
   - per-quadrant image probe position checks against a non-uniform texture,
   - transparent GPU probe miss,
   - forced GPU readback failure miss,
   - late-result rejection after newer pick/probe results were already polled.

Batching was considered after the freshness cleanup and explicitly deferred for now:

1. current hover-style traffic is already coalesced to the newest unresolved request per
   panel/kind scope before execution,
2. that coalescing sharply reduces the payoff of batching for ordinary one-panel hover traffic,
3. compatible batching may still become worthwhile for multi-panel or tool-driven request bursts,
   but it is not the current priority,
4. unless profiling shows real churn from the one-stream-per-request path, move up-stack instead of
   broadening the request executor now.


## Completed Context

Completed implementation records:

1. [../done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
2. [../done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
3. [../done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)
4. [../done/SCENE_PICK_PROBE_EXECUTION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION.md)

Backlog context:

1. [../later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
2. [../later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
