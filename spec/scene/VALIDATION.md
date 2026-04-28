# Scene Validation

This document defines how validation should work in the future scene layer.

Scene validation is a producer-side semantic check.

It exists to reject invalid scene states before DRP2 emission and before runtime execution has to
guess what the scene meant.


## Purpose

Scene validation should:

1. detect invalid scene configurations early,
2. keep error reporting anchored in scene semantics rather than backend failures,
3. make planning preconditions explicit,
4. separate scene-level invalidity from runtime capability mismatch,
5. support deterministic `FramePlan` construction.


## Position

Scene validation sits between:

1. authored scene state,
2. invalidation and update resolution,
3. `FramePlan` construction,
4. DRP2 emission.

The intended order is:

1. scene state mutates,
2. dirty scopes are resolved,
3. the scene validates the relevant semantic objects and dependencies,
4. planning proceeds only if the required validation level is satisfied,
5. runtime submission only sees already-validated scene intent.


## Core Rule

Scene validation should reject invalid meaning before it becomes invalid execution.

That means the scene layer should detect errors such as:

1. a visual missing a required resource role,
2. a panel using an incompatible visual mode,
3. a colorbar requested for a mapping that does not exist,
4. a pickable annotation with no stable identity model,
5. an impossible aggregation of legend semantics.

It should avoid deferring those errors into:

1. backend pipeline failures,
2. missing shader resources at runtime,
3. silent best-effort behavior,
4. cryptic execution-time diagnostics.


## Non-Goals

This document does not define:

1. the exact public error-code enum,
2. the exact callback or logging API,
3. the exact recovery policy for every error,
4. backend validation behavior,
5. performance profiling of validation work.


## Validation Layers

The scene spec should recognize at least four validation layers:

1. structural validation,
2. semantic validation,
3. planning validation,
4. capability validation.


### Structural Validation

Structural validation checks object graph correctness and required references.

Examples:

1. a panel reference points to a missing panel,
2. a visual is attached twice to the same panel when that is forbidden,
3. a resource handle is null or missing,
4. an axis refers to a missing domain source.


### Semantic Validation

Semantic validation checks whether the scene configuration makes sense at the scene-contract level.

Examples:

1. a `point` visual receives a `SampledField` where an `ItemTable` is required,
2. a colorbar is requested for a categorical-only mapping,
3. grouped picking is enabled for a visual that has no group identity,
4. a 2D-only controller is attached to a 3D panel mode.

Semantic validation should also reject:

1. implicit legend or colorbar aggregation when mapping identities are not semantically identical,
2. stale pick result application when the request identity or generation no longer matches current
   scene state.


### Planning Validation

Planning validation checks whether the currently valid scene state is sufficient to build a
deterministic `FramePlan`.

Examples:

1. stage participation is ambiguous,
2. one target mode requires a readback path that is not fully specified,
3. export requires an attachment that was never declared,
4. a visual requires a derived resource that has no valid source mapping.


### Capability Validation

Capability validation checks whether the runtime contract can satisfy the scene’s validated intent.

Examples:

1. the scene requires picking but the runtime path does not expose the needed readback service,
2. a visual requires compute-assisted planning and the active capability set forbids it,
3. an interactive legend requests pickable entries but the target runtime disables picking support.

Capability validation still belongs in the scene layer if the failure is visible at the scene
contract boundary.


## When Validation Happens

Validation should not be a single monolithic pass only at submit time.

The scene spec should allow validation at several moments:

1. object creation or attachment time,
2. property mutation time,
3. pre-planning time,
4. pre-emission time,
5. optional debug-time full-scene audit.


## Eager Versus Deferred Validation

Some checks should happen eagerly and some may be deferred.


### Eager Checks

These should usually happen as soon as the relevant mutation occurs.

Examples:

1. wrong resource class attached to a visual role,
2. incompatible controller attached to a panel,
3. missing required label source for an annotation object,
4. invalid scale-domain shape for a newly configured colorbar.


### Deferred Checks

These may be deferred until planning because they depend on wider scene context.

Examples:

1. conflicting aggregation of several visuals into one legend,
2. export-only annotation placement conflicts,
3. capability-gated variant fallback resolution,
4. target-specific readback requirements.


## Validation Scope

Validation should run on the smallest correct scope, just like invalidation.

Useful validation scopes include:

1. one object,
2. one object plus its direct dependencies,
3. one panel,
4. one visual family instance,
5. one annotation group,
6. one full scene audit.

This keeps interaction and incremental updates responsive.


## What Must Be Validated

The first scene slice should validate at least:

1. scene structure,
2. panel compatibility,
3. visual contracts,
4. resource contracts,
5. transform prerequisites,
6. picking prerequisites,
7. annotation contracts,
8. legend and colorbar contracts,
9. frame-planning prerequisites,
10. capability-gated requirements.


## Scene Structure Validation

The scene layer should validate:

1. unique logical identities where required,
2. valid parent-child or attachment relationships,
3. absence of dangling references,
4. no duplicate forbidden attachments,
5. valid target ownership for panel-local objects.

Typical failures:

1. one visual attached to a panel after the visual was removed from the scene,
2. one annotation references a deleted visual,
3. one axis or controller is attached to a non-existent panel,
4. one readback target belongs to the wrong scene.


## Panel Validation

Each panel should be validated for:

1. dimensionality and controller compatibility,
2. valid target mode,
3. valid attached object types,
4. consistent panel-local transform expectations,
5. legal export or offscreen options.

Typical failures:

1. 3D camera controller on a 2D-only panel mode mismatch,
2. one panel requests picking outputs without a valid picking path,
3. one annotation requires viewport-relative placement on an incompatible panel target model.


## Visual Validation

Each visual should be validated against its family contract.

Checks should include:

1. family identity is known,
2. required resource roles are present,
3. optional roles obey family-specific constraints,
4. parameter schema is internally consistent,
5. variant axes are valid,
6. transform requirements are satisfied,
7. stage participation is coherent,
8. picking mode is coherent.

Typical failures:

1. `path` visual missing grouped item data,
2. `image` visual configured with incompatible sampled-field dimensionality,
3. `volume` visual requesting a 2D-only variant,
4. `marker` visual declaring picking without stable item identity,
5. visual variant flags that cannot coexist.


## Resource Validation

Scene resources should be validated for:

1. class and kind compatibility,
2. schema completeness,
3. shape and dimensional consistency where required,
4. stable ownership scope,
5. valid dirty-range declarations,
6. readback versus writable usage consistency.

Typical failures:

1. `GroupedItemTable` attached where a flat `ItemTable` is required,
2. one `SampledField` missing domain semantics needed by a colorbar,
3. one `ReadbackTarget` used as if it were a source resource,
4. conflicting writable/read-only expectations in one plan path.


## Transform Validation

The scene should validate transform prerequisites from
[pipeline/TRANSFORM_PIPELINE.md](pipeline/TRANSFORM_PIPELINE.md).

Checks should include:

1. required data-space domain information exists,
2. normalization policy is defined when needed,
3. panel-local transform state exists for panel-aware objects,
4. anchor-space and placement-space expectations are coherent.

Typical failures:

1. axis created without a resolvable data domain,
2. annotation claims data-space anchoring without domain semantics,
3. one visual requires normalization but no normalization policy exists.


## Picking Validation

The scene should validate picking contracts from
[interaction/PICKING.md](interaction/PICKING.md).

Checks should include:

1. stable panel identity,
2. stable visual identity,
3. item or group identity where the family requires it,
4. valid readback routing,
5. coherent hover or selection policy.

Typical failures:

1. grouped picking enabled for anonymous batched data with no group model,
2. pick result routing refers to a removed visual,
3. pickable annotation has no stable annotation identity,
4. one panel requests picking without the required readback sink configuration.


## Annotation Validation

The scene should validate annotation contracts from
[ANNOTATIONS.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/ANNOTATIONS.md).

Checks should include:

1. valid attachment scope,
2. valid anchor source,
3. valid placement policy,
4. valid content source,
5. coherent interaction policy,
6. valid target-mode visibility policy.

Typical failures:

1. one annotation references a missing anchor object,
2. one crosshair is declared as scene-shared despite using panel-local pointer state,
3. one probe requests sampled-value readout without a readable source mapping,
4. one export-only overlay is attached to an incompatible target mode.


## Legend And Colorbar Validation

The scene should validate contracts from
[LEGENDS_AND_COLORBARS.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/LEGENDS_AND_COLORBARS.md).

Checks should include:

1. valid mapping source,
2. coherent discrete versus continuous explanation type,
3. valid aggregation across visuals,
4. valid tick and domain policy,
5. valid panel or scene attachment scope,
6. valid interaction policy when entries are pickable.

Typical failures:

1. a colorbar requested for a visual that exposes only categorical style states,
2. one combined legend merges visually similar but semantically distinct mappings,
3. a log-scale colorbar is configured with a non-positive domain,
4. a legend entry source is missing labels for user-visible categories,
5. one interactive legend requires picking but the scene has not enabled the corresponding path.


## Frame-Planning Validation

Before building or reusing a `FramePlan`, the scene should validate that planning inputs are
sufficient and coherent.

Checks should include:

1. every required visual contribution can be derived,
2. every required target path is declared,
3. every readback path has routing information,
4. stage participation and dependencies are deterministic,
5. capability-selected fallbacks are already resolved.

Typical failures:

1. one export request needs a panel-local overlay path that was never declared,
2. one visual requires a picking pass while the panel has no picking target configuration,
3. a fallback choice leaves stage participation ambiguous.


## Capability Validation

Scene validation should explicitly model capability-gated failures rather than letting them appear as
backend surprises.

Checks should include:

1. required feature availability,
2. required offscreen or readback services,
3. required picking services,
4. required compute services when the scene contract demands them,
5. known unsupported target combinations.

The scene may respond in one of three ways:

1. reject the configuration,
2. choose an explicit fallback,
3. mark the object inactive with a scene-visible diagnostic.

The key rule is that the outcome must be explicit and deterministic.


## Error Shape

Scene validation errors should preserve scene semantics in their diagnostics.

A useful conceptual error payload should be able to report:

1. error class,
2. failing object identity,
3. failing dependency or role,
4. validation phase,
5. human-readable explanation,
6. optional suggested fix or fallback note.

The exact final API shape is open, but the content should remain scene-oriented.


## Fatal Versus Recoverable Failures

The scene spec should distinguish between:

1. fatal validation failures,
2. recoverable validation failures,
3. warning-level questionable configurations.


### Fatal

Fatal failures prevent planning or emission.

Examples:

1. missing mandatory resource,
2. invalid panel or visual attachment,
3. impossible family configuration,
4. incoherent target requirements.


### Recoverable

Recoverable failures allow a deterministic fallback or deactivation.

Examples:

1. optional annotation dropped because a capability is unavailable,
2. decorative legend sample mark simplified,
3. optional picking on a non-critical overlay disabled.


### Warning-Level

Warnings do not block planning but should remain diagnosable.

Examples:

1. redundant legend object explaining an unused mapping,
2. suspicious but still legal entry ordering,
3. annotation density likely to cause visual clutter.


## Relationship To Invalidation

Validation and invalidation are related but not identical.

The key rules should be:

1. invalidation decides what must be recomputed,
2. validation decides whether recomputation can proceed safely,
3. changed objects should usually revalidate only the affected scopes,
4. full-scene revalidation should remain available for debug or audit paths.


## Relationship To Frame Lifecycle

Validation should fit the frame stages in
[pipeline/FRAME_LIFECYCLE.md](pipeline/FRAME_LIFECYCLE.md).

The preferred ordering is:

1. input and controller updates mutate scene state,
2. resource and transform dirtiness are resolved,
3. affected objects are validated,
4. `FramePlan` is built,
5. DRP2 is emitted.

The scene should not rely on runtime submission to discover ordinary scene-contract errors.


## Relationship To Runtime Boundary

Validation must respect the runtime boundary in
[RUNTIME_BOUNDARY.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/RUNTIME_BOUNDARY.md).

That means scene validation may talk about:

1. capability query results,
2. readback availability,
3. offscreen target services,
4. DRP2-visible resource and submission concepts.

It must not depend on:

1. Vulkan handles,
2. swapchain internals,
3. backend allocators,
4. backend command-buffer details.


## What This Document Intentionally Leaves Open

This document intentionally does not freeze:

1. whether validation is synchronous only or also supports deferred reporting,
2. the exact error-code taxonomy,
3. the exact warning and logging policy,
4. whether some validation is compiled out in release builds,
5. the exact API for validation-only dry runs.


## Immediate Follow-Up

`ADAPTATION.md` is the natural follow-on: it defines what happens when validation
passes but the runtime contract is insufficient for the preferred configuration.
