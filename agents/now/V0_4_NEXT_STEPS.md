# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-14`
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
   DRP2 emission, and request/result state; `app` owns the small presentation loop over scene,
   canvas, and the DRP2 runtime.
3. Built-in visual families currently implemented are `point`, `primitive`, `mesh`, path-as-line/strip,
   and `image`.
4. Panel controllers are live: panzoom and arcball feed per-panel transforms.
5. Per-panel viewport/scissor and offscreen multi-panel preservation work through the emitted DRP2
   path.
6. Retained sampled fields, scales, scene buffers, and primitive/mesh shading uniforms now share the
   scene -> frame-plan -> DRP2 binding path.
7. Interaction bookkeeping, queued pick/probe requests, result polling, selection/link objects,
   pinned readouts, and text/annotation retained objects now have first source implementations and
   focused bookkeeping tests.
8. GPU-backed request execution is narrow but real: point picking and image probing execute through
   auxiliary DRP2 streams and runtime readbacks after the main figure frame has populated runtime
   resources.
9. `app` is an active presentation module. Recent work added frame callbacks, compact/full DRP2
   trace output, combined FPS/status reporting, and figure-size synchronization before frame
   emission.

Focused validation recorded before the latest `2026-05-13` follow-up commits:

1. `just spec-check`: last recorded pass remained `119/119` DRP2 fixtures; `52` fixture-runner
   tests passed.
2. `just test drp2`: last recorded pass remained `73/73`.
3. `just test scene`: passed `127/127` after finishing the current request-resolution cleanup
   pass. Point pick and image probe now resolve through GPU-backed auxiliary DRP2/readback
   execution only; request freshness is explicit and persistent per panel/request-kind scope, and
   image probes now use the same explicit recentering rule as point picking.
4. `git diff --check`: passed on the latest scene slices.

Recent unreflected code commits after that validation include app trace/status cleanup, request
runtime reset hardening, figure-size synchronization, and point-picking panel coordinate fixes.
Re-run the relevant focused tests before treating the snapshot as fully current.


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

Deliver the next implementation slices in this order unless the user redirects:

1. Done: focused scene-test decomposition split `src/scene/tests/test_scene.c` into short
   domain-named files under `src/scene/tests/` while preserving the current test function names and
   `test_scene(TstSuite*)` as the single module entry point. `test_scene.c` is now the aggregator
   only, `test_scene.h` remains the shared declaration header, and the split files are
   `panzoom_arcball.c`, `frame_plan.c`, `frame_plan_emit.c`, `scene_graph.c`, `fields.c`,
   `interaction.c`, `pick_probe.c`, and `app.c`.
2. Current next: finish and validate the native 3D pressure smoke around the existing
   `examples/c/hello_mesh_glfw.c`. That example already exercises an interactive mesh scene with
   arcball, depth, resize-through-app synchronization, and a frame callback through the scene ->
   DRP2 -> app boundary. The remaining gap is to make capture/readback part of the documented
   smoke path instead of adding a duplicate 3D example.
3. Manual interactive smoke set: point hover picking, image probe, panzoom, arcball, partial texture
   update, and multi-panel examples with clear run commands and expected behavior.
4. Hygiene/safety pass over the hot scene/DRP2/app files that changed most recently: bounds,
   ownership, stale-result handling, transient runtime object cleanup, and warning/static-analysis
   readiness.
5. Early WebGPU feasibility spike: replay a tiny DRP2 subset for clear, static point/primitive/image,
   then depth. Keep it contract-pressure only; do not fork scene semantics.
6. Rendered colorbar/text/annotation realization, reusing the current scene -> DRP2 path after the
   native 3D and manual-smoke gaps are clearer.
7. Picking payload widening after the hardened slice: richer ids, mesh targets, and less ad-hoc RGBA
   payload encoding.

Implementation-level checklists for these lanes are recorded in
[../../docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md).

Sidecar design slice recorded on 2026-05-13:

1. Visual attribute sources and constant-value optimization are specified in
   [../../spec/scene/pipeline/ATTRIBUTE_SOURCES.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/ATTRIBUTE_SOURCES.md).
2. The implementation pickup note is
   [../../docs/tasks/2026-05-13-visual-attribute-sources/NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/docs/tasks/2026-05-13-visual-attribute-sources/NEXT_STEPS.md).
3. This is not yet active code. The first recommended slice is constant point `size` via
   `dvz_visual_set_value(point, "size", &size)` while retaining the current dense per-item fallback.


## Scope Guardrails

For the immediate implementation pass:

1. Do not add a generic public binding API yet; keep public setters typed.
2. Do not invent a second mesh renderer path; reuse the current scene -> DRP2 -> runtime flow.
3. Prefer scene-owned reusable resources over visual-private upload helpers.
4. Keep examples and focused tests in lockstep with each retained slice.
5. Treat declared-but-unimplemented public functions as a priority: either implement them narrowly
   or mark/document the gap before depending on them.


## Roadmap After The Immediate Pass

After the immediate native 3D/manual-smoke/safety passes, proceed in this order unless the user redirects:

1. Browser/WebGPU feasibility: replay a narrow DRP2 subset for point, primitive, image, and minimal
   mesh/depth scenes.
2. Transparency architecture: explicit WBOIT-style scene mode through frame plan, DRP2, runtime, and
   capability fallback.
3. Broader figure features: axes, lines/segments, rendered text/labels, colorbars, richer
   annotations, picking refinements, and additional visual families.
4. Larger code organization cleanup once the active API seams stabilize enough to avoid churn.


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
