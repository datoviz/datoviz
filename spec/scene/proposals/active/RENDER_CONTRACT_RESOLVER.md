> **Execution Status**
> - **Status:** `ACTIVE IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-17`
> - **Purpose:** track the active scene render-contract resolver refactor that prevents
>   transparency, occlusion, depth, and DRP2 lowering regressions.

# Scene Render Contract Resolver

This proposal defines the intended architecture for resolving scene visual requirements into a
generic pass contract before FramePlan and DRP2 emission.

The immediate motivation is the recent class of regressions around volume rendering, volume
occlusion, source-over transparency, scene occlusion, and transparent mesh shells. The same visual
facts are currently inferred in several layers, which makes pass, pipeline, attachment, and bind
group behavior easy to desynchronize.


## Current Implementation State

As of `2026-05-17`, this is no longer just a design sketch. The repository has an
initial passive contract layer:

1. `src/scene/render_contract.h` defines internal draw, attachment, and pass contract structs.
2. `src/scene/render_contract.c` resolves retained visual draws into passive contracts and
   validates generic pass invariants.
3. `src/scene/scene.c` runs FramePlan contract validation after scene emission.
4. `src/scene/scene_emit.c` now asks draw contracts whether source-over transparent draws need a
   depth attachment instead of relying only on local pass-capability flags.
5. `src/scene/technique.c` has graph emitters for WBOIT, depth peeling, ordinary source-over
   blending, volume occlusion, scene occlusion, G-buffer, EDL, SSAO, and MSAA-oriented paths.
6. `src/scene/frame_plan_runtime.c` has ordinal matching between repeated render nodes and repeated
   graph passes with the same `work_label`, which matters for split technique passes.
7. `src/scene/tests/scene_graph.c` contains the first contract-focused fixtures for WBOIT,
   depth peeling, source-over blend, volume occlusion, scene occlusion, G-buffer, EDL, SSAO, and
   several mixed transparent cases.

The refactor is therefore in the middle of Phase 2 and Phase 3 below. Keep new transparency,
occlusion, and depth work aligned with this document rather than adding new independent inference
paths.


## Current Guardrails

The following rules are active design constraints, not optional preferences:

1. `DVZ_ALPHA_BLENDED` means ordinary source-over color blending. It depth-tests when the visual
   requests depth testing, but it does not write normal depth.
2. If transparent geometry must create depth for a later operation, model that as an explicit
   prepass, occlusion pass, depth-peeling path, WBOIT path, or future named technique. Do not make
   ordinary source-over blending write normal depth implicitly.
3. A graph pass may allocate or clear a transient depth attachment so blended draws can depth-test
   against a well-defined buffer. That is separate from pipeline depth writes.
4. Lowering code may assert contract facts and fill mechanical DRP2 details. It should not silently
   choose a different alpha, depth, blend, attachment, or bind-group policy.
5. When a test and this document disagree, update the test or implementation so the contract model
   is the authority, then record any intentional semantic change here.


## Immediate Next Steps

The highest-value next steps are:

1. Add a small resolver-matrix test helper so mixed visual cases can assert draw/pass contracts
   without repeating long scene setup and graph scans.
2. Extend `DvzSceneDrawContract` with explicit blend and pipeline policy fields so the contract
   covers source-over, segment coverage blend, WBOIT accumulation blend, and future additive or
   premultiplied modes.
3. Make DRP2 stream validation compare emitted pipeline depth/blend state against the resolved
   contract, not only against the active render-pass attachment shape.
4. Move WBOIT and depth-peeling attachment/blend details into named pass-contract builders, while
   keeping the technique-specific shaders and formats explicit.
5. Add offscreen readback tests for dense blended point sprites, volume + source-over mesh,
   volume + WBOIT mesh, and volume + depth-peel mesh so stream-shape correctness is not mistaken
   for visual correctness.
6. Remove remaining duplicated lower-layer decisions once the contract layer owns the corresponding
   facts.


## Problem

The active scene pipeline currently spreads one logical draw contract across multiple files:

1. `visual_pipeline.c` resolves visual pass capabilities, shader variants, pipeline depth state,
   and bind-group layout needs.
2. `scene_emit.c` decides which FramePlan render pass a visual belongs to.
3. `technique.c` emits graph resources and pass attachments for transparency, occlusion, G-buffer,
   SSAO, and related techniques.
4. `frame_plan_runtime.c` lowers render nodes and graph passes to DRP2 command streams.

Each layer has reasonable local logic, but they can drift. A typical failure mode is:

1. the pipeline resolver chooses a depth-capable blended pipeline,
2. the frame graph emits a transparent pass without depth,
3. DRP2 or the native runtime rejects `SetPipeline` because the pipeline and pass attachments no
   longer match.

Another failure mode is visual correctness rather than runtime validation:

1. a volume slice samples volume occlusion correctly in isolation,
2. a mesh shell renders correctly in isolation,
3. the combined volume + slice + transparent mesh + scene occlusion state draws in the wrong pass
   order or samples the wrong depth resource.

These are not example-specific problems. They are symptoms of duplicated render-contract inference.


## Design Position

The preferred long-term fix is a middle ground:

1. use generic draw, pass, attachment, and resource contracts,
2. keep named built-in techniques for WBOIT, depth peeling, volume occlusion, scene occlusion,
   G-buffer, SSAO, and future screen-space effects,
3. validate all technique outputs with the same generic invariants,
4. avoid a fully dynamic render-graph plugin system for now.

This keeps rendering semantics explicit and readable while removing duplicated pass and pipeline
reasoning from lower layers.


## Goals

1. Make one resolver the authority for visual draw requirements.
2. Make one pass contract the authority for render-pass attachment requirements.
3. Ensure a pass is created from the union of the requirements of the draws inside it.
4. Let named techniques emit generic contracts rather than directly coordinating several lower
   layers.
5. Make invalid combinations fail as resolver or contract validation errors before DRP2 runtime
   execution.
6. Add automated minimal fixtures that cover meaningful combinations with known expected results.
7. Add offscreen readback tests for visual correctness where stream shape alone is insufficient.


## Non-Goals

1. Do not create a public technique plugin API in this slice.
2. Do not implement shader reflection-driven layouts yet.
3. Do not replace DRP2 or the existing FramePlan graph model.
4. Do not make example-specific GUI states the primary regression fixtures.
5. Do not silently adapt requested transparency techniques unless capability adaptation explicitly
   records the fallback.


## Pipeline Shape

The intended architecture is:

```text
retained visuals
  -> draw contract resolver
  -> named technique selection and expansion
  -> generic pass contract graph
  -> FramePlan
  -> DRP2 stream
  -> runtime
```

Lower layers should consume resolved contracts. They should not re-decide whether a visual needs
depth, samples occlusion, writes depth, uses source-over blending, or needs a particular bind group.


## Contract Vocabulary

The exact C names may change, but the concepts should remain stable.

```c
typedef enum DvzScenePassKind
{
    DVZ_SCENE_PASS_KIND_RASTER,
    DVZ_SCENE_PASS_KIND_FULLSCREEN,
    DVZ_SCENE_PASS_KIND_COMPUTE,
} DvzScenePassKind;

typedef enum DvzSceneAttachmentRole
{
    DVZ_SCENE_ATTACHMENT_COLOR,
    DVZ_SCENE_ATTACHMENT_DEPTH,
    DVZ_SCENE_ATTACHMENT_STORAGE,
    DVZ_SCENE_ATTACHMENT_SAMPLED,
} DvzSceneAttachmentRole;

typedef struct DvzSceneAttachmentUse
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    DvzSceneAttachmentRole role;
    VkFormat format;
    uint32_t sample_count;
    bool read;
    bool write;
    bool clear;
    bool preserve;
} DvzSceneAttachmentUse;

typedef struct DvzSceneDrawContract
{
    DvzVisualType visual_type;
    DvzAlphaMode alpha_mode;
    DvzFramePlanRenderPassRole pass_role;

    bool depth_test;
    bool depth_write;
    bool samples_depth;
    bool samples_volume_occlusion;
    bool samples_scene_occlusion;
    bool writes_volume_occlusion_depth;
    bool writes_scene_occlusion_depth;

    bool needs_common_set;
    bool needs_material_set;
    bool needs_image_set;
    bool needs_volume_set;
    bool needs_scene_occlusion_set;

    DvzSceneShaderFeatures shader_features;
    DvzScenePipelineFeatures pipeline_features;
} DvzSceneDrawContract;

typedef struct DvzScenePassContract
{
    DvzScenePassKind kind;
    DvzFramePlanRenderPassRole role;
    char id[DVZ_SCENE_LABEL_SIZE];
    char panel_id[DVZ_SCENE_LABEL_SIZE];

    DvzSceneAttachmentUse attachments[DVZ_SCENE_MAX_ATTACHMENTS];
    uint32_t attachment_count;

    DvzSceneDrawContract draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;

    bool source_over_blend;
    bool wboit_accumulation;
    bool depth_peel;
    bool fullscreen_resolve;
} DvzScenePassContract;
```

These structs should stay internal. Public API should continue to expose visual alpha modes,
technique settings, and panel/visual configuration rather than these implementation details.


## Named Technique Contracts

Technique code should remain named and specific. The generic part is the contract it emits, not the
math or shader semantics of the technique.

Source-over blending:

1. emits a `transparent_blend` raster pass,
2. writes the final target with source-over blend state,
3. attaches depth when any draw in the pass depth-tests or samples depth,
4. does not write depth for blended geometry unless a future explicit mode requires it.

WBOIT:

1. emits an accumulation raster pass,
2. writes accumulation and weight/revealage targets,
3. reads opaque depth when transparent draws need depth testing,
4. emits a fullscreen resolve pass that samples accumulation targets and writes the final target.

Depth peeling:

1. emits explicit peel init and iteration raster passes,
2. owns the front/back depth and color resources required by the algorithm,
3. emits a fullscreen composite pass,
4. keeps depth-peeling shader and raster state details inside the named technique.

Volume occlusion:

1. emits a volume occlusion prepass that writes a sampled depth-like texture,
2. marks volume-slice draws that sample that texture,
3. guarantees the consumer pass has the right sampled resource and bind group layout.

Scene occlusion:

1. emits a scene occlusion prepass from visible scene occluders,
2. writes the scene occlusion depth texture and any fixed-function depth attachment required by
   the prepass,
3. marks occluded draws that sample the scene occlusion texture,
4. keeps alpha-aware occlusion-depth shader behavior inside the named technique.

G-buffer, SSAO, EDL, and MSAA:

1. should follow the same contract pattern,
2. may remain explicit technique code,
3. must declare all reads, writes, formats, sample counts, and draw requirements through pass
   contracts before lowering.


## Required Invariants

The contract resolver and lowering path must enforce these invariants:

1. A render pass attachment set is the union of the attachment requirements of its draws and
   technique operations.
2. Every pipeline emitted for a pass matches that pass color attachment count, color formats, depth
   presence, depth format, and sample count.
3. A draw that depth-tests is only emitted into a pass with a depth attachment.
4. A draw that samples a resource has a graph read edge from a resource produced earlier in the
   same frame or borrowed externally.
5. A visual that samples volume or scene occlusion has the required bind group layout and bind group
   at the expected set.
6. Technique pass ordering is derived from resource dependencies, with named technique constraints
   only where the algorithm truly requires fixed order.
7. Lowering layers may assert resolved facts, but must not infer alternate facts.
8. Capability adaptation must be explicit and diagnostic. Missing WBOIT, depth-peeling,
   render-target sampling, independent blend, or color attachment support must not silently change
   the visual contract.


## Testing Strategy

The primary regression fixtures should be small automated scene fixtures, not interactive examples.
Use synthetic 64x64 or 128x128 scenes with deterministic geometry, colors, and readbacks.

Tier 1: pure resolver matrix tests.

1. No GPU required.
2. Build retained scenes with small visual combinations.
3. Resolve draw and pass contracts.
4. Assert expected pass roles, attachment requirements, bind group requirements, and draw ordering.

Tier 2: FramePlan and DRP2 validation tests.

1. Lower the same fixtures to FramePlan and DRP2.
2. Run FramePlan graph validation and `dvz_drp2_validate_stream()`.
3. Assert every `SetPipeline` is compatible with the active render pass.
4. Assert sampled occlusion resources have producer-before-consumer order.

Tier 3: native runtime smoke tests.

1. Execute the same fixtures through vklite in small offscreen targets.
2. Cover stable resource recreation across two frames where visibility, alpha mode, or occlusion
   flags change.
3. Treat any DRP2 runtime validation error as a test failure.

Tier 4: offscreen readback correctness tests.

1. Use broad but meaningful pixel assertions, not fragile full-image golden files.
2. Put colored planes, slices, and transparent shells at known positions.
3. Assert that enabling volume occlusion changes slice pixels.
4. Assert that a transparent mesh shell contributes to pixels where it should appear in front of a
   slice.
5. Assert that hidden or fully transparent occluder regions do not affect scene occlusion.

Recommended fixture matrix:

1. volume only,
2. volume + slice,
3. volume + slice + opaque mesh,
4. volume + slice + source-over blended mesh,
5. volume + slice + WBOIT mesh,
6. volume + slice + depth-peel mesh,
7. each relevant case with volume occlusion on and off,
8. each relevant case with scene occlusion on and off,
9. two-frame toggles for visibility, alpha, and occluder flags.

The fixture data should be minimal and generated in test code. The Allen mouse brain example remains
a valuable pressure test, but it should not be the only way to detect these regressions.


## Incremental Implementation Plan

Phase 1: add tests before moving logic.

1. Add minimal resolver-intent fixtures for volume, slice, mesh, transparency, and occlusion.
2. Add DRP2 validation fixtures for pass/pipeline compatibility.
3. Add at least one offscreen readback fixture for volume occlusion and transparent shell ordering.

Phase 2: introduce passive contracts.

1. Add internal draw and pass contract structs.
2. Populate them from the current logic without changing behavior.
3. Add assertions that the existing FramePlan and DRP2 stream match the passive contracts.

Phase 3: move source-over transparency first.

1. Use contracts for `DVZ_ALPHA_BLENDED` pass placement.
2. Use contracts for transparent blend depth attachment requirements.
3. Remove duplicated source-over `needs_depth` inference from lower layers.

Phase 4: move volume and scene occlusion.

1. Express occlusion prepasses and consumer reads as pass contracts.
2. Make occluded draw bind group requirements come from the draw contract.
3. Add two-frame recreation tests for occlusion resources and bind groups.

Phase 5: move WBOIT and depth peeling.

1. Express WBOIT accumulation and resolve passes as contracts.
2. Express depth-peeling init, iter, and composite passes as contracts.
3. Keep technique-specific shaders, formats, and blend equations in named technique code.

Phase 6: broaden validation and delete duplicate logic.

1. Make contract validation mandatory for scene emission tests.
2. Delete parallel `transparent_needs_depth`, `samples_depth`, and ad hoc lower-layer derivations
   once contracts own those facts.
3. Promote stable rules into `../../pipeline/FRAME_PLAN.md`, `../../semantics/TRANSPARENCY.md`, and
   `../../semantics/VISUAL_CONTRACT.md`.


## Acceptance Criteria

The refactor is successful when:

1. adding a new visual or technique requires declaring its draw and pass requirements in one place,
2. lower layers can assert but not re-infer depth, blend, attachment, or occlusion requirements,
3. every scene emission test runs contract and DRP2 stream validation,
4. the synthetic fixture matrix catches pass/pipeline mismatches before runtime execution,
5. offscreen readback tests catch visual regressions in occlusion and transparent ordering,
6. the Allen mouse brain example becomes a pressure test, not the first line of regression defense.


## Open Questions

1. Whether `DvzSceneDrawContract` should be stored in the FramePlan metadata or only used during
   emission.
2. Whether graph pass ordering should be entirely dependency-derived or preserve explicit technique
   ordering for readability.
3. How much capability fallback should happen in the resolver versus a separate adaptation pass.
4. Whether future custom visuals need a private extension point for declaring contracts without
   exposing the full internal technique system.
