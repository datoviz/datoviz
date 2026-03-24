# Scene Runtime Service Sketch

This document sketches the minimal conceptual runtime service surface below scene planning.

It refines `RUNTIME_BOUNDARY.md` into a smaller set of service concepts without freezing a final C
API.


## Position

This document sits:

1. below `FRAME_PLAN_IR.md`,
2. below `SCENE_VALIDATION.md` and `CAPABILITY_ADAPTATION.md`,
3. above any backend-specific execution system,
4. alongside `RUNTIME_BOUNDARY.md` as a more concrete service sketch.


## Normative Status

This document is normative for the current runtime-facing service model at the conceptual level.

It is not normative for:

1. final function names,
2. final ownership mechanics in C,
3. whether some services are represented by one object or several helper objects.


## Core Rule

The runtime service should consume already-planned scene work, expose capabilities needed before
planning, and return execution outcomes in scene-visible terms.


## Minimum Service Objects

The minimum conceptual runtime surface should include:

1. `RuntimeService`: the scene-facing execution service entry point,
2. `CapabilitySnapshot`: a stable capability record suitable for validation and adaptation,
3. `SubmissionResult`: execution acceptance or failure at submission time,
4. `CompletionEvent`: typed completion for readback, picking, export, or execution failure,
5. `RuntimeDiagnostic`: runtime-facing diagnostics mapped back to scene-visible identities.


## `CapabilitySnapshot`

`CapabilitySnapshot` should provide a stable scene-consumable view of execution constraints.

It should include at least:

1. supported formats and limits,
2. sample-count support,
3. readback support,
4. picking-related target constraints,
5. compute and precision support,
6. determinism-relevant export constraints.

The scene should be able to retain and compare snapshots across frames without backend handle
inspection.


## `RuntimeService`

`RuntimeService` should be conceptually able to:

1. provide a `CapabilitySnapshot`,
2. accept one scene-level `FramePlan` for execution,
3. expose completion events for readback-oriented work,
4. expose runtime diagnostics in scene-visible terms.

Conceptually:

```text
caps = runtime_query_capabilities(runtime)
submission = runtime_submit(runtime, frame_plan)
event = runtime_poll_completion(runtime)
```


## Submission Model

Submission should consume one scene-level `FramePlan` as the unit of scene-visible execution.

The runtime may internally translate that plan into:

1. one backend submission,
2. several backend submissions,
3. internal caching or deferred object creation.

But those internal details should not alter:

1. the meaning of the submitted frame,
2. the identity route for diagnostics,
3. the identity route for completions.


## `SubmissionResult`

`SubmissionResult` should report whether the runtime accepted the planned work for execution.

It should distinguish at least:

1. accepted for execution,
2. rejected due to execution-time capability mismatch,
3. rejected due to invalid runtime resource state,
4. rejected due to internal runtime failure.

If submission is rejected, the result should include scene-visible diagnostics rather than backend
handles.


## Completion Model

Completion delivery should preserve scene-facing identity and freshness semantics.

The runtime may support:

1. synchronous helper completion,
2. asynchronous polling,
3. asynchronous callback or queue delivery.

Whichever transport is chosen, completion should preserve:

1. submission identity,
2. request or target identity,
3. completion kind,
4. stale-result rejection information when the scene model requires it.


## `CompletionEvent`

`CompletionEvent` should support at least:

1. picking completion,
2. offscreen image completion,
3. optional compute-result completion for tests or tooling,
4. execution failure completion when the failure occurs after submission acceptance.

Each completion should identify:

1. the originating submission or frame identity,
2. the logical request or target identity,
3. the typed payload,
4. any freshness or discardability metadata needed by the scene.


## `RuntimeDiagnostic`

`RuntimeDiagnostic` should map failures back to scene-visible execution artifacts.

It should be able to refer to:

1. plan node identity,
2. logical target identity,
3. logical resource identity,
4. failure category,
5. optional backend detail kept as opaque debug text rather than required semantic input.


## Service Boundaries

The runtime service should not require the scene layer to expose:

1. Vulkan, Metal, WebGPU, GLFW, or swapchain handles,
2. backend descriptor or pipeline object identifiers,
3. backend synchronization primitives,
4. backend allocator state.

The scene layer should not ask the runtime service to:

1. reinterpret scene semantics,
2. choose scene fallback policy after planning,
3. discover missing validation that should already have failed earlier.


## Relationship To Other Scene Docs

This document should be read together with:

1. `RUNTIME_BOUNDARY.md` for the higher-level allowed/forbidden dependency rule,
2. `FRAME_PLAN_IR.md` for the producer-side artifact this service consumes,
3. `SCENE_VALIDATION.md` and `CAPABILITY_ADAPTATION.md` for the stages that must run before
   submission,
4. `PICKING.md` for freshness and identity semantics on picking completions.


## Deferred Questions

The following questions remain intentionally open:

1. whether the final runtime surface is one object or several cooperating interfaces,
2. whether completion delivery is callback-based, polled, or both,
3. how much diagnostic detail is standardized versus debug-only,
4. whether some export-oriented helpers live above or below the strict runtime boundary.
