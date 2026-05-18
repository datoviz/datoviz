# Scene Screen-Space Effects Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-17`
> - **Purpose:** stage outline rendering, screen-space edge enhancement, and bloom on the existing
>   scene FramePlan graph and technique-planning path.


## Context

The durable design proposal is
[spec/scene/proposals/active/SCREEN_SPACE_EFFECTS_DESIGN.md](../../spec/scene/proposals/active/SCREEN_SPACE_EFFECTS_DESIGN.md).

Do not add a parallel renderer, visual-private postprocess path, or ad-hoc Vulkan path. These
effects should use:

```text
retained scene state
  -> technique planning
  -> FramePlan graph resources and passes
  -> DRP2 command stream
  -> vklite/canvas runtime
```

Relevant existing lanes:

1. [SCENE_TECHNIQUES_MATERIALS_PLAN.md](SCENE_TECHNIQUES_MATERIALS_PLAN.md)
2. [SCENE_SSAO_IMPLEMENTATION_PLAN.md](SCENE_SSAO_IMPLEMENTATION_PLAN.md)
3. [SCENE_SSAO_QUALITY_PLAN.md](SCENE_SSAO_QUALITY_PLAN.md)
4. [FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md](FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md)


## Implementation Order

Preferred order:

1. outline rendering for hover/selection,
2. screen-space edge enhancement,
3. bloom.

Rationale:

1. outline rendering is the highest interaction value and aligns with picking/selection work,
2. edge enhancement reuses the G-buffer/SSAO foundation and can share fullscreen pass plumbing,
3. bloom is useful but more presentation-oriented and needs a careful quantitative-view policy.


## Slice 1: Technique State And Public Descriptors

Scope: retained state only, no rendering changes.

Expected work:

1. add internal panel technique state for outline, edge enhancement, and bloom descriptors;
2. expose typed default descriptor constructors and panel setters only after API naming review;
3. keep all effects default-off;
4. mark panel/frame dirty when a descriptor changes;
5. add tests that descriptor defaults do not change the default emitted FramePlan.

Validation:

```text
just build
just test scene
git diff --check
```


## Slice 2: Outline Mask Or Object-ID Foundation

Scope: create the reusable identity/mask source needed by outlines.

Expected work:

1. choose the first outline identity source: selection mask, object-id texture, or both;
2. add visual pass capability flags for object-id or mask participation;
3. add graph resources for per-panel outline mask or ID targets;
4. add an outline-source render pass that draws only eligible selected/hovered targets;
5. respect panel viewport/scissor metadata;
6. ensure multi-panel figures allocate and use independent outline intermediates.

Preferred first path:

1. start with object-level selected/hovered visuals;
2. support item-level masks only for visual families that already have stable item identity;
3. defer transparent visual outlines until the opaque path is tested.

Validation:

```text
just build
just test test_scene_pick
just test scene
git diff --check
```


## Slice 3: Outline Composite

Scope: render visible outlines over the panel color target.

Expected work:

1. add fullscreen outline shaders in GLSL and WGSL when needed;
2. add graph resources for outline edge output if the implementation uses a separate edge pass;
3. add an outline composite pass after base scene composition and before external UI;
4. keep outline width in physical pixels or define the exact logical-to-physical mapping;
5. add offscreen image-difference tests for selected and hovered object outlines;
6. add multi-panel regression coverage proving no outline bleeding across panels.

Validation:

```text
just build
just test scene
just test drp2
git diff --check
```


## Slice 4: Edge Enhancement

Scope: add an optional depth/normal discontinuity pass.

Expected work:

1. reuse existing G-buffer normal/depth resources when present;
2. fall back to depth-only edges only when the descriptor and capability policy allow it;
3. add graph roles/resources for edge mask and edge composite;
4. run after SSAO/EDL composite by default;
5. respect panel viewport/scissor boundaries;
6. add tests that enabling edge enhancement requests only the required graph resources.

Shader behavior:

1. sample neighboring depth and normal values inside the panel region;
2. compare against descriptor thresholds;
3. composite a controlled color/strength contribution over the shaded scene;
4. avoid sampling outside the active panel rectangle.

Validation:

```text
just build
just test scene
just test drp2
git diff --check
```


## Slice 5: Bloom

Scope: add opt-in panel bloom as a presentation effect.

Expected work:

1. add bright-pass extraction from resolved panel color;
2. add separable blur or mip-chain blur resources;
3. composite bloom before outlines so outlines remain crisp;
4. decide whether the first implementation is LDR-thresholded or introduces an HDR intermediate;
5. add deterministic offscreen image-difference tests with and without bloom;
6. document export inclusion/exclusion behavior.

Validation:

```text
just build
just test scene
git diff --check
```


## Runtime Notes

Keep graph lowering generic where possible:

1. graph resources should drive texture creation and usage flags;
2. graph passes should drive pass ordering and sampled reads;
3. effect-specific runtime code should be limited to shader/pipeline/bind-group preparation and
   fullscreen draw dispatch;
4. descriptor refresh must reuse the existing graph-resource and texture-recreation path;
5. borrowed canvas frame targets must remain borrowed and must not be destroyed by scene/runtime
   effect code.

If an effect needs a new DRP2 feature, write the DRP2 spec change first and add fixtures before
lowering scene work to backend-specific commands.


## Tests And Examples

Focused tests should cover:

1. default-off FramePlan parity,
2. opt-in graph resource and pass creation,
3. graph dependency order,
4. DRP2 runtime lowering smoke coverage,
5. multi-panel scissor correctness,
6. offscreen image differences,
7. hover/selection outline state changes.

Useful examples:

1. mesh/sphere selection outline example,
2. dense surface edge-enhancement example with SSAO comparison,
3. astronomy or fluorescence-style bloom example.


## Completion Criteria

This lane is complete when:

1. all three effects have retained panel state,
2. outline and edge enhancement have graph-backed runtime paths and tests,
3. bloom has an opt-in graph-backed runtime path and tests,
4. export behavior is documented,
5. stable semantics are promoted from the proposal into `spec/scene/semantics/EFFECTS.md` or an
   equivalent specialized spec file.
