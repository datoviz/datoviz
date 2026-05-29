# Scene Spec

This directory defines the scene layer as a consumer of DRP2, not as a backend runtime.

The scene layer should remain pure high-level logic:

1. build and own user-facing visualization state,
2. derive rendering work from that state,
3. emit DRP2 through a runtime-facing contract,
4. stay independent from Vulkan, swapchain, and windowing internals.


## Status

- Status: active specification with multiple implementation slices in `src/scene`
- Implementation priority: prove the declared v0.4 surface for RC1, especially release examples,
  WebGPU/WASM experimental scope, raw bindings, API/status labeling, and v0.3 visible parity
- Primary constraint: do not let scene design leak backend details into its public API

Current source implementation is intentionally smaller than this spec. It includes scene/figure/panel
objects; `pixel`, `point`, `marker`, `primitive`, `mesh`, `sphere`, path/segment, `image`, `volume`,
and text/glyph visuals; capability snapshots; diagnostic reports; frame plans; DRP2 emission; panel
controllers; retained sampled fields; scene buffers; scale/colormap state for image and volume
paths; rendered continuous colorbars; rendered label annotations; rendered scale bars; queued panel
queries with GPU item and sampled-value readback execution; selection/link
bookkeeping; graph-backed techniques; and app/offscreen/GLFW paths. Public headers also declare
broader interaction, annotation/readout, selection, material, technique, and visual-family behavior
that is not fully rendered or semantically complete yet. Treat broader sections of this spec as
design pressure and direction, not as a claim that all families and interactions are already
implemented.


## Directory Layout

The scene spec is split by kind of authority:

1. [core](core/README.md): foundational ownership, object model, runtime boundary, and use cases.
2. [api](api/README.md): public API profile, public header surface, and implementation bridge.
3. [semantics](semantics/README.md): user-visible scene semantics and cross-family behavior.
4. [pipeline](pipeline/README.md): resource, transform, invalidation, frame-plan, and lifecycle contracts.
5. [interaction](interaction/README.md): controllers, picking, selection, callbacks, and animation.
6. [visuals](visuals/README.md): per-family data contracts.
7. [validation](validation/README.md): validation, adaptation, diagnostics, and deferred items.
8. [integration](integration/README.md): host UI, threading, high-DPI, and custom visuals.
9. [export](export/README.md): image export semantics and the current vector-export scope decision.
10. [slices](slices/README.md): implementation-ready work packets for mature spec areas.
11. [headers](headers/README.md): implementation-facing draft C header sketches.
12. [implementation](implementation/README.md): concise notes for active implementation wiring.
13. [proposals](proposals/README.md): active, promoted, future, and historical proposal notes.
14. [decisions](decisions/README.md): historical ADR-style decision records.
15. [examples](examples/README.md): worked examples and API-shape pressure tests.


## Relationship To The DRP2 Spec

The scene layer is a consumer of DRP2.
It emits DRP2 commands — it never calls Vulkan, vklite, or any backend API directly.

The scene spec and the DRP2 spec (`spec/drp2/`) are being designed in parallel and feed
requirements into each other:

1. when the scene spec needs a DRP2 capability that does not yet exist, that is a DRP2 spec
   input — not a reason to add a scene-side workaround or backend escape hatch,
2. when the DRP2 spec adds or changes commands, the scene spec should be updated to reflect
   what the scene can now express,
3. open questions in the scene spec that depend on DRP2 details should be left explicitly open
   until the relevant DRP2 decision is made.

This parallel design constraint applies especially to `core/RUNTIME_BOUNDARY.md`,
`pipeline/FRAME_PLAN.md`, and `validation/ADAPTATION.md`.


## Normative Invariants

The current scene spec should be read with the following invariants in mind:

1. scene owns high-level semantics and authored state, while the runtime owns execution details,
2. exactly one scene-level `FramePlan` is the canonical producer-side execution artifact for a frame
   build, even when it contains panel-local subplans or per-panel nodes,
3. uploads and lazy materialization work should appear in `FramePlan`, not in a separate execution
   side path,
4. identity queries must return scene identity rather than backend identity,
5. hover queries follow latest-request-wins semantics and stale results must be discardable,
6. request processing may coalesce unresolved panel-local query requests, but accepted results
   must still obey the public freshness rules,
7. data normalization and panel-local navigation are separate stages,
8. panel navigation should usually not force normalization rebuild or bulk data reupload,
9. compute-derived resources are frame-local by default unless persistence is declared explicitly,
10. legends and colorbars may aggregate only semantically identical mappings unless sharing is
   configured explicitly,
11. capability adaptation must be explicit and deterministic rather than implicit or backend-shaped,
12. validation runs after dirty-scope resolution and before planning,
13. capability adaptation runs after validation and before planning.


## Reading Conventions

Unless a document says otherwise, this directory should be read with the following conventions:

1. sections titled `Core Rule`, `Rules`, `Requirements`, `Hard Requirements`, `Normative
   Invariants`, `Current Preferred Direction`, or `Contract` are normative for the current planning
   baseline,
2. sections titled `Purpose`, `Position`, `Why`, `Rationale`, `Examples`, `Deferred Questions`,
   `Open Choices`, `Deferred API Choices`, `What This Document Intentionally Leaves Open`,
   `Immediate Follow-Up`, `Immediate Follow-On Specs`, or `Recommended Next Step` are informative
   unless they explicitly say otherwise,
3. worked examples under `examples/` are informative pressure tests, not independent sources of
   normative behavior,
4. when two documents overlap, the more specialized contract document should win over a broader
   orientation document,
5. `proposals/active/` records are active design addenda used to clarify or extend the normative
   documents until those rules are promoted into specialized spec files,
6. `decisions/` records are historical ADR-style records and should explain rationale, not hold
   current implementation-facing rules on their own,
7. `api/API_SURFACE.md` is the normative bridge from scene semantics to public C API shape policy,
8. installed headers under `include/datoviz/scene*.h` are authoritative for names already drafted.

## Status Vocabulary

Scene spec files should use one of these statuses, either explicitly in a `Normative Status` section
or by inheriting the directory role from the nearest `README.md`:

1. `Normative`: implementation-facing rules for the current v0.4 planning baseline.
2. `Proposal`: active design addendum that may contain planning rules not yet absorbed into a
   specialized normative spec.
3. `Informative`: examples, rationale, implementation notes, or trackers that do not override
   normative specs.
4. `Historical Decision`: ADR-style rationale for older choices; not a current standalone source
   of implementation rules.


For cross-tree overlap, use this source-of-truth order:

1. DRP2 command, lifetime, and error prose for protocol semantics,
2. DRP2 active JSON schemas for machine-checkable command shape,
3. installed scene headers for public names and signatures that already exist,
4. scene normative documents for scene semantics,
5. `spec/scene/api/API_SURFACE.md` for public API shape policy and not-yet-implemented groups,
6. scene proposals for rules not yet promoted into specialized spec files,
7. scene implementation slices for concrete work boundaries that apply those rules,
8. historical scene decision records for rationale behind older choices,
9. examples, deferred trackers, and historical header sketches as informative material.


## Recommended Reading Order

Read the scene spec in this order during review.

### 1. Orientation and high-level constraints

1. [core/REQUIREMENTS.md](core/REQUIREMENTS.md) — scene goals, scope, and required runtime/DRP2 support
2. [core/RUNTIME_BOUNDARY.md](core/RUNTIME_BOUNDARY.md) — what scene may and may not depend on
3. [core/USE_CASES.md](core/USE_CASES.md) — pressure-test scenarios to keep in mind while reviewing

### 2. Core scene concepts and public shape

1. [core/OBJECT_MODEL.md](core/OBJECT_MODEL.md) — stable scene concepts and ownership model
2. [core/PANEL_LAYOUT.md](core/PANEL_LAYOUT.md) — grid layout, free placement, fixed columns/rows, span, tight layout
3. [api/API_DESIGN.md](api/API_DESIGN.md) — current preferred scene-facing defaults and resolved API decisions
4. [api/API_SURFACE.md](api/API_SURFACE.md) — public API shape policy and implemented-vs-draft boundary
5. [api/API_IMPLEMENTATION_READINESS.md](api/API_IMPLEMENTATION_READINESS.md) — checklist for the next public API pass
6. [headers/README.md](headers/README.md) — draft header index for pressure-testing the surface
7. [slices/README.md](slices/README.md) — implementation-ready work packets and readiness matrix
8. [proposals/README.md](proposals/README.md) — proposal taxonomy, promotion status, and indexes
9. [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md) — C-facing mapping, Python binding architecture

### 3. Visual semantics

1. [semantics/VISUAL_FAMILIES.md](semantics/VISUAL_FAMILIES.md) — family taxonomy
2. [semantics/VISUAL_CONTRACT.md](semantics/VISUAL_CONTRACT.md) — shared producer contract across visuals
3. [semantics/VISUAL_FAMILY_RULES.md](semantics/VISUAL_FAMILY_RULES.md) — family-level mini-contracts
4. [visuals/README.md](visuals/README.md) — per-family data contracts (attribute schemas, parameters, variants)
5. [semantics/SCALES.md](semantics/SCALES.md) — color, size, and opacity scale objects and colormap model
6. [semantics/LIGHTING.md](semantics/LIGHTING.md) — scene-level lighting model; PBR and ray tracing upgrade path
7. [semantics/AXES.md](semantics/AXES.md) — axes and tick semantics
8. [semantics/ANNOTATIONS.md](semantics/ANNOTATIONS.md) — labels, guides, probes, overlays, and callouts
9. [semantics/LEGENDS_AND_COLORBARS.md](semantics/LEGENDS_AND_COLORBARS.md) — explanatory mapping semantics
10. [semantics/TEXT.md](semantics/TEXT.md) — text content, placement, resources, and DPI behavior
11. [slices/TEXT_RENDERING_SLICE.md](slices/TEXT_RENDERING_SLICE.md) — first rendered text work packet
12. [implementation/TEXT_SHAPING_ATLAS.md](implementation/TEXT_SHAPING_ATLAS.md) — text shaping,
    layout, atlas, cache, and DRP2 emission contract
13. [slices/ANNOTATION_LABEL_SLICE.md](slices/ANNOTATION_LABEL_SLICE.md) — first rendered label annotation work packet
14. [slices/COLORBAR_RENDERING_SLICE.md](slices/COLORBAR_RENDERING_SLICE.md) — first rendered colorbar work packet

### 4. Data, transforms, planning, and runtime handoff

1. [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md) — logical resource model and F64 data ingestion policy
2. [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md) — per-attribute data granularity and mutability hints
3. [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md) — normalization, panel-transform pipeline, and CPU precision policy
4. [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md) — triangulation, curve tessellation, simplification, hull, boolean ops, SDF/MSDF pipeline
5. [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md) — canonical producer-side frame artifact
6. [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md) — update/build/emit flow

### 5. Validation, adaptation, interaction, and diagnostics

1. [validation/VALIDATION.md](validation/VALIDATION.md) — validation rules and error classes
2. [validation/ADAPTATION.md](validation/ADAPTATION.md) — explicit fallback and simplification policy
3. [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md) — dirty scopes and reuse rules
4. [interaction/GPU_QUERY_SYSTEM.md](interaction/GPU_QUERY_SYSTEM.md) — GPU-only panel query
   architecture replacing public pick/probe
5. [interaction/PICKING.md](interaction/PICKING.md) — picking identity and readback behavior
6. [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md) — event routing and interaction ownership
7. [interaction/ANIMATION.md](interaction/ANIMATION.md) — scene clock, animation objects, easing, camera keyframes, video export
8. [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md) — boundary with app-owned UI frameworks
9. [interaction/SELECTION.md](interaction/SELECTION.md) — scene-level selection state, highlight rendering, cross-visual linking, and lasso
10. [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md) — still image capture, render scale, panel-as-texture
11. [integration/HIGH_DPI.md](integration/HIGH_DPI.md) — device pixel ratio, logical vs physical pixels, DPI change handling
12. [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md) — alpha modes and render pass structure
13. [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md) — non-linear coordinate transforms
14. [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md) — v0.4 decision to leave
    publication-oriented vector export to GSP/Matplotlib
15. [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md) — user-defined visual families
16. [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md) — threading model and async data handoff
17. [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md) — scene event observer system
18. [semantics/CLIPPING.md](semantics/CLIPPING.md) — per-visual clip modes
19. [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md) — shared diagnostic shape across the stack

### 6. Worked examples

1. [examples/README.md](examples/README.md) — entry point for worked examples
2. [examples/PLANNING.md](examples/PLANNING.md) — release staging, priorities, and current gaps
3. [examples/FIXTURES.md](examples/FIXTURES.md) — compact validation fixture matrix
4. [examples/scenarios/v04_required/CORE_RELEASE_PROOFS.md](examples/scenarios/v04_required/CORE_RELEASE_PROOFS.md)
5. [examples/scenarios/v04_required/SHOWCASES.md](examples/scenarios/v04_required/SHOWCASES.md)
6. [examples/scenarios/api_sketches/API_PRESSURE_SKETCHES.md](examples/scenarios/api_sketches/API_PRESSURE_SKETCHES.md)


## Document Index

Use this index when you know the topic but not the owning directory.

Current implementation orientation:

- [api/API_SURFACE.md](api/API_SURFACE.md): public API shape policy and implemented-vs-draft
  boundary.
- [api/API_IMPLEMENTATION_READINESS.md](api/API_IMPLEMENTATION_READINESS.md): current readiness
  checklist and remaining public API implementation gaps.
- [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md): C object mapping, app/runtime
  wiring, Python binding architecture, and GPU preparation notes.
- [implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md](implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md):
  active visual-architecture phase for registry-driven generic scene code and family-owned visual
  behavior, plus pointers to retired source-split records.
- [implementation/SCENE_CODE_SPLIT_ROADMAP.md](implementation/SCENE_CODE_SPLIT_ROADMAP.md):
  retired pointer for the completed broad scene source split.
- [core/RUNTIME_BOUNDARY.md](core/RUNTIME_BOUNDARY.md): active scene -> FramePlan -> DRP2 ->
  app/runtime boundary.
- [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md): update/build/emit/submit flow.
- [validation/DEFERRED_TRACKER.md](validation/DEFERRED_TRACKER.md): consolidated index of
  explicitly deferred items.

Normative scene semantics and pipeline contracts:

- [core/REQUIREMENTS.md](core/REQUIREMENTS.md): scene requirements on DRP2 and runtime services.
- [core/OBJECT_MODEL.md](core/OBJECT_MODEL.md): stable concepts and ownership model.
- [core/PANEL_LAYOUT.md](core/PANEL_LAYOUT.md): figure, panel, grid, and layout behavior.
- [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md): scene-owned logical resources,
  sampled fields, uploads, readback, and F64 ingestion policy.
- [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md): attribute granularity and
  mutability vocabulary.
- [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md): normalization and
  panel-transform pipeline.
- [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md): dirty scopes,
  reuse, redraw, uploads, and plan rebuilds.
- [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md): producer-side frame artifact.
- [semantics/VISUAL_FAMILIES.md](semantics/VISUAL_FAMILIES.md): family taxonomy and active
  implementation status.
- [semantics/VISUAL_CONTRACT.md](semantics/VISUAL_CONTRACT.md): shared producer contract for
  visuals.
- [semantics/VISUAL_FAMILY_RULES.md](semantics/VISUAL_FAMILY_RULES.md): cross-family rules and
  fallback constraints.
- [visuals/README.md](visuals/README.md): per-family data contracts and implementation status.
- [semantics/SCALES.md](semantics/SCALES.md): color, size, opacity, colormap, and colorbar scale
  semantics.
- [semantics/LIGHTING.md](semantics/LIGHTING.md): active material/Phong/depth-cue behavior and
  future scene-light direction.
- [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md): alpha modes, WBOIT, depth peeling, and
  pass structure.
- [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md): CPU geometry helpers and
  future SDF/MSDF paths.
- [semantics/AXES.md](semantics/AXES.md), [semantics/ANNOTATIONS.md](semantics/ANNOTATIONS.md),
  [semantics/LEGENDS_AND_COLORBARS.md](semantics/LEGENDS_AND_COLORBARS.md), and
  [semantics/TEXT.md](semantics/TEXT.md): explanatory object semantics.
- [implementation/TEXT_SHAPING_ATLAS.md](implementation/TEXT_SHAPING_ATLAS.md): text shaping,
  layout, atlas, cache, and DRP2 emission contract.
- [semantics/CLIPPING.md](semantics/CLIPPING.md) and
  [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md): specialized rendering
  and coordinate behavior.
- [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md),
  [interaction/PICKING.md](interaction/PICKING.md),
  [interaction/SELECTION.md](interaction/SELECTION.md),
  [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md), and
  [interaction/ANIMATION.md](interaction/ANIMATION.md): user interaction, request/readback,
  selection, callbacks, and animation.
- [validation/VALIDATION.md](validation/VALIDATION.md),
  [validation/ADAPTATION.md](validation/ADAPTATION.md), and
  [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md): validation, fallback, and diagnostic
  behavior.

Integration and export:

- [integration/HOSTED_BACKENDS.md](integration/HOSTED_BACKENDS.md): hosted view,
  external-surface, and render-once integration.
- [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md): boundary with UI frameworks such as
  ImGui.
- [integration/HIGH_DPI.md](integration/HIGH_DPI.md): logical/physical pixel and DPI behavior.
- [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md): render-thread and async handoff
  policy.
- [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md): user-defined visual-family
  registration.
- [integration/CUPY_CUDA_INTEROP.md](integration/CUPY_CUDA_INTEROP.md): CUDA/CuPy external-memory
  path.
- [integration/ANDROID_SUPPORT.md](integration/ANDROID_SUPPORT.md),
  [integration/IOS_SUPPORT.md](integration/IOS_SUPPORT.md), and
  [integration/TOUCH_SUPPORT.md](integration/TOUCH_SUPPORT.md): platform and touch planning.
- [integration/napari/README.md](integration/napari/README.md): informative napari adapter notes.
- [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md): app capture, DVZR recording/replay, and future
  render-scale/panel-as-texture directions.
- [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md): v0.4 vector-export scope decision.

Proposals, history, and examples:

- [proposals/README.md](proposals/README.md): proposal taxonomy, promotion targets, and absorbed
  proposal status.
- [decisions/README.md](decisions/README.md): historical ADR-style policy.
- [headers/README.md](headers/README.md): historical header-sketch notes; installed headers remain
  authoritative for active names.
- [core/USE_CASES.md](core/USE_CASES.md): pressure-test scenarios.
- [examples/README.md](examples/README.md): worked examples and API pressure tests.


## Guiding Principles

1. Keep pushing scene semantics and producer contracts.
2. Avoid freezing backend-shaped details too early.
3. Let DRP2 and runtime work continue underneath without leaking upward.

Deferred items by milestone are tracked in `validation/DEFERRED_TRACKER.md`.
