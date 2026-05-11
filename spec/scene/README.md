# Scene Spec

This directory defines the scene layer as a consumer of DRP2, not as a backend runtime.

The scene layer should remain pure high-level logic:

1. build and own user-facing visualization state,
2. derive rendering work from that state,
3. emit DRP2 through a runtime-facing contract,
4. stay independent from Vulkan, swapchain, and windowing internals.


## Status

- Status: active specification with multiple implementation slices in `src/scene`
- Implementation priority: implement the already-drafted interaction/text/annotation public APIs in
  narrow tested slices, then add picking/probe runtime plumbing and rendered annotation/colorbar work
- Primary constraint: do not let scene design leak backend details into its public API

Current source implementation is intentionally smaller than this spec. It includes scene/figure/panel
objects, `point` / `primitive` / `mesh` / path-as-line/strip / `image` visuals, capability
snapshots, diagnostic reports, frame plans, DRP2 emission, panel controllers, retained sampled
fields, scene buffers, scale/colormap state for images, and an early scene/app/offscreen path.
Public headers also declare interaction, text, and annotation APIs that are draft contracts until
implemented in `src/scene`. Treat broader sections of this spec as design pressure and direction,
not as a claim that all families and interactions are already implemented.


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
9. [export](export/README.md): image and vector export semantics.
10. [headers](headers/README.md): implementation-facing draft C header sketches.
11. [proposals](proposals/README.md): active design addenda awaiting promotion into specialized specs.
12. [decisions](decisions/README.md): historical ADR-style decision records.
13. [examples](examples/README.md): worked examples and API-shape pressure tests.


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
4. picking must return scene identity rather than backend identity,
5. hover picking follows latest-request-wins semantics and stale results must be discardable,
6. data normalization and panel-local navigation are separate stages,
7. panel navigation should usually not force normalization rebuild or bulk data reupload,
8. compute-derived resources are frame-local by default unless persistence is declared explicitly,
9. legends and colorbars may aggregate only semantically identical mappings unless sharing is
   configured explicitly,
10. capability adaptation must be explicit and deterministic rather than implicit or backend-shaped,
11. validation runs after dirty-scope resolution and before planning,
12. capability adaptation runs after validation and before planning.


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
5. `proposals/` records are active design addenda used to clarify or extend the normative documents
   until those rules are promoted into specialized spec files,
6. `decisions/` records are historical ADR-style records and should explain rationale, not hold
   current implementation-facing rules on their own,
7. `api/API_SURFACE.md` is the normative bridge from scene semantics to public C API shape policy,
8. installed headers under `include/datoviz/scene*.h` are authoritative for names already drafted;
   `headers/scene_api.h` remains auxiliary scratch material only where it covers ideas not yet in
   installed headers.

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
3. scene normative documents for scene semantics,
4. scene proposals for rules not yet promoted into specialized spec files,
5. historical scene decision records for rationale behind older choices,
6. installed scene headers for public names and signatures that already exist,
7. `spec/scene/api/API_SURFACE.md` for public API shape policy and not-yet-implemented groups,
8. `spec/scene/headers/scene_api.h` for auxiliary draft C sketches not yet promoted,
9. examples and deferred trackers as informative material.


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
7. [proposals/README.md](proposals/README.md) — active scene proposals awaiting promotion
8. [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md) — C-facing mapping, Python binding architecture

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
4. [interaction/PICKING.md](interaction/PICKING.md) — picking identity and readback behavior
5. [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md) — event routing and interaction ownership
6. [interaction/ANIMATION.md](interaction/ANIMATION.md) — scene clock, animation objects, easing, camera keyframes, video export
7. [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md) — boundary with app-owned UI frameworks
8. [interaction/SELECTION.md](interaction/SELECTION.md) — scene-level selection state, highlight rendering, cross-visual linking, and lasso
9. [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md) — still image capture, render scale, panel-as-texture
10. [integration/HIGH_DPI.md](integration/HIGH_DPI.md) — device pixel ratio, logical vs physical pixels, DPI change handling
11. [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md) — alpha modes and render pass structure
12. [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md) — non-linear coordinate transforms
13. [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md) — structural SVG export
14. [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md) — user-defined visual families
15. [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md) — threading model and async data handoff
16. [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md) — scene event observer system
17. [semantics/CLIPPING.md](semantics/CLIPPING.md) — per-visual clip modes
18. [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md) — shared diagnostic shape across the stack

### 6. Worked examples

1. [examples/README.md](examples/README.md) — entry point for worked examples
2. [examples/POINT_2D.md](examples/POINT_2D.md)
3. [examples/MARKER_PICKING.md](examples/MARKER_PICKING.md)
4. [examples/PATH_AXES_2D.md](examples/PATH_AXES_2D.md)
5. [examples/VOLUME_SLICE.md](examples/VOLUME_SLICE.md)
6. [examples/SPHERE_IMPOSTOR.md](examples/SPHERE_IMPOSTOR.md)
7. [examples/VOLUME_OFFSCREEN.md](examples/VOLUME_OFFSCREEN.md)
8. [examples/LINKED_PANELS_PROBE_COLORBAR.md](examples/LINKED_PANELS_PROBE_COLORBAR.md)
9. [examples/MOUSE_BRAIN_ATLAS_EXPLORER.md](examples/MOUSE_BRAIN_ATLAS_EXPLORER.md)
10. [examples/LINKED_PANELS_AXES_PANZOOM.md](examples/LINKED_PANELS_AXES_PANZOOM.md)
11. [examples/ANIMATION_VIDEO_EXPORT.md](examples/ANIMATION_VIDEO_EXPORT.md)
12. [examples/API_MESH_SELECTION_LINK.md](examples/API_MESH_SELECTION_LINK.md)
13. [examples/API_IMAGE_PROBE_PINNED_READOUT.md](examples/API_IMAGE_PROBE_PINNED_READOUT.md)
14. [examples/API_SCALE_COLORBAR_ANNOTATION.md](examples/API_SCALE_COLORBAR_ANNOTATION.md)
15. [examples/API_SAMPLED_FIELD.md](examples/API_SAMPLED_FIELD.md)


## Document Index

- [core/REQUIREMENTS.md](core/REQUIREMENTS.md): what the scene layer needs from DRP2 and the runtime
- [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md): per-attribute data granularity
  (CONSTANT / PER_ITEM / PER_SPAN / PER_GROUP) and optional mutability hints
- [visuals/README.md](visuals/README.md): per-family data contracts with attribute schemas,
  parameters, variant axes, and v0.3 correspondence
- [semantics/SCALES.md](semantics/SCALES.md): color, size, and opacity scale objects; colormap palette model; domain
  and scale identity for visual attributes and colorbars
- [core/OBJECT_MODEL.md](core/OBJECT_MODEL.md): minimum stable concepts
- [api/API_DESIGN.md](api/API_DESIGN.md): design rationale and resolved API decisions
  behind `headers/scene_api.h`
- [api/API_SURFACE.md](api/API_SURFACE.md): next public-header shape for retained interaction, scales,
  colorbars, text, and annotation objects
- [api/API_IMPLEMENTATION_READINESS.md](api/API_IMPLEMENTATION_READINESS.md): implementation-readiness
  checklist for the next public scene API pass
- [api/IMPLEMENTATION_NOTES.md](api/IMPLEMENTATION_NOTES.md): C object mapping, Python binding
  architecture, and per-family GPU data preparation notes
- [headers/README.md](headers/README.md): draft header index for the current scene, runtime, and
  diagnostics surfaces; `scene_api.h` is authoritative for the API groups it covers
- [proposals/README.md](proposals/README.md): active design addenda with promotion targets for
  implementation-ready spec work
- [decisions/README.md](decisions/README.md): historical ADR-style records and authority policy
- [semantics/AXES.md](semantics/AXES.md): scene-side semantic model for axes, ticks, labels, and related annotations
- [semantics/LIGHTING.md](semantics/LIGHTING.md): scene-level lighting model, PBR shading parameters, and hardware
  ray tracing forward-compatibility path
- [semantics/ANNOTATIONS.md](semantics/ANNOTATIONS.md): semantic model for labels, guides, probes, overlays, legends,
  and callouts
- [semantics/LEGENDS_AND_COLORBARS.md](semantics/LEGENDS_AND_COLORBARS.md): semantic model for discrete legends,
  continuous colorbars, and shared explanatory mappings
- [semantics/TEXT.md](semantics/TEXT.md): text content, placement, font/atlas resources, and DPI behavior
- [validation/DIAGNOSTICS.md](validation/DIAGNOSTICS.md): shared conceptual diagnostic shape across
  validation, adaptation, planning, and runtime execution
- [validation/VALIDATION.md](validation/VALIDATION.md): scene-level pre-emission validation rules, error
  classes, and capability-gated checks
- [validation/ADAPTATION.md](validation/ADAPTATION.md): explicit fallback, simplification, and
  deactivation policy driven by runtime capabilities
- [integration/EXTERNAL_UI.md](integration/EXTERNAL_UI.md): boundary between scene-owned semantics and app-owned UI
  frameworks such as ImGui
- [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md): rules for dirty scopes, reuse,
  redraw, uploads, and plan rebuilds
- [interaction/PICKING.md](interaction/PICKING.md): scene-side picking, identity round-trip, grouped hits, and readback
  semantics
- [interaction/ANIMATION.md](interaction/ANIMATION.md): scene clock, animation handles, easing curves, camera keyframes,
  per-attribute animation, and video export scheduling
- [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md): event routing, panel-owned navigation, picking-driven
  interaction, and redraw
- [semantics/VISUAL_FAMILIES.md](semantics/VISUAL_FAMILIES.md): preferred v0.4 visual-family taxonomy grounded in
  local `v0.3` terminology
- [semantics/VISUAL_CONTRACT.md](semantics/VISUAL_CONTRACT.md): producer-side contract every future visual type must
  satisfy
- [semantics/VISUAL_FAMILY_RULES.md](semantics/VISUAL_FAMILY_RULES.md): cross-family boundary rules, fallback
  constraints, and anti-patterns (the shared template; per-family detail is in `visuals/`)
- [core/PANEL_LAYOUT.md](core/PANEL_LAYOUT.md): grid layout, free placement, fixed-size columns/rows,
  row/column span, colorbar slots, fixed aspect ratio, shared-width constraint, tight layout
- [interaction/SELECTION.md](interaction/SELECTION.md): scene-level selection state, selection modes, parametrizable
  input mapping, highlight descriptor, lasso via GPU ComputeNode, cross-visual linking
- [export/IMAGE_EXPORT.md](export/IMAGE_EXPORT.md): still image capture boundary, render scale (supersampling),
  panel-as-texture with FramePlan ordering and cycle detection
- [integration/HIGH_DPI.md](integration/HIGH_DPI.md): logical pixel coordinate model, device pixel ratio, pixel-unit
  quantity scaling, font rasterization at physical resolution, DPI change handling
- [semantics/TRANSPARENCY.md](semantics/TRANSPARENCY.md): alpha modes (opaque/blended/exact/mask), weighted blended OIT
  default, per-pixel linked list OIT opt-in, render pass split, CPU depth sort fallback
- [semantics/NONLINEAR_TRANSFORMS.md](semantics/NONLINEAR_TRANSFORMS.md): CPU-side projection (v0.4), GPU compute
  pre-pass for persistent derived position buffer (v0.4+), built-in and custom projections
- [export/VECTOR_EXPORT.md](export/VECTOR_EXPORT.md): structural SVG — axes/annotations/colorbars as vector,
  GPU visual output as embedded raster; render scale for embed resolution
- [integration/CUSTOM_VISUALS.md](integration/CUSTOM_VISUALS.md): DvzVisualDesc registration, attribute schema, shader
  injection points, picking/selection/capability integration for user-defined visual families
- [integration/THREAD_SAFETY.md](integration/THREAD_SAFETY.md): single-threaded render path, DvzTransferQueue for
  background threads, data/uniform/callback transfer types, async upload and completion hooks
- [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md): dvz_scene_on observer registration, event types
  (selection changed, pick result, hover, anim step/complete, resize, DPI), lifecycle fire points
- [semantics/CLIPPING.md](semantics/CLIPPING.md): DVZ_CLIP_DATA_AREA (default), DVZ_CLIP_PANEL, DVZ_CLIP_NONE;
  data area derived from axes margins; scissor grouping in render pass
- [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md): scene-owned logical data model for visuals, planning,
  upload, readback, and F64 source data ingestion policy
- [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md): CPU-side geometry utility layer — triangulation
  (earcut + Triangle/PSLG), curve tessellation (Bézier, Catmull-Rom, B-spline), Douglas-Peucker
  simplification, 2D convex hull, boolean polygon ops (Clipper2), and SDF/MSDF pipeline
- [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md): explicit data-normalization and panel-transform
  pipeline for scene visuals
- [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md): producer-side intermediate representation for one planned
  frame
- [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md): update/build/emit flow
- [core/RUNTIME_BOUNDARY.md](core/RUNTIME_BOUNDARY.md): allowed and forbidden runtime dependencies,
  service object model, submission/completion/diagnostics contracts, canvas and render-target
  ownership
- [core/USE_CASES.md](core/USE_CASES.md): pressure-test scenarios (UC1–UC7)
- [validation/DEFERRED_TRACKER.md](validation/DEFERRED_TRACKER.md): consolidated index of all explicitly deferred items
  by milestone (DRP2 2.1, v0.4+, no target)
- [examples/README.md](examples/README.md): worked scene-spec examples that instantiate families,
  transforms, and `FramePlan` shapes
- [examples/API_MESH_SELECTION_LINK.md](examples/API_MESH_SELECTION_LINK.md),
  [examples/API_IMAGE_PROBE_PINNED_READOUT.md](examples/API_IMAGE_PROBE_PINNED_READOUT.md), and
  [examples/API_SCALE_COLORBAR_ANNOTATION.md](examples/API_SCALE_COLORBAR_ANNOTATION.md): tiny
  public API-shape pressure tests for the next header draft


## Guiding Principles

1. Keep pushing scene semantics and producer contracts.
2. Avoid freezing backend-shaped details too early.
3. Let DRP2 and runtime work continue underneath without leaking upward.

Deferred items by milestone are tracked in `validation/DEFERRED_TRACKER.md`.
