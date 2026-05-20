# Scene Validation

> **Status:** normative scene validation model for v0.4.
> **Authority:** this file defines validation layers, timing, scope, error shape, and
> fatal-versus-recoverable rules. Capability fallback policy is defined in
> [`ADAPTATION.md`](ADAPTATION.md), and diagnostic record fields are canonical in
> [`DIAGNOSTICS.md`](DIAGNOSTICS.md).


## Purpose

Scene validation rejects invalid scene meaning before it becomes invalid execution. Runtime
submission should receive already-validated scene intent rather than discover ordinary scene
contract errors through backend failures.


## Core Rules

1. Validate scene semantics before `FramePlan` construction and DRP2 emission.
2. Keep diagnostics anchored in scene objects, roles, dependencies, and phases.
3. Run the smallest correct validation scope after a mutation, with full-scene audit available for
   debug paths.
4. Separate invalid scene configuration from insufficient runtime capability.
5. Treat stale pick result application as a validation failure when request identity or generation
   no longer matches current state.


## Validation Layers

| Layer | Checks | Example failures |
|---|---|---|
| Structural | object graph correctness, required references, attachment ownership | missing panel, null resource handle, forbidden duplicate attachment, axis with no domain source |
| Semantic | scene-contract coherence | sampled field where item table is required, colorbar for categorical-only mapping, 2D controller on 3D-only mode, grouped picking without group identity |
| Planning | sufficient deterministic inputs for `FramePlan` construction | ambiguous stage participation, undeclared export attachment, readback path without routing, derived resource without source mapping |
| Capability | validated intent versus declarative runtime capability report | required picking without readback service, compute-required feature on non-compute path, unsupported interactive legend picking |

Capability validation may reject, simplify, or deactivate only through the explicit adaptation model
in [`ADAPTATION.md`](ADAPTATION.md).


## Timing And Scope

Validation is incremental, not one monolithic submit-time pass.

| Moment | Typical checks |
|---|---|
| object creation/attachment | object class, ownership, compatible attachment target |
| property mutation | resource class, controller compatibility, scale-domain shape, obvious schema errors |
| pre-planning | aggregation, export-only placement, target readback requirements, capability-selected variants |
| pre-emission | final plan inputs, routing, dependencies, selected fallbacks |
| debug audit | full-scene consistency and expensive cross-object checks |

| Scope | Use when |
|---|---|
| object | mutation affects only local invariants |
| object plus dependencies | resource, mapping, axis, annotation, or controller links are involved |
| panel | target mode, viewport, navigation, picking, or export state changed |
| visual family instance | resource roles, variants, transforms, picking, or stage participation changed |
| annotation/legend/colorbar group | derived composite semantics or aggregation changed |
| full scene | debug audit, major topology change, or uncertain dependency fan-out |


## Required Validation Areas

| Area | Required checks | Canonical spec |
|---|---|---|
| Scene structure | unique identities where required; no dangling references; valid parent/child and panel-local ownership | [`../core/OBJECT_MODEL.md`](../core/OBJECT_MODEL.md) |
| Panel | dimensionality, controller compatibility, target mode, attached object types, export/offscreen options | [`../core/PANEL_LAYOUT.md`](../core/PANEL_LAYOUT.md) |
| Visual | known family, required resource roles, schema consistency, variants, transforms, stage participation, picking mode | [`../semantics/VISUAL_CONTRACT.md`](../semantics/VISUAL_CONTRACT.md) |
| Resource | class/kind compatibility, schema completeness, dimensional consistency, dirty ranges, writable/readback usage | [`../pipeline/RESOURCE_MODEL.md`](../pipeline/RESOURCE_MODEL.md) |
| Transform | domain availability, normalization policy, panel-local transform state, anchor/placement expectations | [`../pipeline/TRANSFORM_PIPELINE.md`](../pipeline/TRANSFORM_PIPELINE.md) |
| Picking | stable panel/visual/item/group identity, readback routing, hover/selection policy | [`../interaction/PICKING.md`](../interaction/PICKING.md) |
| Annotation | attachment scope, anchor source, placement, content source, interaction, target-mode visibility | [`../semantics/ANNOTATIONS.md`](../semantics/ANNOTATIONS.md) |
| Legend/colorbar | mapping source, continuous/discrete explanation, aggregation, ticks/domain, attachment, interaction | [`../semantics/LEGENDS_AND_COLORBARS.md`](../semantics/LEGENDS_AND_COLORBARS.md) |
| Frame plan | derivable contributions, declared target paths, readback routing, deterministic dependencies, resolved fallbacks | [`../pipeline/FRAME_PLAN.md`](../pipeline/FRAME_PLAN.md) |
| Capability | required features, readback/offscreen/picking/compute services, unsupported target combinations | [`../../drp2/CAPABILITIES.md`](../../drp2/CAPABILITIES.md) |


## Error Shape

Validation diagnostics should use [`DIAGNOSTICS.md`](DIAGNOSTICS.md) and preserve at least:

1. error class/category;
2. failing object identity;
3. failing dependency or role;
4. validation phase;
5. human-readable explanation;
6. optional suggested fix or fallback note.

The final C names may change, but the content stays scene-oriented.


## Fatal, Recoverable, And Warning Conditions

| Severity | Meaning | Examples |
|---|---|---|
| Fatal | planning or emission cannot proceed | missing mandatory resource, invalid attachment, impossible family configuration, incoherent target requirements |
| Recoverable | deterministic fallback or deactivation may proceed | optional annotation dropped, decorative legend mark simplified, optional overlay picking disabled |
| Warning | planning proceeds but condition remains diagnosable | redundant legend for unused mapping, suspicious but legal ordering, annotation density likely to clutter |

Fatal validation means the scene is wrong. Recoverable adaptation means the preferred scene was
coherent but the selected runtime path cannot realize it exactly.


## Relationships

| Topic | Relationship |
|---|---|
| Invalidation | Invalidation decides what must be recomputed; validation decides whether recomputation can proceed. See [`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md). |
| Frame lifecycle | Validation runs after input/controller/resource dirtiness resolution and before `FramePlan` build. See [`../pipeline/FRAME_LIFECYCLE.md`](../pipeline/FRAME_LIFECYCLE.md). |
| Runtime boundary | Validation may use declarative capability, readback, offscreen, and DRP2-visible concepts, but not Vulkan handles, swapchain internals, allocators, or command-buffer details. See [`../core/RUNTIME_BOUNDARY.md`](../core/RUNTIME_BOUNDARY.md). |
| Capability adaptation | If preferred semantics validate but capabilities are insufficient, use [`ADAPTATION.md`](ADAPTATION.md). |


## Non-Goals

This document does not freeze final error-code enums, callback/logging APIs, release-build pruning,
deferred reporting policy, validation-only dry-run APIs, or backend validation behavior.
