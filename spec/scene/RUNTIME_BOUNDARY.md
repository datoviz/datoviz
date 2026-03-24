# Scene Runtime Boundary

This document defines the allowed contract between the future scene layer and the DRP2 runtime.


## Position

The runtime boundary sits below scene validation, capability adaptation, and `FramePlan`
construction.

The intended relationship is:

1. scene owns authored semantics, dirty tracking, validation, and adaptation,
2. scene builds one scene-level `FramePlan` for the frame,
3. the runtime consumes that plan through DRP2-visible services,
4. the runtime reports execution outcomes without redefining scene meaning.

This document should be read together with:

1. `SCENE_VALIDATION.md` for what must fail before execution,
2. `CAPABILITY_ADAPTATION.md` for how capability-driven simplification or rejection is chosen,
3. `FRAME_PLAN_IR.md` for the producer-side plan shape the runtime receives.


## Core Rule

The runtime is an execution service for already-planned scene work.

It may expose execution capabilities and execution results, but it should not act as a second scene
planner or require backend-shaped data in the public scene surface.


## Allowed Dependencies

The scene layer may depend on:

1. capability query,
2. error reporting,
3. shader ingestion through DRP2-visible concepts,
4. resource creation and update through DRP2-visible concepts,
5. command-stream submission,
6. readback and offscreen target services expressed without backend handle leakage.


## Required Runtime Service Surface

The minimum runtime-facing service surface should include:

1. capability snapshot query with stable scene-consumable semantics,
2. execution submission for already-built plan work,
3. offscreen-target provisioning through logical target descriptions,
4. readback request completion and typed result delivery,
5. runtime diagnostics that refer back to scene-visible plan or resource identity,
6. optional frame timing or debug hooks that remain observational rather than semantic.


## Capability Snapshot Contract

Capability query should provide a stable snapshot suitable for validation and adaptation.

At minimum, the scene should be able to consume capability information such as:

1. supported formats and limits,
2. sample-count availability,
3. readback support,
4. picking-related target constraints,
5. compute and precision availability,
6. determinism-relevant constraints needed by export or testing paths.

The snapshot should be explicit enough that:

1. adaptation can run before planning,
2. planning does not need backend object inspection,
3. diagnostics can explain capability-driven rejection or simplification at the scene level.


## Submission Contract

Submission should consume already-planned work.

The important rule is:

1. scene decides the topology of the frame plan,
2. runtime executes the translated plan,
3. runtime may cache backend objects internally,
4. runtime should not silently reinterpret scene-level ordering, fallback, or identity.

Submission granularity remains intentionally open:

1. one submission may cover the whole scene-level `FramePlan`,
2. one plan may translate to several backend submissions if the runtime needs that internally,
3. but those internal execution details should not change the scene-visible semantics of one frame
   build.


## Readback Contract

Readback and offscreen export services should be expressed in producer-visible terms.

At minimum, the boundary should support:

1. deterministic offscreen image capture,
2. single-pixel or equivalent picking readback,
3. typed completion routing back to scene-visible request or target identity,
4. stale-result rejection where the scene model requires it, such as hover picking.

Completion timing also remains intentionally open:

1. the runtime may support synchronous completion helpers,
2. the runtime may support asynchronous completion delivery,
3. but the scene-facing result model should preserve request identity and freshness semantics in
   either case.


## Diagnostics Contract

Runtime failures should be reportable in terms that the scene layer can map back to:

1. plan node identity,
2. logical target identity,
3. logical resource identity,
4. capability mismatch or execution failure category.

The runtime may keep backend-specific detail internally, but the scene-facing boundary should not
require backend handles to interpret failures.


## Forbidden Dependencies

The scene layer must not depend on:

1. Vulkan handle types,
2. swapchain internals,
3. backend-specific synchronization objects,
4. backend allocator internals,
5. platform window-system handles,
6. backend command-buffer recording APIs.


## Design Rule

If the scene layer needs a reusable low-level behavior, prefer improving the DRP2 or runtime-facing
contract instead of adding a scene-private backend escape hatch.
