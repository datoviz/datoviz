# Datoviz v0.4 C Implementation Checklist

> **Execution Status**
> - **Status:** `ACTIVE OPERATIONAL TRACKER`
> - **Updated on:** `2026-05-19`
> - **Purpose:** tell agents where v0.4 C implementation stands, what to pick next, and which
>   lanes can run in parallel.

This file is the branch-level operational checklist for finishing the C implementation of v0.4.
It complements:

1. [`NEXT_STEPS.md`](NEXT_STEPS.md): current technical context and recent validation.
2. [`RELEASE.md`](RELEASE.md): release scope, API policy,
   v0.3 regression checklist, and post-feature-completion work.
3. [`../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md`](../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md): example
   priority and capability matrix.
4. [`../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md`](../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md):
   example release targets, current readiness, and v0.4/v0.5/later classification.


## How To Use This File

When an agent starts work on v0.4 C implementation:

1. Read this file first.
2. Pick the first non-complete item in **Critical Path** unless the user asks for a specific lane.
3. If a critical-path item is blocked, pick a **Parallel Lane** item with a disjoint write scope.
4. After landing a slice, update this file:
   - mark the item `Done`, `Partial`, `Blocked`, or `Deferred`,
   - add the most relevant validation command and result,
   - move the next concrete pickup item to the top of the appropriate section.

Status vocabulary:

1. `Done`: implemented and covered by focused validation.
2. `Partial`: useful implementation exists, but the release-quality slice is incomplete.
3. `Next`: the next preferred pickup item.
4. `Parallel`: useful work that can run alongside the critical path.
5. `Blocked`: waiting for a dependency or decision.
6. `Deferred`: explicitly not required for v0.4.


## Current Next Pickup

**Next critical-path item:** app frame scheduling refactor.

Reason: `dvz_app_run(app, 0)` currently uses an unconditional interactive render loop. That is
useful for immediate-present benchmarks, but it can burn CPU/GPU on static scenes and mixes frame
production, frame demand, event waiting, and benchmark policy. The next implementation lane is to
make the built-in app loop event-aware and optionally capped while keeping
`dvz_app_window_render_once()`, canvas primitives, and explicit continuous benchmark behavior
scheduler-free.

Primary specs:

1. [`APP_FRAME_SCHEDULING_REFACTOR.md`](APP_FRAME_SCHEDULING_REFACTOR.md)

Previous critical-path text/axes work remains below as the next visual-release lane after the app
scheduler pass.


## Critical Path

### 0. v0.4 Scope And Regression Baseline

Status: `Done`

Outcome:

1. WebGPU/WASM is in v0.4 as experimental scope.
2. OO plotting is external GSP/VisPy2 scope.
3. Publication/vector export is external GSP/Matplotlib scope.
4. v0.3 visible capability regressions to avoid are listed in
   [`RELEASE.md`](RELEASE.md).

Last validation:

1. `git diff --check` passed before commit `10d87091`.


### 1. App Frame Scheduling Refactor

Status: `Next`

Goal:

Make the built-in interactive app loop event-aware, support optional FPS caps for active
continuous work, and preserve explicit run-as-fast-as-possible immediate-present benchmarks.

Required v0.4 slice:

1. monotonic timing helper for scheduling deadlines,
2. window backend `wait` / `wait_timeout` / wakeup hooks with GLFW support,
3. app scheduling config and environment overrides,
4. per-window dirty/frame-requested state,
5. on-demand default behavior for static interactive scenes,
6. continuous rendering for animations, replay, streaming, and explicit benchmark mode,
7. optional FPS cap applied only inside `dvz_app_run()`,
8. focused app/window validation plus manual GLFW smoke notes.

Primary plan:

1. [`APP_FRAME_SCHEDULING_REFACTOR.md`](APP_FRAME_SCHEDULING_REFACTOR.md)

Suggested validation:

1. `git diff --check`
2. `just build`
3. focused app/window/canvas tests
4. manual GLFW smoke for static idle, input wakeup, animation/replay continuity, immediate
   continuous benchmark, and immediate capped behavior


### 2. Text Release-Hardening And Integration

Status: `Next after scheduler`

Goal:

Turn the existing retained/rendered text path into a dependable v0.4 explanatory-object primitive.

Current state:

1. `examples/c/visuals/text.c` exercises basic `dvz_text()` rendering, strings, size, color,
   anchor, angle, multiline text, tick-like labels, and UTF-8 fallback behavior.
2. The scene visual path already has text/glyph state and a bitmap-atlas renderer lane.
3. The remaining work is integration, validation, and release-quality behavior rather than first
   proof of visibility.

Required v0.4 slice:

1. built-in fallback font,
2. visible single-line UTF-8 strings,
3. run-level size and color,
4. screen-space placement,
5. simple data-space anchoring,
6. panel viewport/scissor clipping,
7. offscreen and GLFW rendering,
8. retained update and destroy behavior,
9. focused scene tests plus one app/offscreen smoke,
10. documented behavior for unsupported glyphs and renderer choices.

Non-goals for v0.4:

1. paragraph layout,
2. complex shaping,
3. rich text style runs,
4. TeX/math layout inside Datoviz,
5. color emoji,
6. glyph-level picking,
7. public glyph-atlas APIs,
8. collision avoidance.

Suggested validation:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. focused app/offscreen visible-text smoke
5. `clang-tidy -p build --quiet` on touched scene files when practical


### 3. 2D Axes And Ticks

Status: `Partial`; axis API/example state exists, but visible generated ticks/labels are incomplete.

Goal:

Restore visible 2D axes quality without copying v0.3 API shape.

Current state:

1. `examples/c/techniques/scatter_axes.c` already uses panel-owned axes, domains, grid enablement,
   labels, panzoom, and normalized data positions.
2. Treat that example as an axes API smoke until generated tick geometry and tick/axis labels render
   through the text path.

Required v0.4 slice:

1. panel-owned x/y axes for panzoom panels,
2. linear numeric tick generation,
3. tick labels and axis labels,
4. units/format policy,
5. axis lines and tick marks,
6. optional grid lines,
7. panzoom and resize updates,
8. diagnostics for unsupported axis kinds.

Primary specs:

1. [`../../spec/scene/semantics/AXES.md`](../../spec/scene/semantics/AXES.md)
2. [`../soon/scene/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md`](../soon/scene/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md)


### 4. Continuous Colorbars

Status: `Parallel`; retained colorbar bookkeeping exists, ramp work can proceed, and final
title/tick labels depend on the text integration pass.

Goal:

Render continuous colorbars bound to `DvzScale`/`DvzColormap`.

Required v0.4 slice:

1. one panel-attached continuous colorbar,
2. vertical and horizontal orientation,
3. visible ramp,
4. deterministic tick generation,
5. title, units, and formatted labels,
6. updates when scale domain or colormap changes,
7. explicit diagnostics for categorical or unsupported scales.

Primary spec:

1. [`../../spec/scene/slices/COLORBAR_RENDERING_SLICE.md`](../../spec/scene/slices/COLORBAR_RENDERING_SLICE.md)


### 5. Basic Annotations And Readouts

Status: `Parallel`; retained annotation/readout bookkeeping exists, visible labels depend on the
text integration pass.

Goal:

Make retained annotation/readout objects visible enough for examples.

Required v0.4 slice:

1. panel-attached text labels,
2. data-anchored labels,
3. pinned image-probe readout text,
4. optional crosshair/probe overlay if needed by the linked-panel example,
5. update/destroy tests.

Primary specs:

1. [`../../spec/scene/slices/ANNOTATION_LABEL_SLICE.md`](../../spec/scene/slices/ANNOTATION_LABEL_SLICE.md)
2. [`../../spec/scene/semantics/ANNOTATIONS.md`](../../spec/scene/semantics/ANNOTATIONS.md)


### 6. Grid Layout And Linked Panels

Status: `Partial`

Goal:

Make multi-panel figures ergonomic and stable enough for axes, colorbars, linked probes, and
dashboard-like examples.

Required v0.4 slice:

1. retained grid/subplot API,
2. rows, columns, spans, and panel reservation,
3. predictable resize behavior,
4. linked panzoom by x, y, or both dimensions,
5. linked crosshair/probe state for image examples,
6. offscreen and GLFW multi-panel tests.

Primary specs:

1. [`../../spec/scene/dashboards/GRID_LAYOUT.md`](../../spec/scene/dashboards/GRID_LAYOUT.md)
2. [`../../spec/scene/core/PANEL_LAYOUT.md`](../../spec/scene/core/PANEL_LAYOUT.md)


### 7. Visual Family Polish Pass

Status: `Partial`

Goal:

Make the supported v0.4 visual families visibly usable and avoid v0.3-visible regressions.

Required v0.4 checks:

1. `point`: high-count rendering, size/color updates, point pick compatibility.
2. `pixel`: dense 2D rendering and update performance.
3. `marker`: shape, edge, linewidth, size, color, and marker pick readiness.
4. `segment`/`path`: width, color, cap/join basics, grouped/ragged path policy.
5. `image`: filtering, scale/colormap binding, probe correctness.
6. `mesh`: material/depth/transparency basics and example coverage.
7. `sphere`: impostor mode, lighting/material controls, SSAO/MSAA composition.
8. `volume`: slice/raymarch mode, transfer/value range, clipping basics.
9. `text`/`glyph`: covered by the text release-hardening and integration slice.

Primary docs:

1. [`../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`](../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
2. [`../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md`](../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md)


### 8. Selection, Pick, Probe, And Highlight

Status: `Partial`

Current baseline:

1. GPU-backed point picking exists.
2. GPU-backed image probing exists.
3. Selection/link bookkeeping exists.

Required v0.4 slice:

1. robust point pick and image probe steady-state behavior,
2. marker picking if marker is a supported v0.4 visual family,
3. selection highlight styling for point/marker/image examples,
4. linked-panel probe/crosshair state,
5. explicit deferral for mesh/path/volume/text picking if not implemented.

Primary specs:

1. [`../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md`](../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
2. [`../../spec/scene/interaction/PICKING.md`](../../spec/scene/interaction/PICKING.md)
3. [`../../spec/scene/interaction/SELECTION.md`](../../spec/scene/interaction/SELECTION.md)


### 8. WebGPU/WASM Experimental Slice

Status: `Partial`

Goal:

Ship a narrow experimental browser path in v0.4, without claiming native feature parity.

Required v0.4 slice:

1. documented supported DRP2 command subset,
2. point, primitive, image, and preferably basic mesh scene streams,
3. WGSL emission for the supported subset,
4. browser WebGPU runner with no demo-only shortcuts in strict paths,
5. unsupported-feature diagnostics,
6. fixture/preflight validation.

Primary docs:

1. [`../soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md`](../soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md)
2. [`../soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md`](../soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md)


### 9. Gallery And Example Pressure Tests

Status: `Partial`

Goal:

Use examples to prove the C implementation is coherent.

First v0.4 batch:

1. scatter with axes and markers,
2. image probe with colorbar and pinned readout,
3. linked panels,
4. protein/mesh/sphere flagship,
5. volume/brain example,
6. LiDAR or dense point cloud with EDL,
7. one WebGPU/WASM experimental page for the supported subset.

Primary docs:

1. [`../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md`](../../spec/scene/examples/EXAMPLE_PRIORITIZATION.md)
2. [`../../spec/scene/examples/README.md`](../../spec/scene/examples/README.md)
3. [`../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md`](../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md)


### 10. Runtime Hardening

Status: `Parallel`

Goal:

Keep the active scene -> DRP2 -> vklite/canvas path safe under long-running examples.

Required v0.4 checks:

1. repeated-frame resource reuse,
2. resize safety,
3. descriptor refresh after stable resource recreation,
4. destroy paths and partial-initialization cleanup,
5. no transient backend-object accumulation,
6. bounded GLFW/offscreen smoke tests,
7. Vulkan validation smoke for touched graphics paths.


### 11. API Inventory And Release Readiness

Status: `Parallel`

Goal:

Make the public C surface coherent once feature work is close enough.

Required v0.4 checks:

1. public header inventory,
2. public/internal/experimental symbol classification,
3. ownership and destroy rules,
4. Doxygen comments,
5. explicit lower-level API status,
6. docs synchronized with actual v0.4 behavior.

Primary doc:

1. [`RELEASE.md`](RELEASE.md)


## Parallel Work Guidance

Good parallel lanes right now:

1. **Text release-hardening:** `src/scene`, scene shaders, `examples/c/visuals/text.c`, focused text
   tests, and app/offscreen visible-text smoke.
2. **Axes/tick integration:** axis tick generation, semantic state, grid/line geometry, and tests
   that can use current text as labels mature.
3. **Colorbar ramp planning:** scale/colormap/ramp layout and diagnostics, with label emission wired
   once the text integration pass is stable.
4. **WebGPU command parity:** `examples/webgpu`, DRP2 fixture/preflight work, no scene API churn.
5. **Example audit/polish:** C examples and gallery harnesses that use already-implemented
   features.
6. **Runtime hardening:** DRP2/vklite/app lifetime bugs with focused tests.
7. **API inventory/docs:** read-only or markdown-only work that does not alter active C code.
8. **DRP2 contract and fixture maintenance:** `spec/drp2`, `spec/drp2/AGENT_SPEC_PHASE.md`,
   `tools/drp2_fixture_runner.py`, schema docs, and fixture updates that keep the active command
   surface aligned with native and WebGPU pressure.
9. **DVZR recording/replay portability:** `src/drp2` recording/replay code, `src/app` recording
   hooks, replay/player examples, and raw-fallback diagnostics. Keep this coordinated with DRP2
   schema/fixture work when portable command coverage changes.
10. **Render-contract and technique hardening:** scene FramePlan contracts, technique builders,
    post-emit DRP2 validation, and deterministic offscreen readback coverage for source-over,
    WBOIT, depth peeling, MSAA, EDL, SSAO, volume occlusion, and scene occlusion.
11. **Material and shader ABI polish:** active scene shader ABI, material uniforms, vertex
    attribute descriptors, generated shader variants, GLSL/WGSL parity checks, and material-model
    examples.
12. **Selection and request payload widening:** richer point/marker/image pick/probe payloads,
    highlight state, linked-panel request propagation, and explicit deferrals for mesh/path/volume
    picking.
13. **Test-runner scheduling follow-up:** process-level sharding, CI orchestration, optional
    thread-safe workers, and remaining skip cleanup as tracked in
    [../soon/tooling/TEST_RUNNER_SCHEDULING.md](../soon/tooling/TEST_RUNNER_SCHEDULING.md). The completed serial
    modernization history is in
    [../done/TEST_RUNNER_MODERNIZATION.md](../done/TEST_RUNNER_MODERNIZATION.md).
14. **Performance and long-run smoke:** immediate-presentation FPS preservation, repeated-frame
    allocation/destructor pressure, descriptor churn, bounded live GLFW loops, and trace-assisted
    investigation of unexpected stream changes.
15. **Capture/video/export support for examples:** offscreen screenshots, frame-sequence/video
    paths, gallery artifact generation, and codec/backend skip behavior. Keep publication/vector
    export out of Datoviz v0.4 scope.
16. **GUI-driven example controls:** ImGui/example-side controls that exercise existing scene/app
    parameters without promoting the GUI module into a broad public API surface.
17. **Release documentation and website staging:** user-facing docs, example staging tables,
    capability matrix updates, public/private/experimental labels, and known-gap notes.

Avoid parallel edits that touch the same write scope:

1. two agents both changing the same scene visual emission code,
2. text and axes both rewriting the same FramePlan contribution structures,
3. WebGPU and DRP2 schema/native validation changes to the same command without coordination,
4. app/runtime hardening and feature emission changing the same request or frame loop,
5. DRP2 contract/schema updates and DVZR portable-command updates changing the same command
   serialization without a shared compatibility decision,
6. render-contract technique work and visual-family polish both changing pass roles, resource ids,
   or shader-variant selection in `src/scene`,
7. material/shader ABI work and WebGPU/WGSL parity work changing the same binding layout without a
   fixture/spec update,
8. test-runner metadata migration and subsystem feature tests rewriting the same test registration
   blocks at the same time.


## Example Agent Prompts For Parallel Lanes

Use these as starting prompts when assigning one lane to an agent. Each prompt should be narrowed to
one concrete pickup item before execution. Agents should update this checklist and the lane-specific
planning document when they land a slice.

Common coordination rules:

1. start by reading this checklist, `NEXT_STEPS.md`, and the lane-specific docs named below,
2. state the write scope before editing and avoid files owned by another active lane,
3. do not rewrite shared scene emission, DRP2 schemas, shader bind layouts, or test registration
   blocks unless that is the explicit lane objective,
4. if subagents are authorized, delegate only disjoint read-only audits or disjoint write scopes,
5. do not delegate the immediate blocker for the lane; keep that work local,
6. make every subagent report changed files, validation run, and remaining risks,
7. finish with `git diff --check` and the narrowest relevant build/test command.


### Prompt: Text Release-Hardening

```text
You own the text release-hardening lane for Datoviz v0.4. Read
agents/now/IMPLEMENTATION.md, spec/scene/slices/TEXT_RENDERING_SLICE.md,
spec/scene/semantics/TEXT.md, spec/scene/implementation/TEXT_SHAPING_ATLAS.md,
and agents/soon/text-layout/SCENE_TEXT_GLYPH_PLAN.md.

Goal: make the existing retained/rendered text path release-quality for the next small slice.
Stay inside src/scene text/glyph code, scene shaders needed for text, examples/c/visuals/text.c,
and focused scene/app text tests. Do not redesign axes, colorbars, annotations, or shared
FramePlan contribution structures unless the exact text bug requires it.

Conflict avoidance: coordinate with axes/colorbar/annotation agents by exposing stable text
behavior and tests, not by changing their semantic state. Avoid shader bind-layout changes unless
you also update material/WebGPU owners and fixtures.

If subagents are authorized, use one explorer to audit text tests/examples for missing behavior and
one worker only for a disjoint example or test update. Keep core text implementation local.
Validate with git diff --check, just build, just test scene, and one focused app/offscreen visible
text smoke.
```


### Prompt: Axes And Tick Integration

```text
You own the 2D axes/tick integration lane. Read the checklist, spec/scene/semantics/AXES.md, and
agents/soon/scene/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md.

Goal: land one narrow axes slice: generated numeric ticks, axis/tick line geometry, label emission,
or resize/panzoom update behavior. Stay in axes semantic state, panel layout hooks, line/segment
geometry, examples/c/techniques/scatter_axes.c, and focused scene tests.

Conflict avoidance: do not rewrite text rendering internals; call the current text API and document
any required text behavior as a dependency. Do not change colorbar scale policy unless the task is
explicitly shared with that lane.

If subagents are authorized, use an explorer for v0.3-visible axes capability gaps and a worker for
example-only polish. Keep tick generation and scene state changes local. Validate with git
diff --check, just build, and focused scene axes/scatter tests.
```


### Prompt: Colorbar Ramp Planning

```text
You own the continuous colorbar lane. Read the checklist and
spec/scene/slices/COLORBAR_RENDERING_SLICE.md.

Goal: implement one colorbar slice such as ramp geometry, scale/colormap binding, deterministic
ticks, orientation layout, diagnostics, or update/destroy behavior. Stay in colorbar state,
scale/colormap integration, ramp visual emission, examples using image/colormap, and focused tests.

Conflict avoidance: do not change text internals; wire labels through the current text surface and
leave label-quality blockers documented. Do not alter global scale semantics used by images unless
the image/visual-family owner is coordinated.

If subagents are authorized, use one explorer for scale/colormap call sites and one worker for a
disjoint example or doc update. Keep colorbar state and emission local. Validate with git
diff --check, just build, and focused scene colorbar/image tests.
```


### Prompt: WebGPU Command Parity

```text
You own the WebGPU/WASM experimental parity lane. Read the checklist,
agents/soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md, agents/soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md, and
spec/drp2/AGENT_SPEC_PHASE.md.

Goal: advance the narrow browser path for the supported DRP2 subset. Work in examples/webgpu,
WebGPU preflight fixtures, WGSL emission, and docs for supported/unsupported commands. Do not
change native scene APIs or invent a second scene renderer contract.

Conflict avoidance: coordinate before changing DRP2 schemas, command semantics, shader bind
layouts, or fixture expectations used by native DRP2/DVZR. Unsupported native features should
produce diagnostics rather than browser-only semantics.

If subagents are authorized, use one explorer for fixture/preflight gaps and one worker for
example/web page polish. Keep command semantics and WGSL/runtime changes local. Validate with git
diff --check, just spec-check, and the relevant WebGPU preflight/browser runner.
```


### Prompt: Example Audit And Polish

```text
You own the example audit/polish lane. Read the checklist,
spec/scene/examples/EXAMPLE_PRIORITIZATION.md, spec/scene/examples/README.md, and
spec/scene/examples/EXAMPLE_RELEASE_STAGING.md.

Goal: improve one release-target C example using already-implemented features. Prefer examples that
pressure axes, colorbars, linked panels, mesh/sphere/materials, volume, EDL, or WebGPU without
adding new core semantics.

Conflict avoidance: do not change src/scene, src/drp2, shaders, or public APIs unless fixing a
confirmed bug with a focused test. File implementation gaps in this checklist rather than solving
them inside example polish.

If subagents are authorized, split by independent examples or assign one explorer to compare the
example against the staging matrix. Validate with git diff --check, just build, and bounded runs of
the touched examples.
```


### Prompt: Runtime Hardening

```text
You own the runtime hardening lane. Read the checklist, NEXT_STEPS.md, and recent runtime
notes in agents/done that match the suspected subsystem.

Goal: fix one lifetime, resize, descriptor refresh, repeated-frame, destroy-path, or transient
object accumulation issue in the scene -> DRP2 -> vklite/canvas/app path.

Conflict avoidance: do not change visual feature semantics while fixing runtime behavior. Avoid
DRP2 schema changes unless the bug proves the command contract is wrong. Coordinate with feature
agents before touching shared request/frame loops or shader layout decisions.

If subagents are authorized, use explorers for independent audits such as descriptor lifetimes,
destroy paths, and repeated-frame allocations. Keep the actual fix local unless the write scope is
clearly isolated. Validate with git diff --check, just build, the narrow subsystem tests, and a
bounded GLFW/offscreen smoke when graphics paths are touched.
```


### Prompt: API Inventory And Docs

```text
You own the API inventory/docs lane. Read the checklist and
agents/now/RELEASE.md.

Goal: classify public/internal/experimental C API surface, document ownership/destroy rules, and
sync docs with actual v0.4 behavior. Prefer read-only audits and markdown/header comment updates.

Conflict avoidance: do not rename or redesign APIs while feature agents are active unless the task
explicitly asks for an API cleanup. Record proposed breaking changes separately when they would
touch active implementation lanes.

If subagents are authorized, use explorers by module: scene/app, DRP2/runtime, low-level graphics,
and examples/docs. Avoid parallel workers editing the same headers. Validate with git diff --check
and just build if headers or Doxygen comments changed.
```


### Prompt: DRP2 Contract And Fixture Maintenance

```text
You own the DRP2 contract/fixture lane. Read spec/drp2/AGENT_SPEC_PHASE.md, the checklist, spec/drp2
docs, schema files, and tools/drp2_fixture_runner.py.

Goal: keep the active DRP2 2.0 command surface, prose, schemas, fixtures, C serialization, and
fixture runner aligned for one concrete contract gap.

Conflict avoidance: coordinate with WebGPU and DVZR agents before changing command fields,
serialization, or fixture expectations. Do not introduce Vulkan-specific terms into public DRP2
definitions, and do not change scene semantics unless a scene pressure fixture requires it.

If subagents are authorized, use one explorer for schema/prose mismatch and one explorer for C
serialization/validation mismatch. Keep schema or C edits in one local write scope. Validate with
git diff --check, just spec-check, and just test drp2 when C code changes.
```


### Prompt: DVZR Recording And Replay

```text
You own the DVZR recording/replay portability lane. Read the DVZR updates in
agents/now/NEXT_STEPS.md, spec/drp2/AGENT_SPEC_PHASE.md, and relevant src/drp2 recording code.

Goal: improve one recording/replay slice: portable command coverage, app recording hooks, replay
target setup, raw-fallback diagnostics, player behavior, or example validation.

Conflict avoidance: coordinate with DRP2 contract/schema owners before changing portable command
serialization. Do not alter scene visual semantics or app frame loops beyond recording/replay hooks.

If subagents are authorized, use one explorer for raw fallback coverage and one worker for a
disjoint replay/example update. Keep recording format and loader/runtime changes local. Validate
with git diff --check, just build, focused drp2_recording tests, and a replay/player smoke.
```


### Prompt: Render-Contract And Technique Hardening

```text
You own the render-contract/technique hardening lane. Read
agents/done/RENDER_CONTRACT_RESOLVER_AUDIT.md, the checklist, and the active scene technique tests.

Goal: strengthen one active technique contract or readback case for source-over, WBOIT, depth peel,
MSAA, EDL, SSAO, volume occlusion, scene occlusion, or G-buffer.

Conflict avoidance: do not polish visual-family public behavior at the same time as changing pass
roles, resource ids, graph ordering, or shader variants. Coordinate with material/shader and WebGPU
agents before changing bind layouts or shader ABI.

If subagents are authorized, use explorers for independent contract/readback gaps by technique.
Delegate implementation only if each worker owns a disjoint technique/test file set. Validate with
git diff --check, just build, focused scene/drp2 technique tests, and relevant offscreen readbacks.
```


### Prompt: Material And Shader ABI Polish

```text
You own the material/shader ABI polish lane. Read the checklist, NEXT_STEPS.md, shader ABI
notes in recent commits/docs, and active material-model examples/tests.

Goal: improve one material or shader ABI slice: uniform layout, vertex attribute descriptors,
generated shader variants, material controls, GLSL/WGSL parity, or example coverage.

Conflict avoidance: coordinate before changing binding layouts used by DRP2, WebGPU, or technique
contracts. Do not fold unrelated visual-family polish into shader ABI work.

If subagents are authorized, use one explorer for GLSL/WGSL layout parity and one worker for a
disjoint example/test update. Keep ABI and core shader edits local. Validate with git diff --check,
just build, focused scene/drp2 tests, shader generation checks if present, and one visible example.
```


### Prompt: Selection And Request Payload Widening

```text
You own the selection/request payload lane. Read the checklist,
spec/scene/validation/IMAGE_PICKING_RECOVERY.md, spec/scene/interaction/PICKING.md, and
spec/scene/interaction/SELECTION.md.

Goal: improve one interaction slice: point/image request robustness, marker picking, richer payload
fields, highlight styling, linked-panel probe state, or explicit deferrals for unsupported picking.

Conflict avoidance: do not change the app frame loop or request executor broadly while runtime
hardening agents are active. Do not add mesh/path/volume/text picking unless explicitly scoped.

If subagents are authorized, use one explorer for request lifecycle tests and one worker for a
disjoint example/highlight test. Keep request executor or payload structure changes local. Validate
with git diff --check, just build, focused scene request tests, and app/offscreen pick/probe smoke.
```


### Prompt: Test-Runner Scheduling Follow-Up

```text
You own the test-runner scheduling follow-up lane. Read
agents/soon/tooling/TEST_RUNNER_SCHEDULING.md and the checklist.

Goal: land one scheduling or reporting slice: process-level sharding, child JSON aggregation,
CI orchestration, exact-case rerun ergonomics, or remaining skip cleanup.

Conflict avoidance: avoid editing the same test registration blocks that feature agents are using.
Do not change subsystem behavior while migrating test metadata. Keep generic runner logic separate
from Datoviz-specific Vulkan/GLFW/video skip probes.

If subagents are authorized, split by independent modules or use explorers to audit remaining
legacy skips. Delegate workers only when each owns a distinct module test file. Validate with git
diff --check, cmake --build build --target touched runners, --list/JSON smoke, and representative
focused tests.
```


### Prompt: Performance And Long-Run Smoke

```text
You own the performance/long-run smoke lane. Read the checklist, NEXT_STEPS.md, and any recent
runtime hardening notes for the target path.

Goal: identify and fix or document one performance or churn issue: immediate-present FPS
regression, repeated-frame allocations, descriptor churn, unexpected DRP2 stream changes, or
long-running GLFW/offscreen instability.

Conflict avoidance: do not change feature semantics to improve a benchmark. Coordinate with
runtime hardening before touching descriptor/resource lifetimes and with app/example agents before
changing example loops.

If subagents are authorized, use explorers for trace analysis, allocation/churn audit, and benchmark
comparison. Keep fixes local after the evidence is clear. Validate with git diff --check, just
build, focused tests, bounded live smoke, and trace/FPS notes.
```


### Prompt: Capture, Video, And Gallery Artifacts

```text
You own the capture/video/gallery artifact lane. Read the checklist,
RELEASE.md, and example staging docs.

Goal: improve one raster capture, frame-sequence/video, gallery artifact, or backend skip behavior
needed for release examples. Keep publication/vector export out of scope.

Conflict avoidance: do not redesign scene/app rendering or video APIs while feature agents are
active. Treat codec/backend availability as runner-visible skips rather than hard failures.

If subagents are authorized, use one explorer for current capture/video examples and one worker for
a disjoint gallery artifact script/example. Validate with git diff --check, just build, focused
video/app tests where available, and one artifact-producing smoke.
```


### Prompt: GUI-Driven Example Controls

```text
You own the GUI-driven example controls lane. Read the checklist and the staged example docs.

Goal: add or polish ImGui/example-side controls that exercise existing scene/app parameters such as
materials, EDL, SSAO, volume ranges, transparency, or camera state.

Conflict avoidance: do not promote the GUI module into a broad public API and do not add core
scene semantics solely for a control panel. Coordinate with material/technique agents before
changing controlled shader parameters.

If subagents are authorized, split by independent examples or use an explorer to audit existing GUI
patterns. Keep core implementation changes local and minimal. Validate with git diff --check, just
build, and bounded runs of touched GUI examples.
```


### Prompt: Release Documentation And Website Staging

```text
You own the release documentation/website staging lane. Read
agents/now/RELEASE.md, the checklist, example staging docs, and current docs.

Goal: update user-facing documentation, capability matrices, known-gap notes, public/private labels,
or website/gallery staging based on behavior already implemented in C.

Conflict avoidance: do not claim unfinished behavior. Do not edit active C implementation while
feature agents are landing slices unless the doc task explicitly includes small header comments.

If subagents are authorized, use explorers by doc area: API surface, examples/gallery, WebGPU
status, and release gaps. Delegate markdown-only workers only on disjoint files. Validate with git
diff --check and any docs/link/gallery checks available in the repo.
```


## Definition Of Done For A Slice

A C implementation slice is done when:

1. public scope and deferrals are documented,
2. C headers and source follow current naming/ownership/comment rules,
3. focused tests cover create/update/destroy and at least one failure path,
4. repeated-frame behavior is tested when retained resources are involved,
5. visible features have an offscreen or bounded GLFW smoke,
6. `git diff --check` passes,
7. the narrow relevant `just test <filter>` passes,
8. this checklist is updated with status and validation notes.
