# Scene Runtime Boundary And Service Model

This document defines the allowed contract between the scene layer and the DRP2 runtime, and
the minimal conceptual service surface the runtime must expose to the scene.

**The "runtime" here is the DRP2 runtime executor.**
The scene layer emits DRP2 commands — it does not call Vulkan, vklite, or any backend API
directly.
The service concepts defined here are the scene-facing view of the DRP2 runtime interface,
not a separate layer between scene and DRP2.


## Normative Status

This document is normative for:

1. dependency and ownership boundaries (what the scene may and must not rely on),
2. the conceptual runtime service model (objects, contracts, API shape).

It is not normative for:

1. final function names,
2. final ownership mechanics in C,
3. whether some services are represented by one object or several helper objects.

The scene spec and the DRP2 spec are designed in parallel. When the scene spec needs a capability
or behavior that DRP2 does not yet provide, that is a DRP2 spec input, not a reason to add a
scene-side workaround. See `spec/drp2/` for the active DRP2 command surface.


## Position

The runtime boundary sits below:

1. scene validation (`validation/VALIDATION.md`),
2. capability adaptation (`validation/ADAPTATION.md`),
3. `FramePlan` construction (`pipeline/FRAME_PLAN.md`).

The intended relationship is:

1. scene owns authored semantics, dirty tracking, validation, and adaptation,
2. scene builds one scene-level `FramePlan` for the frame,
3. the scene-to-DRP2 converter translates that plan into a DRP2 command stream,
4. the runtime consumes the DRP2 command stream,
5. the runtime reports execution outcomes without redefining scene meaning.


## Core Rules

**Boundary rule:** The runtime is an execution service for already-planned scene work. It may
expose execution capabilities and execution results, but it should not act as a second scene
planner or require backend-shaped data in the public scene surface.

**Service rule:** The runtime service should consume already-planned scene work, expose
capabilities needed before planning, and return execution outcomes in scene-visible terms.

**Design rule:** If the scene layer needs reusable low-level behavior, prefer improving the DRP2
or runtime-facing contract rather than adding a scene-private backend escape hatch.


## Allowed Dependencies

The scene layer may depend on:

1. capability query,
2. error reporting,
3. shader ingestion through DRP2-visible concepts,
4. resource creation and update through DRP2-visible concepts,
5. command-stream submission,
6. readback and offscreen target services expressed without backend handle leakage.


## Forbidden Dependencies

The scene layer must not depend on:

1. Vulkan handle types,
2. swapchain internals,
3. backend-specific synchronization objects,
4. backend allocator internals,
5. platform window-system handles,
6. backend command-buffer recording APIs.


## Minimum Service Objects

The minimum conceptual runtime surface includes:

1. `RuntimeService` — the scene-facing execution entry point,
2. `CapabilitySnapshot` — a stable capability record for validation and adaptation,
3. `SubmissionResult` — execution acceptance or failure at submission time,
4. `CompletionEvent` — typed completion for readback, picking, export, or execution failure,
5. `RuntimeDiagnostic` — runtime failures mapped back to scene-visible identities.

The scene layer interacts through a single opaque `DvzRuntime*` handle for submission and
completion. There is no split device/encoder/queue model exposed to the scene. Encoder lifecycle
and DRP2 session management are hidden inside the runtime.

```c
DvzRuntime* rt = dvz_runtime_create(/* backend init */);
dvz_runtime_submit_commands(rt, command_stream);
dvz_runtime_submit_frame_plan(rt, frame_plan);  // convenience: converts then submits
dvz_runtime_get_capabilities(rt, &caps);
dvz_runtime_destroy(rt);
```


## `CapabilitySnapshot`

`CapabilitySnapshot` provides a stable scene-consumable view of execution constraints.

It should include at least:

1. supported formats and limits,
2. sample-count support,
3. readback support,
4. picking-related target constraints,
5. compute and precision support,
6. determinism-relevant export constraints,
7. lower-level render-pass features needed by transparency techniques.

The scene should be able to retain and compare snapshots across frames without backend handle
inspection.


## `RuntimeService` Conceptual Interface

```text
caps       = runtime_query_capabilities(runtime)
commands   = scene_convert_frame_plan(frame_plan, caps)
submission = runtime_submit_commands(runtime, commands)
event      = runtime_poll_completion(runtime)
```


## `SubmissionResult`

`SubmissionResult` reports whether the runtime accepted the planned work for execution.

It distinguishes at least:

1. accepted for execution,
2. rejected due to execution-time capability mismatch,
3. rejected due to invalid runtime resource state,
4. rejected due to internal runtime failure.

If submission is rejected, the result includes scene-visible diagnostics rather than backend
handles.


## `CompletionEvent`

`CompletionEvent` supports at least:

1. picking completion,
2. offscreen image completion,
3. optional compute-result completion for tests or tooling,
4. execution failure completion when the failure occurs after submission acceptance.

Each completion identifies:

1. the originating submission or frame identity,
2. the logical request or target identity,
3. the typed payload,
4. any freshness or discardability metadata needed by the scene.


## `RuntimeDiagnostic`

`RuntimeDiagnostic` maps failures back to scene-visible execution artifacts.

It refers to:

1. plan node identity,
2. logical target identity,
3. logical resource identity,
4. failure category,
5. optional backend detail kept as opaque debug text rather than required semantic input.


## Capability Snapshot Contract

At minimum, the scene should be able to consume capability information such as:

1. supported formats and limits,
2. sample-count availability,
3. readback support,
4. picking-related target constraints,
5. compute and precision availability,
6. determinism-relevant constraints needed by export or testing paths,
7. buffer alignment limits for dynamic offset planning,
8. color blending and color-attachment limits for transparent rendering.

The following alignment fields must be present in the capability snapshot:

| Field | Type | Description |
|---|---|---|
| `min_uniform_buffer_offset_alignment` | `uint32` | minimum byte alignment for dynamic uniform buffer offsets |
| `min_storage_buffer_offset_alignment` | `uint32` | minimum byte alignment for dynamic storage buffer offsets |
| `min_texture_copy_bytes_per_row_alignment` | `uint32` | minimum row-pitch alignment for texture copy operations |

These fields are consumed by the scene planner when packing multiple parameter blocks into
shared buffers or when scheduling texture uploads.
Individual DRP2 fixtures need not declare them unless the fixture specifically exercises dynamic
buffer offsets or texture-copy alignment.

Weighted blended OIT is derived from this lower-level snapshot. It is available only when the
runtime reports enough color attachments for accumulation and reveal targets, compatible
floating-point render-target formats, color blending support, and the ability to execute the
transparent accumulation and resolve passes. There is no standalone "WBOIT supported" boolean in
the capability snapshot.

The snapshot must be explicit enough that:

1. adaptation can run before planning,
2. planning does not need backend object inspection,
3. diagnostics can explain capability-driven rejection or simplification at the scene level.


## Submission Contract

The important rules are:

1. scene decides the topology of the frame plan,
2. the scene-to-DRP2 converter emits the command stream,
3. runtime executes the command stream,
4. runtime may cache backend objects internally,
5. runtime must not silently reinterpret scene-level ordering, fallback, or identity.

The runtime may internally translate one DRP2 command stream into one or several backend
submissions, with internal caching or deferred object creation. A convenience
`FramePlan` submission helper may combine conversion and command-stream submission, but that helper
does not make `FramePlan` the primary runtime contract. Internal details must not alter the meaning
of the submitted frame, the identity route for diagnostics, or the identity route for completions.


## Readback Contract

Readback and offscreen export services are expressed in producer-visible terms.

At minimum:

1. deterministic offscreen image capture,
2. single-pixel or equivalent picking readback,
3. typed completion routing back to scene-visible request or target identity,
4. stale-result rejection where the scene model requires it (e.g. hover picking).

Completion delivery: v0.4 uses polling — `dvz_scene_poll_pick_result` and the
`DVZ_EVENT_PICK_RESULT` callback cover the primary readback use case. DRP2 does not expose
async signaling primitives; the polling model is the authoritative completion path.


## Diagnostics Contract

Runtime failures are reportable in terms the scene layer can map back to:

1. plan node identity,
2. logical target identity,
3. logical resource identity,
4. capability mismatch or execution failure category.

DRP2 error codes are non-normative from the scene API's perspective. DRP2 failures are mapped to
scene-level error categories in `DvzDiagnosticReport`. The raw DRP2 symbolic code appears only
in a verbose debug string field and is not required semantic input. See `validation/DIAGNOSTICS.md`
for the shared scene-facing diagnostic record shape.


## Canvas And Presentation Ownership

The scene layer does not own or reference the canvas, window, swapchain, or stream/sink objects.

The intended ownership model is:

1. the application creates a canvas (window + device + swapchain + stream + sinks),
2. the application creates a DRP2 runtime, which plugs into the canvas draw callback,
3. the application creates a scene, passing it the DRP2 runtime as its submission target,
4. the scene never references canvas, stream, sink, or swapchain directly.

Per-frame flow:

```text
canvas fires draw callback
  → application asks scene to build frame → FramePlan
  → scene-to-DRP2 converter emits a command stream
  → DRP2 runtime executes the command stream through the active backend
  → canvas submits → stream routes to sinks (swapchain, video, etc.)
```

Video and offscreen sinks are attached to the canvas stream by the application.


## Logical Render Target

The scene needs to know the logical output dimensions and format for a frame — but not the backend
object behind them.

This is expressed as a `DvzRenderTarget`, a scene-level logical handle resolved by the DRP2
runtime to actual backend resources.

Two variants:

```c
// Interactive: target backed by application/canvas presentation resources
DvzRenderTarget* target = dvz_runtime_target_canvas(runtime, app_canvas_token);

// Offscreen/export: target backed by a readback-capable image
DvzRenderTarget* target = dvz_runtime_target_offscreen(runtime, width, height, format);

dvz_figure_set_target(fig, target);
```

The scene holds a `DvzRenderTarget` and uses it when building the `FramePlan`.
It does not know whether the target is a swapchain image or an export buffer.
The canvas is not a scene concept — the scene API never takes a `DvzCanvas*` argument directly.


## Render-Pass Attachments And Texture Views

In the active DRP2 `2.0` surface, render-pass attachments reference textures directly via
`texture_id`, not via texture-view objects. Texture views (`CreateTextureView`) are used only
for bind-group bindings, not for render-pass attachment slots.

Scene-layer code that constructs render passes should therefore reference `DvzRenderTarget`
logical handles and let the runtime resolve them to the correct texture id — it should not
assume texture-view ids are needed on the attachment path.


## Export Helpers

`dvz_figure_export_png` and `dvz_figure_export_svg` live in the scene layer and drive the
runtime via a standard offline frame sequence. They are not part of the runtime surface.


## Service Boundaries

The runtime service must not require the scene layer to expose:

1. Vulkan, Metal, WebGPU, GLFW, or swapchain handles,
2. backend descriptor or pipeline object identifiers,
3. backend synchronization primitives,
4. backend allocator state.

The scene layer must not ask the runtime service to:

1. reinterpret scene semantics,
2. choose scene fallback policy after planning,
3. discover missing validation that should already have failed earlier.


## Relationship To Other Scene Docs

| Document | Relationship |
|---|---|
| `pipeline/FRAME_PLAN.md` | the producer-side artifact this service consumes |
| `validation/VALIDATION.md` | stages that must run before submission |
| `validation/ADAPTATION.md` | capability-driven adaptation before planning |
| `interaction/PICKING.md` | freshness and identity semantics on picking completions |
| `validation/DIAGNOSTICS.md` | shared scene-facing diagnostic record shape |
| `api/IMPLEMENTATION_NOTES.md` | C object mapping, Python binding architecture |
