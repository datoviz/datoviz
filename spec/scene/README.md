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

This parallel design constraint applies especially to `RUNTIME_SERVICE_SKETCH.md`,
`FRAME_PLAN_IR.md`, and `CAPABILITY_ADAPTATION.md`.


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
5. [PREFERRED_API_PROFILE.md](PREFERRED_API_PROFILE.md) — current preferred scene-facing defaults
6. [SCENE_API_SKETCH.md](SCENE_API_SKETCH.md) — tentative user-facing construction surface
7. [IMPLEMENTATION_BRIDGE.md](IMPLEMENTATION_BRIDGE.md) — C-facing mapping of the current design
8. [headers/README.md](headers/README.md) — draft header index for pressure-testing the surface

### 3. Visual semantics

9. [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md) — family taxonomy
10. [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md) — shared producer contract across visuals
11. [VISUAL_MINI_CONTRACTS.md](VISUAL_MINI_CONTRACTS.md) — family-level mini-contracts
12. [visuals/README.md](visuals/README.md) — per-family data contracts (attribute schemas, parameters, variants)
13. [SCALES.md](SCALES.md) — color, size, and opacity scale objects and colormap model
14. [AXES.md](AXES.md) — axes and tick semantics
14. [ANNOTATIONS.md](ANNOTATIONS.md) — labels, guides, probes, overlays, and callouts
15. [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md) — explanatory mapping semantics

### 4. Data, transforms, planning, and runtime handoff

16. [RESOURCE_MODEL.md](RESOURCE_MODEL.md) — logical resource model
17. [ATTRIBUTE_SOURCES.md](ATTRIBUTE_SOURCES.md) — per-attribute data granularity and mutability hints
18. [TRANSFORM_PIPELINE.md](TRANSFORM_PIPELINE.md) — normalization and panel-transform pipeline
19. [FRAME_PLAN_IR.md](FRAME_PLAN_IR.md) — canonical producer-side frame artifact
20. [FRAME_LIFECYCLE.md](FRAME_LIFECYCLE.md) — update/build/emit flow
21. [RUNTIME_SERVICE_SKETCH.md](RUNTIME_SERVICE_SKETCH.md) — conceptual runtime service surface

### 5. Validation, adaptation, interaction, and diagnostics

22. [SCENE_VALIDATION.md](SCENE_VALIDATION.md) — validation rules and error classes
23. [CAPABILITY_ADAPTATION.md](CAPABILITY_ADAPTATION.md) — explicit fallback and simplification policy
24. [INVALIDATION_AND_CACHING.md](INVALIDATION_AND_CACHING.md) — dirty scopes and reuse rules
25. [PICKING.md](PICKING.md) — picking identity and readback behavior
26. [CONTROLLERS.md](CONTROLLERS.md) — event routing and interaction ownership
27. [ANIMATION.md](ANIMATION.md) — scene clock, animation objects, easing, camera keyframes, video export
28. [EXTERNAL_UI.md](EXTERNAL_UI.md) — boundary with app-owned UI frameworks
29. [DIAGNOSTICS_SCHEMA.md](DIAGNOSTICS_SCHEMA.md) — shared diagnostic shape across the stack

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


## Document Index

- [REQUIREMENTS.md](REQUIREMENTS.md): what the scene layer needs from DRP2 and the runtime
- [ATTRIBUTE_SOURCES.md](ATTRIBUTE_SOURCES.md): per-attribute data granularity (CONSTANT / PER_ITEM
  / PER_GROUP) and optional mutability hints
- [visuals/README.md](visuals/README.md): per-family data contracts with attribute schemas,
  parameters, variant axes, and v0.3 correspondence
- [SCALES.md](SCALES.md): color, size, and opacity scale objects; colormap palette model; domain
  and scale identity for visual attributes and colorbars
- [OBJECT_MODEL.md](OBJECT_MODEL.md): minimum stable concepts
- [PREFERRED_API_PROFILE.md](PREFERRED_API_PROFILE.md): the current preferred scene-facing API
  defaults selected from the API sketch
- [IMPLEMENTATION_BRIDGE.md](IMPLEMENTATION_BRIDGE.md): tentative C-facing type and operation
  mapping derived from the current scene spec
- [headers/README.md](headers/README.md): non-authoritative draft header index for the current
  scene, runtime, and diagnostics surfaces
- [AXES.md](AXES.md): scene-side semantic model for axes, ticks, labels, and related annotations
- [ANNOTATIONS.md](ANNOTATIONS.md): semantic model for labels, guides, probes, overlays, legends,
  and callouts
- [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md): semantic model for discrete legends,
  continuous colorbars, and shared explanatory mappings
- [DIAGNOSTICS_SCHEMA.md](DIAGNOSTICS_SCHEMA.md): shared conceptual diagnostic shape across
  validation, adaptation, planning, and runtime execution
- [SCENE_VALIDATION.md](SCENE_VALIDATION.md): scene-level pre-emission validation rules, error
  classes, and capability-gated checks
- [CAPABILITY_ADAPTATION.md](CAPABILITY_ADAPTATION.md): explicit fallback, simplification, and
  deactivation policy driven by runtime capabilities
- [SCENE_API_SKETCH.md](SCENE_API_SKETCH.md): tentative user-facing scene construction surface
  above planning and DRP2
- [EXTERNAL_UI.md](EXTERNAL_UI.md): boundary between scene-owned semantics and app-owned UI
  frameworks such as ImGui
- [INVALIDATION_AND_CACHING.md](INVALIDATION_AND_CACHING.md): rules for dirty scopes, reuse,
  redraw, uploads, and plan rebuilds
- [PICKING.md](PICKING.md): scene-side picking, identity round-trip, grouped hits, and readback
  semantics
- [CONTROLLERS.md](CONTROLLERS.md): event routing, panel-owned navigation, picking-driven
  interaction, and redraw
- [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md): preferred v0.4 visual-family taxonomy grounded in
  local `v0.3` terminology
- [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md): producer-side contract every future visual type must
  satisfy
- [VISUAL_MINI_CONTRACTS.md](VISUAL_MINI_CONTRACTS.md): family-level mini-contracts for the
  current preferred v0.4 visuals
- [RESOURCE_MODEL.md](RESOURCE_MODEL.md): scene-owned logical data model for visuals, planning,
  upload, and readback
- [TRANSFORM_PIPELINE.md](TRANSFORM_PIPELINE.md): explicit data-normalization and panel-transform
  pipeline for scene visuals
- [FRAME_PLAN_IR.md](FRAME_PLAN_IR.md): producer-side intermediate representation for one planned
  frame
- [FRAME_LIFECYCLE.md](FRAME_LIFECYCLE.md): update/build/emit flow
- [RUNTIME_BOUNDARY.md](RUNTIME_BOUNDARY.md): allowed and forbidden dependencies on the runtime
  layer
- [RUNTIME_SERVICE_SKETCH.md](RUNTIME_SERVICE_SKETCH.md): minimal conceptual runtime service
  surface below scene planning and above backend execution
- [USE_CASES.md](USE_CASES.md): pressure-test scenarios
- [examples/README.md](examples/README.md): worked scene-spec examples that instantiate families,
  transforms, and `FramePlan` shapes


## Next Steps

The current scene spec now covers:

1. object model,
2. visual families and family mini-contracts,
3. resource model,
4. transform pipeline,
5. frame planning,
6. axes,
7. invalidation and caching,
8. picking,
9. controllers and interaction,
10. animation and frame scheduling,
11. worked examples.

The next recommended iterations are:

1. public C API surface — resolving open choices in `SCENE_API_SKETCH.md` and
   `PREFERRED_API_PROFILE.md`,
2. axes ↔ domain and panel binding spec,
3. more worked examples, especially multi-panel, animation, and video export cases,
4. optional family deep dives for major families such as `point`, `path`, `image`, `sphere`, and
   `volume`.

The general rule should remain:

1. keep pushing scene semantics and producer contracts,
2. avoid freezing backend-shaped details too early,
3. let DRP2 and runtime work continue underneath without leaking upward.
