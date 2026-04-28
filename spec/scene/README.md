# Scene Spec

This directory defines the future scene layer as a consumer of DRP2, not as a backend runtime.

The scene layer should remain pure high-level logic:

1. build and own user-facing visualization state,
2. derive rendering work from that state,
3. emit DRP2 through a runtime-facing contract,
4. stay independent from Vulkan, swapchain, and windowing internals.


## Status

- Status: planning only
- Implementation priority: after the DRP2 contract is frozen enough to avoid churn
- Primary constraint: do not let scene design leak backend details into its public API


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

This parallel design constraint applies especially to `RUNTIME_BOUNDARY.md`,
`pipeline/FRAME_PLAN.md`, and `ADAPTATION.md`.


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
   orientation document.


## Recommended Reading Order

Read the scene spec in this order during review.

### 1. Orientation and high-level constraints

1. [REQUIREMENTS.md](REQUIREMENTS.md) — scene goals, scope, and required runtime/DRP2 support
2. [RUNTIME_BOUNDARY.md](RUNTIME_BOUNDARY.md) — what scene may and may not depend on
3. [USE_CASES.md](USE_CASES.md) — pressure-test scenarios to keep in mind while reviewing

### 2. Core scene concepts and public shape

4. [OBJECT_MODEL.md](OBJECT_MODEL.md) — stable scene concepts and ownership model
5. [PANEL_LAYOUT.md](PANEL_LAYOUT.md) — grid layout, free placement, fixed columns/rows, span, tight layout
6. [API_DESIGN.md](API_DESIGN.md) — current preferred scene-facing defaults and resolved API decisions
7. [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) — C-facing mapping, Python binding architecture
8. [headers/README.md](headers/README.md) — draft header index for pressure-testing the surface

### 3. Visual semantics

9. [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md) — family taxonomy
10. [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md) — shared producer contract across visuals
11. [VISUAL_FAMILY_RULES.md](VISUAL_FAMILY_RULES.md) — family-level mini-contracts
12. [visuals/README.md](visuals/README.md) — per-family data contracts (attribute schemas, parameters, variants)
13. [SCALES.md](SCALES.md) — color, size, and opacity scale objects and colormap model
14. [LIGHTING.md](LIGHTING.md) — scene-level lighting model; PBR and ray tracing upgrade path
15. [AXES.md](AXES.md) — axes and tick semantics
16. [ANNOTATIONS.md](ANNOTATIONS.md) — labels, guides, probes, overlays, and callouts
17. [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md) — explanatory mapping semantics

### 4. Data, transforms, planning, and runtime handoff

16. [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md) — logical resource model and F64 data ingestion policy
17. [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md) — per-attribute data granularity and mutability hints
18. [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md) — normalization, panel-transform pipeline, and CPU precision policy (F64 throughout, F32 at upload)
18b. [GEOMETRY_UTILITIES.md](GEOMETRY_UTILITIES.md) — triangulation, curve tessellation, simplification, hull, boolean ops, SDF/MSDF pipeline
19. [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md) — canonical producer-side frame artifact
20. [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md) — update/build/emit flow

### 5. Validation, adaptation, interaction, and diagnostics

22. [VALIDATION.md](VALIDATION.md) — validation rules and error classes
23. [ADAPTATION.md](ADAPTATION.md) — explicit fallback and simplification policy
24. [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md) — dirty scopes and reuse rules
25. [interaction/PICKING.md](interaction/PICKING.md) — picking identity and readback behavior
26. [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md) — event routing and interaction ownership
27. [interaction/ANIMATION.md](interaction/ANIMATION.md) — scene clock, animation objects, easing, camera keyframes, video export
28. [EXTERNAL_UI.md](EXTERNAL_UI.md) — boundary with app-owned UI frameworks
29. [interaction/SELECTION.md](interaction/SELECTION.md) — scene-level selection state, highlight rendering, cross-visual linking, and lasso
30. [IMAGE_EXPORT.md](IMAGE_EXPORT.md) — still image capture, render scale (supersampling), panel-as-texture
31. [HIGH_DPI.md](HIGH_DPI.md) — device pixel ratio, logical vs physical pixels, DPI change handling
32. [TRANSPARENCY.md](TRANSPARENCY.md) — alpha modes, weighted blended OIT, per-pixel linked list OIT, render pass structure
33. [NONLINEAR_TRANSFORMS.md](NONLINEAR_TRANSFORMS.md) — geographic projections, polar coordinates, GPU compute pre-pass
34. [VECTOR_EXPORT.md](VECTOR_EXPORT.md) — structural SVG export: vector axes/annotations + raster visual embed
35. [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md) — user-defined visual families, descriptor registration, shader injection
36. [THREAD_SAFETY.md](THREAD_SAFETY.md) — threading model, transfer queue, async data upload, background computation handoff
37. [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md) — scene event observer system: selection, pick, hover, animation, resize, DPI
38. [CLIPPING.md](CLIPPING.md) — per-visual clip modes: data area (default), panel, none
39. [DIAGNOSTICS.md](DIAGNOSTICS.md) — shared diagnostic shape across the stack

### 6. Worked examples

29. [examples/README.md](examples/README.md) — entry point for worked examples
30. [examples/POINT_2D.md](examples/POINT_2D.md)
31. [examples/MARKER_PICKING.md](examples/MARKER_PICKING.md)
32. [examples/PATH_AXES_2D.md](examples/PATH_AXES_2D.md)
33. [examples/IMAGE_SLICE.md](examples/IMAGE_SLICE.md)
34. [examples/SPHERE_IMPOSTOR.md](examples/SPHERE_IMPOSTOR.md)
35. [examples/VOLUME_OFFSCREEN.md](examples/VOLUME_OFFSCREEN.md)
36. [examples/LINKED_PANELS_PROBE_COLORBAR.md](examples/LINKED_PANELS_PROBE_COLORBAR.md)
37. [examples/MOUSE_BRAIN_ATLAS_EXPLORER.md](examples/MOUSE_BRAIN_ATLAS_EXPLORER.md)
38. [examples/LINKED_PANELS_AXES_PANZOOM.md](examples/LINKED_PANELS_AXES_PANZOOM.md)
39. [examples/ANIMATION_VIDEO_EXPORT.md](examples/ANIMATION_VIDEO_EXPORT.md)


## Document Index

- [REQUIREMENTS.md](REQUIREMENTS.md): what the scene layer needs from DRP2 and the runtime
- [pipeline/ATTRIBUTE_SOURCES.md](pipeline/ATTRIBUTE_SOURCES.md): per-attribute data granularity (CONSTANT / PER_ITEM
  / PER_GROUP) and optional mutability hints
- [visuals/README.md](visuals/README.md): per-family data contracts with attribute schemas,
  parameters, variant axes, and v0.3 correspondence
- [SCALES.md](SCALES.md): color, size, and opacity scale objects; colormap palette model; domain
  and scale identity for visual attributes and colorbars
- [OBJECT_MODEL.md](OBJECT_MODEL.md): minimum stable concepts
- [API_DESIGN.md](API_DESIGN.md): design rationale and resolved API decisions
  behind `headers/scene_api.h`
- [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md): C object mapping, Python binding
  architecture, and per-family GPU data preparation notes
- [headers/README.md](headers/README.md): non-authoritative draft header index for the current
  scene, runtime, and diagnostics surfaces
- [AXES.md](AXES.md): scene-side semantic model for axes, ticks, labels, and related annotations
- [LIGHTING.md](LIGHTING.md): scene-level lighting model, PBR shading parameters, and hardware
  ray tracing forward-compatibility path
- [ANNOTATIONS.md](ANNOTATIONS.md): semantic model for labels, guides, probes, overlays, legends,
  and callouts
- [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md): semantic model for discrete legends,
  continuous colorbars, and shared explanatory mappings
- [DIAGNOSTICS.md](DIAGNOSTICS.md): shared conceptual diagnostic shape across
  validation, adaptation, planning, and runtime execution
- [VALIDATION.md](VALIDATION.md): scene-level pre-emission validation rules, error
  classes, and capability-gated checks
- [ADAPTATION.md](ADAPTATION.md): explicit fallback, simplification, and
  deactivation policy driven by runtime capabilities
- [EXTERNAL_UI.md](EXTERNAL_UI.md): boundary between scene-owned semantics and app-owned UI
  frameworks such as ImGui
- [pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md): rules for dirty scopes, reuse,
  redraw, uploads, and plan rebuilds
- [interaction/PICKING.md](interaction/PICKING.md): scene-side picking, identity round-trip, grouped hits, and readback
  semantics
- [interaction/ANIMATION.md](interaction/ANIMATION.md): scene clock, animation handles, easing curves, camera keyframes,
  per-attribute animation, and video export scheduling
- [interaction/CONTROLLERS.md](interaction/CONTROLLERS.md): event routing, panel-owned navigation, picking-driven
  interaction, and redraw
- [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md): preferred v0.4 visual-family taxonomy grounded in
  local `v0.3` terminology
- [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md): producer-side contract every future visual type must
  satisfy
- [VISUAL_FAMILY_RULES.md](VISUAL_FAMILY_RULES.md): cross-family boundary rules, fallback
  constraints, and anti-patterns (the shared template; per-family detail is in `visuals/`)
- [PANEL_LAYOUT.md](PANEL_LAYOUT.md): grid layout, free placement, fixed-size columns/rows,
  row/column span, colorbar slots, fixed aspect ratio, shared-width constraint, tight layout
- [interaction/SELECTION.md](interaction/SELECTION.md): scene-level selection state, selection modes, parametrizable
  input mapping, highlight descriptor, lasso via GPU ComputeNode, cross-visual linking
- [IMAGE_EXPORT.md](IMAGE_EXPORT.md): still image capture boundary, render scale (supersampling),
  panel-as-texture with FramePlan ordering and cycle detection
- [HIGH_DPI.md](HIGH_DPI.md): logical pixel coordinate model, device pixel ratio, pixel-unit
  quantity scaling, font rasterization at physical resolution, DPI change handling
- [TRANSPARENCY.md](TRANSPARENCY.md): alpha modes (opaque/blended/exact/mask), weighted blended OIT
  default, per-pixel linked list OIT opt-in, render pass split, CPU depth sort fallback
- [NONLINEAR_TRANSFORMS.md](NONLINEAR_TRANSFORMS.md): CPU-side projection (v0.4), GPU compute
  pre-pass for persistent derived position buffer (v0.4+), built-in and custom projections
- [VECTOR_EXPORT.md](VECTOR_EXPORT.md): structural SVG — axes/annotations/colorbars as vector,
  GPU visual output as embedded raster; render scale for embed resolution
- [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md): DvzVisualDesc registration, attribute schema, shader
  injection points, picking/selection/capability integration for user-defined visual families
- [THREAD_SAFETY.md](THREAD_SAFETY.md): single-threaded render path, DvzTransferQueue for
  background threads, data/uniform/callback transfer types, async upload and completion hooks
- [interaction/EVENT_CALLBACKS.md](interaction/EVENT_CALLBACKS.md): dvz_scene_on observer registration, event types
  (selection changed, pick result, hover, anim step/complete, resize, DPI), lifecycle fire points
- [CLIPPING.md](CLIPPING.md): DVZ_CLIP_DATA_AREA (default), DVZ_CLIP_PANEL, DVZ_CLIP_NONE;
  data area derived from axes margins; scissor grouping in render pass
- [pipeline/RESOURCE_MODEL.md](pipeline/RESOURCE_MODEL.md): scene-owned logical data model for visuals, planning,
  upload, readback, and F64 source data ingestion policy
- [GEOMETRY_UTILITIES.md](GEOMETRY_UTILITIES.md): CPU-side geometry utility layer — triangulation
  (earcut + Triangle/PSLG), curve tessellation (Bézier, Catmull-Rom, B-spline), Douglas-Peucker
  simplification, 2D convex hull, boolean polygon ops (Clipper2), and SDF/MSDF pipeline
- [pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md): explicit data-normalization and panel-transform
  pipeline for scene visuals
- [pipeline/FRAME_PLAN.md](pipeline/FRAME_PLAN.md): producer-side intermediate representation for one planned
  frame
- [pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md): update/build/emit flow
- [RUNTIME_BOUNDARY.md](RUNTIME_BOUNDARY.md): allowed and forbidden runtime dependencies,
  service object model, submission/completion/diagnostics contracts, canvas and render-target
  ownership
- [USE_CASES.md](USE_CASES.md): pressure-test scenarios (UC1–UC7)
- [DEFERRED_TRACKER.md](DEFERRED_TRACKER.md): consolidated index of all explicitly deferred items
  by milestone (DRP2 2.1, v0.4+, no target)
- [examples/README.md](examples/README.md): worked scene-spec examples that instantiate families,
  transforms, and `FramePlan` shapes


## Guiding Principles

1. Keep pushing scene semantics and producer contracts.
2. Avoid freezing backend-shaped details too early.
3. Let DRP2 and runtime work continue underneath without leaking upward.

Deferred items by milestone are tracked in `DEFERRED_TRACKER.md`.
