# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT DISPATCH`
> - **Updated on:** `2026-05-25`
> - **Purpose:** tell agents where to start without duplicating the release roadmap or feature
>   gateboard.

Use this file as the short branch dispatch note. It intentionally does not carry feature tables,
release criteria, or long implementation history.


## Start Here

1. For release sequencing, feature freeze, RCs, final release, release validation, or release
   documentation, read
   [`V0_4_RELEASE_MASTER_CHECKLIST.md`](V0_4_RELEASE_MASTER_CHECKLIST.md).
2. For current implementation lane status, RC1 blockers, and parallel-work guidance, read
   [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
3. For scene semantics, public scene API shape, frame planning, visual families, interaction,
   annotations, scales, or runtime boundaries, read [`../../spec/scene/README.md`](../../spec/scene/README.md).
4. For DRP2 commands, fixtures, schemas, or scene code that emits DRP2, read
   [`../../spec/drp2/README.md`](../../spec/drp2/README.md) and
   [`../../spec/drp2/AGENT_SPEC_PHASE.md`](../../spec/drp2/AGENT_SPEC_PHASE.md).


## Current Position

The active stack is `scene` -> `drp2` -> `vklite`/`canvas`, with `app` as the presentation layer.
The low-level graphics modules (`vk`, `vklite`, `canvas`, `stream`, `video`, and `window`) are the
runtime foundation; do not create parallel presentation, frame-stream, or Vulkan wrapper paths.

Native first slices are active for retained visual families, sampled fields, material/controller
state, pick/probe, selection bookkeeping, rendered text/glyphs, label annotations, continuous
colorbars, scale bars, graph-backed techniques, app/offscreen/GLFW rendering, and capture. Treat
these as active code, not future scaffolding.

The CPU-side `geom` subset is also active: owned `DvzGeometry` buffers, cube/plane/sphere/surface
grid generators, bounds, normals, transforms, merges, edges, contours, polygon triangulation, mesh
upload, polygon scene helpers, and focused tests/examples. Remaining `geom` work is optional unless
a release example needs it.


## Guardrails

1. Stabilize active modules first; keep inactive scaffolding such as broad `wasm` and renderer/client
   layers untouched unless explicitly requested.
2. Do not invent a second mesh renderer, presentation layer, frame stream, or Vulkan wrapper path.
3. Prefer scene-owned reusable resources over visual-private upload helpers.
4. Keep examples and focused tests in lockstep with each retained slice.
5. Treat declared-but-unimplemented public functions as a priority: implement them narrowly or
   document the gap before depending on them.
6. Preserve immediate presentation paths for run-as-fast-as-possible benchmarks through explicit
   continuous scheduling.


## Important Records

1. [`../done/APP_FRAME_SCHEDULING_REFACTOR.md`](../done/APP_FRAME_SCHEDULING_REFACTOR.md) before
   changing app loop, frame pacing, window wait/wakeup, request-frame wakeups, or immediate-present
   CPU behavior.
2. [`../done/SCENE_DRP2_IMPLEMENTATION.md`](../done/SCENE_DRP2_IMPLEMENTATION.md) and
   [`../done/DRP2_SCENE_SAFETY.md`](../done/DRP2_SCENE_SAFETY.md) before changing the completed
   scene -> DRP2 -> runtime path.
3. [`../done/TEST_RUNNER_MODERNIZATION.md`](../done/TEST_RUNNER_MODERNIZATION.md) and
   [`../later/TEST_RUNNER_SCHEDULING.md`](../later/TEST_RUNNER_SCHEDULING.md) before changing test
   scheduling, process sharding, CI orchestration, or skip/reporting behavior.
4. [`../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md`](../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
   before changing image probe coordinates, hidden pick-capable image behavior, panzoom probe
   mapping, or CPU fallback behavior.


## Validation Defaults

For documentation-only passes, run `git diff --check` and inspect `git status --short`.

For scene/DRP2/runtime code changes, run `just build`, the narrowest relevant `just test <filter>`,
and `just spec-check` for DRP2 schema, fixture, or portable-command changes. Use Vulkan validation
or bounded GLFW/offscreen smoke when graphics lifetimes, command buffers, render targets,
swapchains, or synchronization are touched.
