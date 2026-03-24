# Scene Frame Lifecycle

This document defines the logical frame flow for the future scene layer.


## High-Level Flow

1. ingest events,
2. update controllers,
3. update animations,
4. update cameras and scene-visible derived state,
5. resolve invalidation scopes and resource dirtiness,
6. validate the affected scene state,
7. apply capability adaptation,
8. build a frame plan,
9. emit DRP2,
10. submit through the runtime,
11. process readback or picking results.


## Detailed Stages

### 1. Input Update

Translate raw runtime events into scene-level events and dispatch them to the relevant panel and
controller state.


### 2. State Update

Mutate:

1. camera state,
2. selection and hover state,
3. animations,
4. any derived visual parameters.


### 3. Invalidation And Resource Update

Identify changed CPU-owned resources and subranges.
The scene layer should resolve those changes into planning-visible upload and materialization needs for
the current frame.

This stage should determine:

1. which source resources are dirty,
2. which normalized or derived resources must be recomputed,
3. which invalidation scopes are active,
4. which uploads, creates, or readbacks must appear in the next `FramePlan`.


### 4. Validation

Validate the smallest correct scene scope affected by the current invalidation state.

This stage should determine:

1. whether the current scene structure is coherent,
2. whether required resources and mappings are present,
3. whether transforms, attachments, and grouped data are semantically valid,
4. whether planning may proceed for the affected scope.


### 5. Capability Adaptation

Apply runtime capability information to the already-validated preferred scene intent.

This stage should determine:

1. whether the preferred path is accepted,
2. whether an explicit simplification is selected,
3. whether an affected object or feature is deactivated or rejected,
4. which additional invalidation consequences follow from the chosen adaptation outcome.


### 6. Frame Planning

Build a per-frame execution plan that decides:

1. target attachments,
2. render stages,
3. compute stages if needed,
4. ordering and dependencies,
5. offscreen and picking paths,
6. upload and lazy materialization nodes needed for the frame.


The current spec direction is that this build produces one scene-level `FramePlan` for the frame,
even when the plan contains panel-local nodes or subplans.


### 7. DRP2 Emission

Emit:

1. resource creates, if needed,
2. resource writes for dirty content,
3. shader and pipeline commands for newly-required variants,
4. pass commands,
5. draw and dispatch commands,
6. submission.

Emission should be a translation of the already-built `FramePlan`.

It should not rediscover upload work outside the plan.


### 8. Runtime Submission

Submit the emitted work through the runtime-facing boundary.

This stage should treat the runtime as an execution service, not as a second planner.


### 9. Post-Frame Readback

Interpret picking or offscreen readback results at the scene level.


## Rules

1. Scene state update happens before DRP2 emission.
2. Validation runs after invalidation resolution and before planning.
3. Capability adaptation runs after validation and before planning.
4. Scene should not mutate state while a frame plan is being emitted.
5. DRP2 emission should be a deterministic function of validated and adapted scene state plus runtime
   capabilities.
6. Runtime failures must map back to scene-visible diagnostics without backend leakage.
7. Upload and lazy materialization work should be represented in `FramePlan`, not introduced as an
   execution-time side path.
