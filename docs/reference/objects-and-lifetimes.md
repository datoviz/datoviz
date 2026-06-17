# Objects And Lifetimes

This page summarizes public ownership and lifetime rules for the v0.4 scene path. The active
runtime boundary is:

```text
scene frame plans -> DRP2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

Scene objects describe visualization state. Runtime objects execute emitted frame work. Do not
borrow Vulkan, WebGPU, swapchain, command-buffer, or synchronization ownership from a scene object.

## Ownership Summary

| Object | Owner | Lifetime rule |
| --- | --- | --- |
| `DvzScene` | application | Top-level owner for figures, panels, visuals, controllers, scales, sampled fields, and scene-owned resources. Destroy scene-owned children before or with the scene according to the public API used to create them. |
| `DvzFigure` | scene | Layout/output container. A figure may be emitted repeatedly; one emitted frame artifact represents one immutable frame snapshot. |
| `DvzPanel` | figure | Logical viewport, camera/controller binding target, and visual attachment target. Panel pointers are invalid after the owning figure/scene is destroyed. |
| `DvzVisual` | scene | Retained visual family instance. Visuals reference copied data or scene-owned resources; they do not own backend buffers or command buffers. |
| `DvzSampledField` and other scene resources | scene | Authoritative CPU-side resource state. Visuals borrow resource handles; the scene/lifetime API owns destruction. |
| `DvzController` | scene | Scene-side input/navigation state. Panels bind controllers; controllers do not own panels. |
| `DvzSceneFrameArtifact` | caller after emit | Immutable snapshot of one emitted frame, including DRP2 stream snapshot and packet payloads. Destroy it after the runtime or tooling has consumed it. |
| Runtime/canvas/stream/app objects | application/runtime layer | Execute or present frame work. They are outside scene ownership and may outlive individual frame artifacts only if their API says so. |

## Data Writes

`dvz_visual_set_data()` and range-style visual writes copy caller-provided data unless an API
explicitly documents borrowed or external-buffer semantics. After a successful ordinary data write,
the caller may release the source memory.

External buffers, scene compute buffers, and future zero-copy paths require an explicit lifetime and
synchronization contract. Do not infer borrowed pointer behavior from `static`, `dynamic`, or
`streaming` mutability hints; those hints guide planning and allocation, not pointer ownership.

## Frame Artifacts

`DvzSceneFrameArtifact` is the scene emission product. It owns the immutable DRP2 stream snapshot
and frozen upload payload bytes needed to execute or inspect that frame safely after retained scene
state changes.

Rules:

| Rule | Consequence |
| --- | --- |
| Retained scene mutation is legal after successful artifact creation. | Artifacts must not borrow mutable visual payload memory. |
| Raw DRP2 stream access is an artifact-owned snapshot view. | Do not keep stream pointers after destroying the artifact. |
| Browser packet spans are borrowed from artifact-owned memory. | Decode or copy packet spans before releasing the artifact, emitting the next frame on the same scene, or destroying the scene. |
| JSON/debug exports are projections of the artifact snapshot. | JSON is diagnostic/export material, not the browser runtime transport. |

## Callbacks And User Data

Callbacks borrow scene/runtime objects for the duration of the call unless their API documents
otherwise. User-data pointers are application-owned; Datoviz stores and returns the pointer but does
not copy or free the pointee.

Keep callback work short. Expensive updates should be scheduled into normal frame/update paths so
controller state, dirty scopes, validation, frame planning, and runtime execution remain ordered.

## Query And Readback Results

Queries are request/result objects associated with a panel, frame, and scene generation. Results
must be matched to the request they answer before mutating hover, selection, annotations, or
application state. Stale results are valid diagnostics but should not update current interaction
state.

Readback payload ownership depends on the query/runtime API. Treat returned result structs as value
snapshots and treat any borrowed payload spans as valid only for the documented polling/result
lifetime.

## Destroy Order

Recommended high-level order:

1. stop app/event delivery and frame callbacks;
2. finish or discard outstanding query/readback work;
3. release frame artifacts and debug exports;
4. destroy runtime/app/canvas/stream objects according to their API;
5. destroy scene-owned resources or the scene.

For graphics ownership, only destroy, begin, end, reset, submit, or transition handles that the API
contract says you own.

## See Also

- [Callbacks](callbacks.md)
- [Queries](queries.md)
- [Visual attributes](visual-attributes.md)
- [C API reference](c-api/index.md)
