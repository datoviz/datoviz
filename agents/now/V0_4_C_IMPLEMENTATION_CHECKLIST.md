# Datoviz v0.4 C Implementation Checklist

> **Execution Status**
> - **Status:** `ACTIVE OPERATIONAL TRACKER`
> - **Updated on:** `2026-05-18`
> - **Purpose:** tell agents where v0.4 C implementation stands, what to pick next, and which
>   lanes can run in parallel.

This file is the branch-level operational checklist for finishing the C implementation of v0.4.
It complements:

1. [`V0_4_NEXT_STEPS.md`](V0_4_NEXT_STEPS.md): current technical context and recent validation.
2. [`V0_4_RELEASE_READINESS_PLAN.md`](V0_4_RELEASE_READINESS_PLAN.md): release scope, API policy,
   v0.3 regression checklist, and post-feature-completion work.
3. [`../soon/SCENE_EXAMPLE_PRIORITIZATION.md`](../soon/SCENE_EXAMPLE_PRIORITIZATION.md): example
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

**Next critical-path item:** rendered text first slice.

Reason: text unlocks axes, tick labels, colorbars, annotations, pinned readouts, legends, and most
polished 2D examples. v0.3 already exposed visible glyph/text and axes/colorbars; v0.4 should avoid
regressing those visible capabilities even though source compatibility is not required.

Primary specs:

1. [`../../spec/scene/slices/TEXT_RENDERING_SLICE.md`](../../spec/scene/slices/TEXT_RENDERING_SLICE.md)
2. [`../../spec/scene/semantics/TEXT.md`](../../spec/scene/semantics/TEXT.md)
3. [`../soon/SCENE_TEXT_GLYPH_PLAN.md`](../soon/SCENE_TEXT_GLYPH_PLAN.md)


## Critical Path

### 0. v0.4 Scope And Regression Baseline

Status: `Done`

Outcome:

1. WebGPU/WASM is in v0.4 as experimental scope.
2. OO plotting is external GSP/VisPy2 scope.
3. Publication/vector export is external GSP/Matplotlib scope.
4. v0.3 visible capability regressions to avoid are listed in
   [`V0_4_RELEASE_READINESS_PLAN.md`](V0_4_RELEASE_READINESS_PLAN.md).

Last validation:

1. `git diff --check` passed before commit `10d87091`.


### 1. Rendered Text First Slice

Status: `Next`

Goal:

Implement visible retained text through scene -> FramePlan -> DRP2 -> vklite/canvas.

Required v0.4 slice:

1. built-in fallback font,
2. visible single-line UTF-8 strings,
3. run-level size and color,
4. screen-space placement,
5. simple data-space anchoring,
6. panel viewport/scissor clipping,
7. offscreen and GLFW rendering,
8. retained update and destroy behavior,
9. focused scene tests plus one app/offscreen smoke.

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


### 2. 2D Axes And Ticks

Status: `Blocked` on rendered text for visible labels; geometry/tick work can begin in parallel.

Goal:

Restore visible 2D axes quality without copying v0.3 API shape.

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
2. [`../soon/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md`](../soon/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md)


### 3. Continuous Colorbars

Status: `Blocked` on rendered text for title/tick labels; ramp layout can begin in parallel.

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


### 4. Basic Annotations And Readouts

Status: `Blocked` on rendered text.

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


### 5. Grid Layout And Linked Panels

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

1. [`../soon/SCENE_GRID_LAYOUT_SPEC.md`](../soon/SCENE_GRID_LAYOUT_SPEC.md)
2. [`../../spec/scene/core/PANEL_LAYOUT.md`](../../spec/scene/core/PANEL_LAYOUT.md)


### 6. Visual Family Polish Pass

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
9. `text`/`glyph`: covered by the rendered text first slice.

Primary docs:

1. [`VISUAL_FAMILY_IMPLEMENTATION_DECISIONS.md`](VISUAL_FAMILY_IMPLEMENTATION_DECISIONS.md)
2. [`../soon/SCENE_EXAMPLE_PRIORITIZATION.md`](../soon/SCENE_EXAMPLE_PRIORITIZATION.md)


### 7. Selection, Pick, Probe, And Highlight

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

1. [`IMAGE_PICKING_RECOVERY_PLAN.md`](IMAGE_PICKING_RECOVERY_PLAN.md)
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

1. [`../soon/DRP2_WEBGPU_SUPPORT_PLAN.md`](../soon/DRP2_WEBGPU_SUPPORT_PLAN.md)
2. [`../soon/SCENE_WASM_WEBGPU_PORT_PLAN.md`](../soon/SCENE_WASM_WEBGPU_PORT_PLAN.md)


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

1. [`../soon/SCENE_EXAMPLE_PRIORITIZATION.md`](../soon/SCENE_EXAMPLE_PRIORITIZATION.md)
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

1. [`V0_4_RELEASE_READINESS_PLAN.md`](V0_4_RELEASE_READINESS_PLAN.md)


## Parallel Work Guidance

Good parallel lanes right now:

1. **Text implementation:** `src/scene`, `include/datoviz/scene/text.h`, scene shaders, focused
   text tests.
2. **Axes geometry/tick planning:** axis tick generation, semantic state, tests that do not require
   glyph rendering yet.
3. **Colorbar ramp planning:** scale/colormap/ramp layout and diagnostics, excluding final text
   labels until text lands.
4. **WebGPU command parity:** `examples/webgpu`, DRP2 fixture/preflight work, no scene API churn.
5. **Example audit/polish:** C examples and gallery harnesses that use already-implemented
   features.
6. **Runtime hardening:** DRP2/vklite/app lifetime bugs with focused tests.
7. **API inventory/docs:** read-only or markdown-only work that does not alter active C code.

Avoid parallel edits that touch the same write scope:

1. two agents both changing the same scene visual emission code,
2. text and axes both rewriting the same FramePlan contribution structures,
3. WebGPU and DRP2 schema/native validation changes to the same command without coordination,
4. app/runtime hardening and feature emission changing the same request or frame loop.


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
