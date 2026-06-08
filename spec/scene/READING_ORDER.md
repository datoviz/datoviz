# Scene Spec Reading Order

Read the scene spec in this order during review.


## 1. Orientation And High-Level Constraints

1. [core/REQUIREMENTS.md](core/REQUIREMENTS.md): scene goals, scope, and required runtime/DRP2
   support.
2. [core/RUNTIME_BOUNDARY.md](core/RUNTIME_BOUNDARY.md): what scene may and may not depend on.
3. [core/USE_CASES.md](core/USE_CASES.md): pressure-test scenarios to keep in mind while reviewing.


## 2. Core Scene Concepts And Public Shape

1. [core/OBJECT_MODEL.md](core/OBJECT_MODEL.md): stable scene concepts and ownership model.
2. [core/PANEL_LAYOUT.md](core/PANEL_LAYOUT.md): grid layout, free placement, fixed columns/rows,
   span, and tight layout.
3. [api/API_DESIGN.md](api/API_DESIGN.md): current preferred scene-facing defaults and resolved API
   decisions.
4. [api/API_SURFACE.md](api/API_SURFACE.md): public API shape policy and implemented-vs-draft
   boundary.
5. [api/API_IMPLEMENTATION_READINESS.md](api/API_IMPLEMENTATION_READINESS.md): checklist for the
   next public API pass.
6. [headers/README.md](headers/README.md): draft header index for pressure-testing the surface.
7. [slices/README.md](slices/README.md): implementation-ready work packets and readiness matrix.
8. [proposals/README.md](proposals/README.md): proposal taxonomy, promotion status, and indexes.
9. [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md): C-facing mapping and Python binding
   architecture.
10. [ROADMAP.md](ROADMAP.md): compact backlog and post-v0.4 direction.


## 3. Visual Semantics

1. [semantics/VISUAL_FAMILIES.md](semantics/VISUAL_FAMILIES.md): family taxonomy.
2. [semantics/VISUAL_CONTRACT.md](semantics/VISUAL_CONTRACT.md): shared producer contract across
   visuals.
3. [semantics/VISUAL_FAMILY_RULES.md](semantics/VISUAL_FAMILY_RULES.md): family-level
   mini-contracts.
4. [visuals/README.md](visuals/README.md): per-family data contracts.
5. [semantics/SCALES.md](semantics/SCALES.md): color, size, and opacity scale objects and colormap
   model.
6. [semantics/LIGHTING.md](semantics/LIGHTING.md): scene-level lighting model and upgrade path.
7. [semantics/AXES.md](semantics/AXES.md): axes and tick semantics.
8. [semantics/ANNOTATIONS.md](semantics/ANNOTATIONS.md): labels, guides, probes, overlays, and
   callouts.
9. [semantics/LEGENDS_AND_COLORBARS.md](semantics/LEGENDS_AND_COLORBARS.md): explanatory mapping
   semantics.
10. [semantics/TEXT.md](semantics/TEXT.md): text content, placement, resources, and DPI behavior.
11. [slices/TEXT_RENDERING_SLICE.md](slices/TEXT_RENDERING_SLICE.md): first rendered text work
    packet.
12. [implementation/TEXT_SHAPING_ATLAS.md](implementation/TEXT_SHAPING_ATLAS.md): text shaping,
    layout, atlas, cache, and DRP2 emission contract.
13. [slices/ANNOTATION_LABEL_SLICE.md](slices/ANNOTATION_LABEL_SLICE.md): first rendered label
    annotation work packet.
14. [slices/COLORBAR_RENDERING_SLICE.md](slices/COLORBAR_RENDERING_SLICE.md): first rendered
    colorbar work packet.


## 4. Data, Transforms, Planning, And Runtime Handoff

1. [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md): logical resource model and F64 data
   ingestion policy.
2. [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md): per-attribute data granularity
   and mutability hints.
3. [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md): normalization, panel-transform
   pipeline, and CPU precision policy.
4. [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md): triangulation, curve
   tessellation, simplification, hull, boolean ops, and SDF/MSDF pipeline.
5. [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md): canonical producer-side frame artifact.
6. [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md): update/build/emit flow.
7. [implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md](implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md):
   active migration from raw scene-emitted streams to `DvzSceneFrameArtifact`.


## 5. Validation, Adaptation, Interaction, And Diagnostics

1. [validation/VALIDATION.md](validation/VALIDATION.md): validation rules and error classes.
2. [validation/ADAPTATION.md](validation/ADAPTATION.md): explicit fallback and simplification
   policy.
3. [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md): dirty scopes and
   reuse rules.
4. [interaction/GPU_QUERY_SYSTEM.md](interaction/GPU_QUERY_SYSTEM.md): GPU-only panel query
   architecture replacing public pick/probe.
5. [interaction/PICKING.md](interaction/PICKING.md): picking identity and readback behavior.
6. [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md): event routing and interaction
   ownership.
7. [interaction/ANIMATION.md](interaction/ANIMATION.md): scene clock, animation objects, easing,
   camera keyframes, and video export.
8. [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md): boundary with app-owned UI frameworks.
9. [interaction/SELECTION.md](interaction/SELECTION.md): selection state, highlight rendering,
   cross-visual linking, and lasso.
10. [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md): still image capture, render scale, and
    panel-as-texture.
11. [integration/HIGH_DPI.md](integration/HIGH_DPI.md): device pixel ratio, logical vs physical
    pixels, and DPI changes.
12. [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md): alpha modes and render pass structure.
13. [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md): non-linear coordinate
    transforms.
14. [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md): v0.4 decision to leave
    publication-oriented vector export to GSP/Matplotlib.
15. [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md): user-defined visual families.
16. [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md): threading model and async data
    handoff.
17. [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md): scene event observer system.
18. [semantics/CLIPPING.md](semantics/CLIPPING.md): per-visual clip modes.
19. [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md): shared diagnostic shape.


## 6. Worked Examples

1. [examples/README.md](examples/README.md): entry point for worked examples.
2. [examples/PLANNING.md](examples/PLANNING.md): release staging, priorities, and current gaps.
3. [examples/FIXTURES.md](examples/FIXTURES.md): compact validation fixture matrix.
4. [Core release proofs][core-release-proofs]: core release-proof scenario set.
5. [examples/scenarios/v04_required/SHOWCASES.md](examples/scenarios/v04_required/SHOWCASES.md).
6. [API pressure sketches][api-pressure-sketches].


## Document Index

Current implementation orientation:

1. [api/API_SURFACE.md](api/API_SURFACE.md): public API shape policy and implemented-vs-draft
   boundary.
2. [api/API_IMPLEMENTATION_READINESS.md](api/API_IMPLEMENTATION_READINESS.md): current readiness
   checklist and remaining public API implementation gaps.
3. [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md): C object mapping, app/runtime
   wiring, Python binding architecture, and GPU preparation notes.
4. [ROADMAP.md](ROADMAP.md): compact backlog, release-proof order, and post-v0.4 direction.
5. [Scene visual boundary guardrails][scene-visual-boundary-guardrails]: active
   visual-architecture phase and pointers to retired source-split records.
6. [implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md](implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md):
   active scene emission artifact refactor plan.
7. [implementation/SCENE_CODE_SPLIT_ROADMAP.md](implementation/SCENE_CODE_SPLIT_ROADMAP.md):
   retired pointer for the completed broad scene source split.
8. [core/RUNTIME_BOUNDARY.md](core/RUNTIME_BOUNDARY.md): active scene -> FramePlan -> DRP2 ->
   app/runtime boundary.
9. [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md): update/build/emit/submit flow.
10. [validation/DEFERRED_TRACKER.md](validation/DEFERRED_TRACKER.md): consolidated deferred-item
   index.

Normative scene semantics and pipeline contracts:

1. [core/REQUIREMENTS.md](core/REQUIREMENTS.md): scene requirements on DRP2 and runtime services.
2. [core/OBJECT_MODEL.md](core/OBJECT_MODEL.md): stable concepts and ownership model.
3. [core/PANEL_LAYOUT.md](core/PANEL_LAYOUT.md): figure, panel, grid, and layout behavior.
4. [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md): scene-owned logical resources,
   sampled fields, uploads, readback, and F64 ingestion policy.
5. [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md): attribute granularity and
   mutability vocabulary.
6. [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md): normalization and
   panel-transform pipeline.
7. [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md): dirty scopes,
   reuse, redraw, uploads, and plan rebuilds.
8. [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md): producer-side frame artifact.
9. [semantics/VISUAL_FAMILIES.md](semantics/VISUAL_FAMILIES.md): family taxonomy.
10. [semantics/VISUAL_CONTRACT.md](semantics/VISUAL_CONTRACT.md): shared producer contract.
11. [semantics/VISUAL_FAMILY_RULES.md](semantics/VISUAL_FAMILY_RULES.md): cross-family rules and
    fallback constraints.
12. [visuals/README.md](visuals/README.md): per-family data contracts.
13. [semantics/SCALES.md](semantics/SCALES.md): color, size, opacity, colormap, and colorbar scale
    semantics.
14. [semantics/LIGHTING.md](semantics/LIGHTING.md): active material/Phong/depth-cue behavior and
    future scene-light direction.
15. [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md): alpha modes, WBOIT, depth peeling, and
    pass structure.
16. [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md): CPU geometry helpers and
    future SDF/MSDF paths.
17. [semantics/AXES.md](semantics/AXES.md), [semantics/ANNOTATIONS.md](semantics/ANNOTATIONS.md),
    [semantics/LEGENDS_AND_COLORBARS.md](semantics/LEGENDS_AND_COLORBARS.md), and
    [semantics/TEXT.md](semantics/TEXT.md): explanatory object semantics.
18. [implementation/TEXT_SHAPING_ATLAS.md](implementation/TEXT_SHAPING_ATLAS.md): text shaping,
    layout, atlas, cache, and DRP2 emission contract.
19. [semantics/CLIPPING.md](semantics/CLIPPING.md) and
    [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md): specialized rendering
    and coordinate behavior.
20. [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md),
    [interaction/PICKING.md](interaction/PICKING.md),
    [interaction/SELECTION.md](interaction/SELECTION.md),
    [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md), and
    [interaction/ANIMATION.md](interaction/ANIMATION.md): user interaction, request/readback,
    selection, callbacks, and animation.
21. [validation/VALIDATION.md](validation/VALIDATION.md),
    [validation/ADAPTATION.md](validation/ADAPTATION.md), and
    [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md): validation, fallback, and diagnostic
    behavior.

Integration and export:

1. [integration/HOSTED_BACKENDS.md](integration/HOSTED_BACKENDS.md): hosted view, external-surface,
   and render-once integration.
2. [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md): boundary with UI frameworks such as
   ImGui.
3. [integration/HIGH_DPI.md](integration/HIGH_DPI.md): logical/physical pixel and DPI behavior.
4. [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md): render-thread and async handoff
   policy.
5. [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md): user-defined visual-family
   registration.
6. [integration/CUPY_CUDA_INTEROP.md](integration/CUPY_CUDA_INTEROP.md): CUDA/CuPy external-memory
   path.
7. [integration/future/ANDROID_SUPPORT.md](integration/future/ANDROID_SUPPORT.md),
   [integration/future/IOS_SUPPORT.md](integration/future/IOS_SUPPORT.md), and
   [integration/future/TOUCH_SUPPORT.md](integration/future/TOUCH_SUPPORT.md): platform and touch
   planning.
8. [integration/napari/README.md](integration/napari/README.md): informative napari adapter notes.
9. [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md): app capture, DVZR recording/replay, and future
   render-scale/panel-as-texture directions.
10. [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md): v0.4 vector-export scope decision.

Proposals, history, and examples:

1. [proposals/README.md](proposals/README.md): proposal taxonomy, promotion targets, and absorbed
   proposal status.
2. [decisions/README.md](decisions/README.md): historical ADR-style policy.
3. [headers/README.md](headers/README.md): historical header-sketch notes; installed headers remain
   authoritative for active names.
4. [core/USE_CASES.md](core/USE_CASES.md): pressure-test scenarios.
5. [examples/README.md](examples/README.md): worked examples and API pressure tests.


[api-pressure-sketches]: examples/scenarios/api_sketches/API_PRESSURE_SKETCHES.md
[core-release-proofs]: examples/scenarios/v04_required/CORE_RELEASE_PROOFS.md
[scene-visual-boundary-guardrails]: implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md
