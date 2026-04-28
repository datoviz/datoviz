# Scene Diagnostics Schema

This document defines the preferred conceptual schema for diagnostics across the future scene layer.

It unifies diagnostics emitted by validation, capability adaptation, frame planning, and runtime
execution.


## Normative Status

This document is normative at the conceptual schema level.

It is not normative for:

1. final struct names,
2. final enum names,
3. final memory ownership conventions in C.


## Core Rule

All scene-facing diagnostics should preserve scene meaning first and backend detail second.

That means diagnostics should answer:

1. what failed,
2. at which semantic phase it failed,
3. which scene-visible object or plan artifact it concerns,
4. whether the condition is fatal, recoverable, or advisory.


## Diagnostic Record

The preferred conceptual unit is one `DiagnosticRecord`.

Each record should contain at least:

1. `severity`
2. `phase`
3. `category`
4. `code`
5. `message`
6. `subject_kind`
7. `subject_id`
8. `scope`
9. optional related identities
10. optional contextual payload


## Severity

The preferred severity set is:

1. `Fatal`
2. `Recoverable`
3. `Warning`
4. `Info`

`Fatal` means the relevant stage cannot proceed.


## Phase

The preferred phase set is:

1. `Validation`
2. `CapabilityAdaptation`
3. `FramePlanning`
4. `RuntimeSubmission`
5. `RuntimeCompletion`

This phase field should let the reader understand where the failure or advisory originated without
guessing from the text message.


## Category

Useful category groups include:

1. `Structure`
2. `Resource`
3. `Transform`
4. `Mapping`
5. `Picking`
6. `Annotation`
7. `Capability`
8. `PlanTopology`
9. `Readback`
10. `RuntimeExecution`


## Subject Kind

The preferred subject-kind set is:

1. `Scene`
2. `Panel`
3. `Visual`
4. `Resource`
5. `Axis`
6. `Annotation`
7. `Legend`
8. `Colorbar`
9. `ScaleMapping`
10. `FramePlan`
11. `PlanNode`
12. `RuntimeService`


## Scope

Diagnostics should report the smallest correct affected scope.

Useful scope values include:

1. `Global`
2. `Scene`
3. `Panel`
4. `Visual`
5. `Resource`
6. `PlanNode`
7. `Request`


## Related Identities

One failure often concerns more than one object.

The schema should therefore allow optional related identities such as:

1. panel plus visual,
2. visual plus resource,
3. colorbar plus mapping,
4. plan node plus target,
5. runtime completion plus originating request.


## Contextual Payload

The schema should allow optional structured or semi-structured context such as:

1. requested capability versus available capability,
2. expected resource kind versus actual kind,
3. requested format versus supported formats,
4. stale request generation versus accepted generation,
5. chosen adaptation outcome.

This payload should remain scene-readable and should not require backend handles to interpret.


## Record Shape By Phase

### Validation

Validation diagnostics should typically emphasize:

1. structural or semantic subject identity,
2. missing or incompatible dependencies,
3. whether planning is blocked.


### Capability Adaptation

Capability-adaptation diagnostics should typically emphasize:

1. preferred semantic path,
2. capability mismatch,
3. chosen explicit fallback, simplification, or deactivation outcome.


### Frame Planning

Planning diagnostics should typically emphasize:

1. plan node or topology problem,
2. unresolved dependency,
3. unsupported target or stage arrangement.


### Runtime Submission

Runtime-submission diagnostics should typically emphasize:

1. the submitted scene-level `FramePlan`,
2. the affected plan node, target, or resource identity,
3. acceptance versus rejection at submission time.


### Runtime Completion

Completion diagnostics should typically emphasize:

1. originating request or target identity,
2. completion kind,
3. freshness or discardability state when relevant.


## DRP2 Error Code Mapping

DRP2 error codes (`DRP2_ERR_*`) are protocol-level identifiers that belong below the scene
semantic layer. From the scene's perspective they are non-normative; the scene does not surface
DRP2 error code constants to users.

When a DRP2 execution failure occurs, the runtime maps it to a `DiagnosticRecord` with:

- **phase**: `RuntimeSubmission` (if the error is detected at submission time) or
  `RuntimeCompletion` (if detected during readback or completion routing),
- **category**: `RuntimeExecution`,
- **subject_kind**: `RuntimeService`,
- **severity**: `Fatal` for submission rejection; `Recoverable` if the scene can retry or
  fall back; `Warning` for non-blocking protocol-level anomalies.

The DRP2 error code may appear in the `contextual_payload` field for diagnostics tooling, but
it must not be the primary user-visible error identifier — the scene-visible message should
describe the affected plan node, resource, or target in scene terms.


## Diagnostics Aggregation

The preferred aggregation unit is one `DiagnosticReport`.

A report should contain:

1. the phase or phases covered,
2. one or more `DiagnosticRecord` items,
3. a summary result such as success, degraded success, recoverable failure, or fatal failure.


## Interaction With Existing Scene Docs

This schema should be used by:

1. `SCENE_VALIDATION.md`
2. `CAPABILITY_ADAPTATION.md`
3. `FRAME_PLAN_IR.md`
4. `RUNTIME_BOUNDARY.md`
5. `RUNTIME_SERVICE_SKETCH.md`

Those documents may define additional phase-specific rules, but they should reuse this common
diagnostic shape.


## Implementation Pressure

If implementation work begins, the schema most naturally pressures the codebase toward:

1. one shared diagnostic record type,
2. one shared phase enum,
3. one shared subject-kind enum,
4. one shared report container used across scene and runtime-facing validation paths.
