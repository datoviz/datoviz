# Datoviz v0.4 Start

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT DISPATCH`
> - **Updated on:** `2026-05-29`
> - **Purpose:** tell agents where to start without duplicating the release roadmap or feature
>   gateboard.

Use this file as the short branch dispatch note. It intentionally does not carry feature tables,
release criteria, or long implementation history.


## Start Here

1. For release sequencing, feature freeze, RCs, final release, release validation, or release
   documentation, read
   [`RELEASE.md`](RELEASE.md).
2. For v0.4 public documentation deliverables, API/docs inventory, RC documentation gates, gallery
   documentation, or migration/known-issues work, read [`DOCUMENTATION.md`](DOCUMENTATION.md).
3. For current implementation lane status, RC1 blockers, and parallel-work guidance, read
   [`STATUS.md`](STATUS.md).
4. For high-payoff shiny demo follow-up beyond WebGPU/WASM and raw `ctypes`, read
   [`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md).
5. For scene semantics, public scene API shape, frame planning, visual families, interaction,
   annotations, scales, or runtime boundaries, read [`../../spec/scene/README.md`](../../spec/scene/README.md).
6. For broad `src/scene` source splits across visual helpers, frame-plan files, runtime emission,
   core scene ownership, query execution, annotations, or domain helpers, read
   [`../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
   and
   [`../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md).
   The immediate scene-refactor execution queue is in
   `SCENE_ARCHITECTURE_COMPLETION_PLAN.md` under **Immediate Next Execution Queue**.
7. For DRP2 commands, fixtures, schemas, or scene code that emits DRP2, read
   [`../../spec/drp2/README.md`](../../spec/drp2/README.md) and
   [`../../spec/drp2/AGENT_SPEC_PHASE.md`](../../spec/drp2/AGENT_SPEC_PHASE.md).


## Current Position

The active stack is `scene` -> `drp2` -> `vklite`/`canvas`, with `app` as the presentation layer.
The low-level graphics modules (`vk`, `vklite`, `canvas`, `stream`, `video`, and `window`) are the
runtime foundation; do not create parallel presentation, frame-stream, or Vulkan wrapper paths.

Native first slices are active for retained visual families, sampled fields, material/controller
state, broad item pick/probe request execution, selection bookkeeping, rendered text/glyphs, label
annotations, continuous colorbars, categorical legends, first-class integer labels, scale bars,
graph-backed techniques, app/offscreen/GLFW rendering, and capture. Treat these as active code, not
future scaffolding.

Current shiny-demo recommendations are recorded in
[`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md):
vector/arrow visuals are the best new demo unlock, labels probe hardening is now transform,
large-field, and request-path pressure work, explanatory layout proof is mostly validation/polish,
and splats are an optional v0.4 experimental showcase if the new visual lands cleanly.

The CPU-side `geom` subset is also active: owned `DvzGeometry` buffers, cube/plane/sphere/surface
grid generators, bounds, normals, transforms, merges, edges, contours, polygon triangulation, mesh
upload, polygon scene helpers, semantic polygon/polygon-set composites, and focused tests/examples.
Remaining `geom` work is optional unless a release example needs it.

For the large scene source/architecture refactor, the latest pickup is after the upload-support,
panel-helper, helper-declaration boundary, shared query-helper, query render-metadata guard,
standard item-id decode, standard item-target eligibility, query native-target policy,
sample-target native policy, FramePlan/render-contract metadata enforcement, typed point-like
fallback label resolution, vector/stroke query-family decode ownership, field dirty propagation,
scalar field sampling split, and typed splat/primitive/image/labels/textured-mesh fallback label
resolution commits through `14b1ab6f7`. `_scene.h` no longer exposes narrow helper declarations;
its remaining broadness is shared retained scene object and type definitions. Start with
annotation/domain cleanup, residual query-family scratch/unsupported-policy ownership, remaining
compatibility quarantine for explicit fixture/import paths, and standalone candidate assessment in
[`../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md);
do not reopen dense/index/material upload emission, panel drawable/viewport helper extraction, or
the helper declarations already moved into owner-private headers unless a new regression points
there. Do not re-split generic query allocation, dense-attribute, target-extent, render-state,
standard item-id decode, standard item-target eligibility, native/sample target fallback policy,
vector/segment/path family decode ownership, or render-metadata completeness helpers from
`src/scene/query/`; render metadata completeness now lives in `frame_plan/`, and normal typed WGSL
fallback label lookup now covers point, pixel, marker, splat, primitive, image/labels, and textured
mesh. The closest standalone scene layers are currently `frame_plan/`,
`render_contract/`, `query/`, `text/`,
`domain/`, and
`visuals/registry/`; `scene_emit/`, `runtime/`, `techniques/`, and `app/` remain
orchestration/runtime layers.


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
4. [`../soon/scene/SCENE_GPU_QUERY_OVERHAUL.md`](../soon/scene/SCENE_GPU_QUERY_OVERHAUL.md) and
   [`../../spec/scene/interaction/GPU_QUERY_SYSTEM.md`](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md)
   before changing pick/probe/query execution, GPU request readback, visual-family query policy, or
   CPU fallback behavior.
5. [`../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md`](../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
   before changing image/labels probe coordinates, panzoom probe mapping, or CPU fallback behavior.
6. [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md)
   before changing pinned readout cards, selected-item metadata cards, public overlay cards, or
   private rich text-block lowering.
7. [`../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
   before broad source-file splits in `src/scene/frame_plan`, `src/scene/scene_emit`,
   `src/scene/render_contract`, `src/scene/runtime`, `src/scene/core`, `src/scene/annotation`,
   `src/scene/domain`, `src/scene/query`, or visual helper folders.


## Validation Defaults

For documentation-only passes, run `git diff --check` and inspect `git status --short`.

For scene/DRP2/runtime code changes, run `just build`, the narrowest relevant `just test <filter>`,
and `just spec-check` for DRP2 schema, fixture, or portable-command changes. Use Vulkan validation
or bounded GLFW/offscreen smoke when graphics lifetimes, command buffers, render targets,
swapchains, or synchronization are touched.
