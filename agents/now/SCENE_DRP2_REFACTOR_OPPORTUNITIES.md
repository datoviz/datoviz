# Scene / DRP2 Refactor Opportunities

> **Execution Status**
> - **Status:** `ACTIVE FOLLOW-UP TRACKER; SCENE.C DOMAIN SPLIT IN PROGRESS`
> - **Updated on:** `2026-05-13`
> - **Scope:** identify high-payoff cleanup and architecture work outside the active
>   scene -> DRP2 emitter split.


## Context

The first `src/scene/converter.c` split has landed according to
[SCENE_CONVERTER_REFACTOR_PLAN.md](SCENE_CONVERTER_REFACTOR_PLAN.md). This note records the next
places where the active scene, scene-to-frame-plan, and DRP2 runtime code would benefit from the
same treatment.

The goal is not to change behavior first. The preferred sequence is to isolate ownership boundaries,
preserve the current scene -> FramePlan -> DRP2 -> vklite/canvas path, and only then make targeted
behavior or data-model improvements.


## Highest-Payoff Refactor Targets

### 0. Finish the Post-Converter Emitter Cleanup

The converter file is gone, but the runtime emitter still has important follow-ups before larger
scene/DRP2 splits:

1. broaden typed FramePlan visual/resource metadata so runtime emission can finish dropping
   semantic inference from keys such as `v%u_%s`, `b%u`, `v%u_texture`, and `v%u#index=b%u`.
2. continue hardening persistent emitter/resource diagnostics as new failure paths are exposed,
3. done for the first slice: `scene_emit.c` now owns retained scene -> FramePlan lowering.
4. done for the retained field/visual/scale slices: `field.c` owns sampled fields, scene buffers,
   field bindings, scalar/image texture staging, and field dirty-state helpers; `visual.c` owns
   visual construction, attributes, bindings, dirty ranges, background visuals, and reset helpers;
   and `scale.c` owns scale, colormap, colorbar, and retained colormap color resolution.

Historical post-converter slices are now landed. The first visual descriptor extraction slice
landed in `3d4bd920`: `visual_pipeline.c` now owns
per-visual resource resolution, family classification, vertex/index counts, and image draw-buffer
narrowing for the multi-visual scene render path. The shader descriptor slice landed in
`bc65010f`: visual shader keys, shader source selection, SPIR-V keys, and pipeline cache keys now
live in `visual_pipeline.c`. The pipeline descriptor slice landed in `45ae792a`: vertex layout and
depth-state selection for the multi-visual scene render path now live in `visual_pipeline.c`. The
bind descriptor slice landed in `0f29f4f1`: MVP/image/shading bind-role selection now lives in
`visual_pipeline.c`, while `frame_plan_runtime.c` still creates the concrete DRP2 bind-group
objects. The first emitter state hardening slice landed in `586d3fa0`: runtime resource/object
state now grows dynamically, fixture temporary state is destroyed explicitly, texture row-pitch
overflow is guarded, and the compute-buffer emitter reacquires entries after possible resource-map
growth. The resource-key helper slice landed in `cb0576a1`: `scene_resource_key.c` now owns the
current visual, buffer, texture, indexed-visual, and split-visual string conventions. The typed
metadata slices add `DvzFramePlanVisualMeta` and `DvzFramePlanUploadMeta`, populate them from
retained scene visuals/uploads, store resource kind/role on persisted runtime resources, and make
runtime visual-family/descriptor/depth resolution prefer typed ids and roles. Fixture/manual
FramePlans still fall back to the old strings, but that parsing is now centralized behind
`_render_visual_resource_id()` in `visual_pipeline.c`. Commit `2c90c912` adds focused diagnostics
for malformed typed visual metadata, covering the first item in the suggested order.

This lane is now mostly in maintenance mode: add diagnostics/typed metadata only when new
retained paths expose a concrete gap. The next higher-payoff lane is the `scene.c` domain split.


### 1. Split `src/scene/scene.c`

`scene.c` remains the largest active scene implementation file and owns too many domains
(~5k lines after the first `scene_emit.c` extraction):

1. `DvzScene`, `DvzFigure`, and `DvzPanel` lifecycle.
2. Visual family constructors, visual attributes, bindings, dirty ranges, and background visuals.
3. Sampled field storage, region validation, scalar field conversion, and image texture staging.
4. Scale, colormap, and colorbar bookkeeping.
5. Interaction policies, selections, link channels, hover state, and pinned readouts.
6. Text and annotation retained-object bookkeeping.
7. Done for first slice: scene -> FramePlan upload/render lowering now lives in `scene_emit.c`.
8. Scene JSON serialization.

Recommended split:

1. `scene_core.c` - scene, figure, panel lifecycle and shared ownership helpers.
2. `visual.c` - visual families, attributes, bindings, dirty ranges, and visual reset helpers.
3. `field.c` - sampled fields, field regions, field geometry, scalar reads, and texture staging.
4. `scale.c` - scale, colormap, and colorbar retained objects.
5. `interaction.c` - interaction policy, selection, link channels, hover/readout bookkeeping.
6. `text_annotation.c` - font, text, annotation, and label retained objects.
7. Done: `scene_emit.c` - scene -> `DvzFramePlan` lowering only.
8. `scene_json.c` - `dvz_scene_json()` and scene serialization helpers.

Do this mechanically at first. The field, visual, and scale slices are now split out. The next best
slice is `interaction.c`, because interaction policies, selections, link channels, hover state, and
pinned readouts are still mixed into `scene.c` and are already isolated from the FramePlan emitter.
Keep `_scene.h` as the private shared state header until the split settles, then consider smaller
private headers by ownership domain.


### 2. Make Scene Resource Keys Explicit

The current scene -> frame-plan path still relies on string-encoded semantics such as `v%u_%s`,
`b%u`, `v%u_texture`, and `v%u#index=b%u`. The converter then recovers meaning from names and data
tags. This works for the current slice but will become fragile as visual families, picking payloads,
constant attributes, and WebGPU replay broaden.

Recommended three-step path:

1. Done in `cb0576a1`: `scene_resource_key.c` / `_scene_resource_key.h` owns the current key
   formatting and split parsing while preserving the existing strings.
2. Continue routing any remaining debug/JSON-only uses through that helper when they become part of
   the scene -> DRP2 contract.
3. Replace stringly visual/resource detection with typed `DvzFramePlan` metadata for visual
   type, attribute role, topology, index buffer, texture role, panel attachment, and controller mode.

The helper extraction was behavior-preserving. The first typed metadata slices have landed with
focused FramePlan/runtime-emitter coverage, malformed typed metadata now has focused diagnostics,
and `scene_emit.c` now isolates the first scene -> FramePlan lowering slice. Next work should
extract the retained visual/field domains, with emitter failure-path hardening as a sidecar when
concrete gaps appear.


### 3. Split `src/scene/pick_probe.c`

`pick_probe.c` is already a useful extraction from `scene.c`, but it still mixes separate concerns:

1. Pending pick/probe request queues.
2. Freshness scopes and stale-result rejection.
3. Result ring buffers.
4. Panel coordinate and request-NDC mapping.
5. CPU point hit testing.
6. Synthetic image-probe frame-plan construction.
7. Auxiliary DRP2 runtime/readback execution.

Recommended split:

1. `request_queue.c` - pending request coalescing, freshness scopes, result queues, poll helpers.
2. `hit_test.c` - panel coordinate helpers and CPU point hit testing.
3. `probe_plan.c` - synthetic image probe frame-plan construction.
4. `request_execute.c` - auxiliary runtime creation, DRP2 execution, and readback download.

Keep the public API unchanged: `dvz_panel_pick()`, `dvz_panel_probe()`,
`dvz_scene_poll_pick()`, `dvz_scene_poll_probe()`, and `dvz_figure_process_requests()` should remain
the outward surface.


### 4. Split `src/drp2/runtime.c`

`runtime.c` currently combines backend-agnostic semantic validation with the vklite backend
implementation. That makes ownership and failure-path review harder than it needs to be.

Recommended split:

1. `runtime.c` - public facade and runtime configuration/lifecycle.
2. `runtime_semantic.c` - semantic object state, validation, state clone/commit, and command rules.
3. `runtime_vklite_objects.c` - vklite object registry, destruction, borrowed objects, and deferred
   destruction.
4. `runtime_vklite_pipeline.c` - shaderc loading, shader modules, bind group layouts, bind groups,
   render pipelines, and compute pipelines.
5. `runtime_vklite_transfer.c` - buffer/texture writes, staging, copies, and buffer download.
6. `runtime_vklite_pass.c` - command buffers, render passes, compute passes, viewport/scissor,
   draw/dispatch, and submit handling.

This should happen after the scene/converter boundary is calmer, because the runtime API is a key
stability point for app, request execution, and future WebGPU contract pressure.


### 5. Split DRP2 Stream Construction From JSON

`src/drp2/stream.c` is large but mostly coherent. The first useful split is to separate command
appenders from JSON/debug serialization:

1. `stream.c` - stream lifecycle and command appending.
2. `stream_json.c` - `dvz_drp2_stream_json()` and command serialization helpers.

There is already a scene-local JSON helper in `src/scene/_json.h`, while `drp2/stream.c` carries a
second JSON builder. Move the generic builder to `src/common/_json.h` first, then let scene,
FramePlan, and DRP2 stream serializers share it.


### 6. Split `src/scene/tests/test_scene.c`

`test_scene.c` now spans panzoom, arcball, camera, FramePlan, converter emission, retained scene,
fields, scales, interaction, pick/probe, app/offscreen, capture, and mesh/depth rendering. It is
valuable coverage, but it is too broad for focused refactor loops.

Recommended split under `src/scene/tests/`:

1. `test_panzoom_arcball.c`
2. `test_frame_plan.c`
3. `test_frame_plan_emit.c`
4. `test_scene_graph.c`
5. `test_scene_fields.c`
6. `test_scene_interaction.c`
7. `test_scene_pick_probe.c`
8. `test_scene_app.c`

Keep `test_scene(TstSuite* suite)` as the single module entry point and have it delegate to smaller
registration helpers.


## Suggested Order

1. Done: extract `field.c` for sampled fields, retained scene buffers, regions, scalar conversion,
   image texture staging, and field dirty-state helpers.
2. Done: extract `visual.c` from `scene.c` mechanically for visual constructors, attributes,
   bindings, dirty ranges, background visual helpers, and visual reset/destruction helpers.
3. Done: extract `scale.c` for scale, colormap, colorbar, and retained colormap color resolution.
4. Extract `interaction.c` for interaction policies, selections, link channels, hover state, and
   pinned readouts.
5. Add emitter diagnostics/overflow/downcast guards only where the extraction or tests expose a
   concrete failure path.
6. Extract `text_annotation.c`.
7. Split `pick_probe.c` after one more focused request-path validation pass.
8. Split `drp2/runtime.c` after the scene/DRP2 contract stops moving quickly.
9. Move JSON builder support to `src/common` and split serializers.
10. Split scene tests in parallel with the implementation files they cover.


## Validation Guidance

For mechanical file splits:

1. `just build`
2. `just test scene`
3. `just test drp2` when DRP2 runtime or stream files move
4. `git diff --check`

For behavior-changing follow-up work:

1. run the narrow tests for the touched domain,
2. run runtime/offscreen smoke tests for scene -> DRP2 -> vklite changes,
3. run Vulkan validation smoke tests for changes touching frame lifetimes, command buffers,
   render targets, synchronization, readbacks, or borrowed handles.
