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


## Documents

- `REQUIREMENTS.md`: what the scene layer needs from DRP2 and the runtime
- `OBJECT_MODEL.md`: minimum stable concepts
- `PREFERRED_API_PROFILE.md`: the current preferred scene-facing API defaults selected from the API sketch
- `IMPLEMENTATION_BRIDGE.md`: tentative C-facing type and operation mapping derived from the current scene spec
- `headers/`: non-authoritative draft headers pressure-testing the current scene, runtime, and diagnostics surfaces
- `AXES.md`: scene-side semantic model for axes, ticks, labels, and related annotations
- `ANNOTATIONS.md`: semantic model for labels, guides, probes, overlays, legends, and callouts
- `LEGENDS_AND_COLORBARS.md`: semantic model for discrete legends, continuous colorbars, and shared explanatory mappings
- `DIAGNOSTICS_SCHEMA.md`: shared conceptual diagnostic shape across validation, adaptation, planning, and runtime execution
- `SCENE_VALIDATION.md`: scene-level pre-emission validation rules, error classes, and capability-gated checks
- `CAPABILITY_ADAPTATION.md`: explicit fallback, simplification, and deactivation policy driven by runtime capabilities
- `SCENE_API_SKETCH.md`: tentative user-facing scene construction surface above planning and DRP2
- `EXTERNAL_UI.md`: boundary between scene-owned semantics and app-owned UI frameworks such as ImGui
- `INVALIDATION_AND_CACHING.md`: rules for dirty scopes, reuse, redraw, uploads, and plan rebuilds
- `PICKING.md`: scene-side picking, identity round-trip, grouped hits, and readback semantics
- `CONTROLLERS.md`: event routing, panel-owned navigation, picking-driven interaction, and redraw
- `VISUAL_FAMILIES.md`: preferred v0.4 visual-family taxonomy grounded in local `v0.3` terminology
- `VISUAL_CONTRACT.md`: producer-side contract every future visual type must satisfy
- `VISUAL_MINI_CONTRACTS.md`: family-level mini-contracts for the current preferred v0.4 visuals
- `RESOURCE_MODEL.md`: scene-owned logical data model for visuals, planning, upload, and readback
- `TRANSFORM_PIPELINE.md`: explicit data-normalization and panel-transform pipeline for scene visuals
- `FRAME_PLAN_IR.md`: producer-side intermediate representation for one planned frame
- `FRAME_LIFECYCLE.md`: update/build/emit flow
- `RUNTIME_BOUNDARY.md`: allowed and forbidden dependencies on the runtime layer
- `RUNTIME_SERVICE_SKETCH.md`: minimal conceptual runtime service surface below scene planning and above backend execution
- `USE_CASES.md`: pressure-test scenarios
- `examples/`: worked scene-spec examples that instantiate families, transforms, and `FramePlan`
  shapes


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
10. worked examples.

The next recommended iterations are:

1. more worked examples, especially multi-panel and annotation-heavy cases,
2. optional family deep dives for major families such as `point`, `path`, `image`, `sphere`, and
   `volume`.

The general rule should remain:

1. keep pushing scene semantics and producer contracts,
2. avoid freezing backend-shaped details too early,
3. let DRP2 and runtime work continue underneath without leaking upward.
