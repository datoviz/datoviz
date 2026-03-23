# Scene Frame Lifecycle

This document defines the logical frame flow for the future scene layer.


## High-Level Flow

1. ingest events,
2. update controllers,
3. update animations,
4. update cameras and derived transforms,
5. resolve resource dirtiness,
6. build a frame plan,
7. emit DRP2,
8. submit through the runtime,
9. process readback or picking results.


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


### 3. Resource Update

Identify changed CPU-owned resources and subranges.
The scene layer should emit only the required DRP2 resource writes for the current frame.


### 4. Frame Planning

Build a per-frame execution plan that decides:

1. target attachments,
2. render stages,
3. compute stages if needed,
4. ordering and dependencies,
5. offscreen and picking paths.


### 5. DRP2 Emission

Emit:

1. resource creates, if needed,
2. resource writes for dirty content,
3. shader and pipeline commands for newly-required variants,
4. pass commands,
5. draw and dispatch commands,
6. submission.


### 6. Post-Frame Readback

Interpret picking or offscreen readback results at the scene level.


## Rules

1. Scene state update happens before DRP2 emission.
2. Scene should not mutate state while a frame plan is being emitted.
3. DRP2 emission should be a deterministic function of scene state plus runtime capabilities.
4. Runtime failures must map back to scene-visible diagnostics without backend leakage.
