# GUI Viewport Presentation

Status: normative target architecture for post-v0.4 embedded-viewport synchronization. The active v0.4 implementation remains synchronous where required for correctness.

## Purpose

An embedded `DvzGuiViewport` renders an offscreen Datoviz source view and samples its completed image inside a host Dear ImGui frame. This contract defines the ownership, freshness, synchronization, resize, and lifetime rules needed to remove steady-state device-wide waits without adding a parallel renderer or presentation path.

## Ownership

The source `DvzView` owns scene rendering, its offscreen canvas, and produced image generations. The GUI viewport borrows completed source images for display. The app owns scheduling and the handoff between source completion and host-frame preparation. Canvas, stream, DRP2, and the backend runtime retain their existing command, image, and synchronization ownership.

The GUI layer must not begin, end, submit, transition, or destroy source-owned command buffers or images. The source must not destroy or overwrite an image generation while a submitted host GUI frame may still sample it.

## Completion Model

A visible viewport may display the newest completed source generation while a newer generation is still rendering. One-frame latency is an acceptable consequence of non-blocking presentation. Sampling incomplete work, publishing generations out of order, or allowing staleness to grow without a bounded cause is not.

The target flow is:

1. resize, input, scene mutation, animation, or explicit demand marks the source dirty;
2. the app admits and submits bounded source work;
3. the GUI continues to reference the last completed compatible image;
4. completion is observed in a later host iteration without a device-wide idle wait;
5. the newest completed compatible generation is atomically promoted for GUI sampling;
6. retired images and synchronization resources are reclaimed only after both source and host use are complete.

Steady-state embedded presentation must not call a device-wide idle wait. Any required wait should be scoped to the source generation or frame resource whose completion is actually required.

## Scheduling And Fairness

An idle embedded source does not render merely because its host GUI renders. Repeated dirty requests coalesce while source work is pending. Multiple visible viewports receive bounded fair admission; one continuously changing source must not indefinitely prevent another source from presenting.

Source submissions should be collected before completion handling where the backend permits batching. The implementation must not serialize multiple independent viewport sources through one device-wide wait per source.

If no completed compatible image exists yet, the viewport may defer drawing or display an explicit placeholder according to existing creation behavior. It must not expose uninitialized or partially rendered storage.

## Resize And Generations

Logical extent, framebuffer extent, device scale, source resource generation, submitted generation, completed generation, and displayed generation remain distinct facts.

During resize, the previous completed image remains displayable until a new image matching the committed request is complete. A completion from an obsolete size or resource generation is retired without becoming visible. Rapid resize requests may coalesce, but final committed size must make bounded progress.

## Visibility And Input

Hidden or collapsed viewports preserve the existing no-work policy unless explicitly configured to render while hidden. Hiding a viewport suppresses new presentation work but does not permit unsafe reclamation of an in-flight source or host reference.

Input forwarding continues to target source logical coordinates and may mark the source dirty. Displaying a previous completed image does not change controller event ordering or permit input to mutate GPU resources outside the owner-thread contract.

## Failure And Teardown

Source render failure retains the last valid compatible image when safe and reports a bounded diagnostic. Device loss, source destruction, host destruction, and GUI viewport destruction cancel future promotion immediately while deferring resource reclamation until outstanding backend and host references are safe.

No failure path may fall back to sampling an incomplete generation or silently resume device-wide waits as the ordinary steady-state policy.

## Implementation Milestones

1. Split source render, completion wait/poll, image promotion, and GUI sampling timings; record source submissions, promotions, reused frames, and waits.
2. Represent submitted and completed source generations explicitly while retaining synchronous behavior.
3. Prove double- or bounded multi-buffered source images for one viewport with last-completed-image presentation.
4. Add batched fair scheduling for multiple viewports, scoped completion, resize-generation rejection, and deferred retirement.
5. Remove the steady-state device-wide wait after native correctness, latency, memory, teardown, and multi-viewport evidence passes; preserve explicit synchronous tools where required.

## Validation

Required evidence includes:

- empty and representative populated embedded-viewport benchmarks;
- idle, continuous input, animation, hide/show, rapid resize, and explicit-frame workloads;
- one and multiple viewport fairness;
- generation ordering and stale-completion rejection;
- source and host destruction with work in flight;
- device-loss and render-failure recovery;
- no steady-state device-wide wait in the promoted path;
- bounded retained images and synchronization resources;
- native validation-layer cleanliness and explicit WebGPU capability behavior.

Wall-clock comparisons are machine-local. CI should assert generation, ownership, scheduling, allocation, and retirement invariants.

## Non-Goals

This contract does not add another renderer, event loop, GUI backend, public synchronization handle, or general external-frame provider API. Broader provider and copied-frame interoperability remains owned by the future interoperability architecture.
