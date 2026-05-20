# Scene Capability Adaptation

> **Status:** normative capability adaptation model for v0.4.
> **Authority:** this file defines what happens when validated scene intent meets a runtime
> capability set that cannot provide the preferred path. Validation layers live in
> [`VALIDATION.md`](VALIDATION.md); diagnostics live in [`DIAGNOSTICS.md`](DIAGNOSTICS.md).


## Purpose

Capability adaptation is a scene-side policy step between validation, runtime capability query,
`FramePlan` construction, and DRP2 emission. It keeps fallback behavior deterministic and prevents
silent degradation from changing scene meaning.


## Core Rules

1. Preserve semantic clarity before visual resemblance.
2. Choose one explicit outcome: accept, simplify, deactivate, or reject.
3. Keep backend-specific toggles out of public scene semantics.
4. Run adaptation on the smallest correct scope.
5. Emit scene-visible diagnostics whenever the preferred path was not taken.
6. Plan only from adapted intent; do not build a `FramePlan` and then discover unsupported
   requirements.


## Capability Boundary

The scene consumes declarative runtime capability reports from the runtime/DRP2 boundary. It may use
capability fields, DRP2-visible feature support, readback/offscreen services, and resource limits.
It must not depend on Vulkan constants, private handles, backend allocation details, or private
command-buffer behavior.

Canonical boundaries:

| Topic | Canonical document |
|---|---|
| DRP2 capabilities | [`../../drp2/CAPABILITIES.md`](../../drp2/CAPABILITIES.md) |
| Runtime boundary | [`../core/RUNTIME_BOUNDARY.md`](../core/RUNTIME_BOUNDARY.md) |
| Frame lifecycle | [`../pipeline/FRAME_LIFECYCLE.md`](../pipeline/FRAME_LIFECYCLE.md) |
| Invalidation | [`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md) |
| Transparency policy | [`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md) |
| Diagnostics schema | [`DIAGNOSTICS.md`](DIAGNOSTICS.md) |


## Outcomes

| Outcome | Meaning | Allowed when |
|---|---|---|
| Accept | preferred scene intent is fully supported | no semantic or quality change is needed |
| Simplify | use a reduced but semantically acceptable form | only quality, performance, or optional affordance changes |
| Deactivate | omit the affected feature/object with a diagnostic | the object contract permits absence and the rest of the scene remains coherent |
| Reject | fail the configuration | no honest simplification exists, or the feature is semantically required |


## Semantic Preservation

| Class | Rule | Examples |
|---|---|---|
| Semantic requirement | reject or deactivate if unavailable | scalar colorbar cannot become unrelated categorical legend; picking workflow cannot claim support without identity round-trip; 3D volume cannot silently become a semantically different 2D image |
| Quality preference | simplify with a warning when meaning remains intact | lower annotation decoration, lower precision when optional, simpler legend marks, lower-fidelity transparency realization |
| Optional affordance | deactivate with a recoverable diagnostic | interactive legend picking, decorative overlays, timestamp-query diagnostics, hover probes |

When multiple adaptations are possible, prefer preserving meaning, then object visibility, then
interaction that does not alter meaning, then visual fidelity.


## Adaptation Order And Scope

1. Validate preferred scene semantics.
2. Classify requirements, preferences, and optional affordances.
3. Consult runtime capabilities.
4. Choose an explicit outcome.
5. Record diagnostics and invalidation consequences.
6. Build the plan from adapted state.

Useful scopes are one visual, annotation, legend/colorbar, panel target path, export request, or
full scene build. Small-scope adaptation avoids degrading unrelated objects.


## Capability Pressure Table

| Capability class | Acceptable simplification | Must reject/deactivate when |
|---|---|---|
| Compute | replace acceleration with non-compute path; disable optional effect | compute is semantically essential |
| Readback/picking | disable optional hover; keep display while disabling optional interaction | stable identity round-trip is unavailable for required selection/query |
| Offscreen/export | reduce annotation set; remove interactive overlays from headless output | deterministic target or readback is required and unsupported |
| Formats/limits | reduce optional samples or split allowed derived artifacts | unsupported format/limit would rescale, truncate, or reinterpret data meaning |
| FP64/precision | use FP32 with warning when FP64 is preferred, not required | FP64 is required for correctness |
| Determinism | continue best-effort or warn on unsatisfied preference | deterministic export, test, or replay is required |
| Hardware ray tracing | fall back to rasterization when preferred; use rasterization when off | ray tracing is declared required and unavailable |
| Transparency | choose lower-quality realization while preserving emphasis relationships | selected/context visibility semantics cannot be represented honestly |

Weighted blended OIT is selected from lower-level capabilities, not from a single standalone flag:
floating-point render targets, blending, attachment count, and pass structure must all be available.


## Domain Pressure Table

| Domain | Priority order |
|---|---|
| Primary visuals | preserve data meaning; keep object visible if honest; reduce fidelity last |
| Annotations | preserve required explanatory meaning; drop decoration, dense overlays, optional probes, or pickability first |
| Legends/colorbars | preserve mapping, labels/ticks, placement, decoration/interactivity in that order |
| Multi-panel scenes | adapt panel-locally where possible; do not degrade all panels because one panel loses an optional path |
| Export | preserve reproducibility and declared deterministic requirements before convenience overlays |


## Transparency And Emphasis

Transparency is scene-visible styling and emphasis, not a backend algorithm choice. Adaptation must
preserve semantic relationships such as selected objects being more prominent than context objects
before reducing transparency quality. It must not silently make selected and unselected state
equivalent when that difference is meaningful. Detailed pass semantics live in
[`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md).


## Diagnostics And Invalidation

When the preferred path is not taken, emit a diagnostic with:

| Field | Expected value |
|---|---|
| `phase` | `CapabilityAdaptation` |
| `category` | `Capability` |
| `subject_kind` | usually `Visual`, `Annotation`, `Legend`, `Colorbar`, `Panel`, or `Scene` |
| `severity` | `Warning` for simplification, `Recoverable` for deactivation, `Fatal` for rejection |

Deactivated visuals emit no DRP2 commands for the affected frame. Adaptation can invalidate the
frame plan, readback routing, annotation layout, or derived resources according to
[`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md).


## Non-Goals

This document does not freeze the final adaptation-policy API, whether policy is per object/family
or scene-wide, exact runtime capability field names, UI display policy for diagnostics, or which
choices become user-configurable at runtime.
