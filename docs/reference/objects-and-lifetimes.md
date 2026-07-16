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
| `DvzScene` | application | Top-level owner for figures, panels, visuals, controllers, scales, sampled fields, and scene-owned resources. The application may destroy a child early only through its matching public destroy function; scene destruction releases children that remain. |
| `DvzFigure` | scene | Layout/output container. A figure may be emitted repeatedly; one emitted frame artifact represents one immutable frame snapshot. |
| `DvzPanel` | figure | Logical viewport, camera/controller binding target, and visual attachment target. Panel pointers are invalid after the owning figure/scene is destroyed. |
| `DvzVisual` | scene | Retained visual family instance. Visuals reference copied data or scene-owned resources; they do not own backend buffers or command buffers. |
| `DvzSampledField` and other scene resources | scene | Authoritative CPU-side resource state. Visuals borrow resource handles; the scene/lifetime API owns destruction. |
| `DvzController` | scene | Scene-side input/navigation state. Panels bind controllers; controllers do not own panels. |
| `DvzSceneFrameArtifact` | caller after emit | Immutable snapshot of one emitted frame, including DRP2 stream snapshot and packet payloads. Destroy it after the runtime or tooling has consumed it. |
| `DvzApp` | application | Borrows the scene and owns its views plus the runtime resources it creates. Destroy the app before the borrowed scene. |
| `DvzView` | app | Presents or captures one borrowed figure. There is no independent view destroy call; the pointer becomes invalid when its app is destroyed. |
| Lower runtime/canvas/stream objects | application/runtime layer | Advanced execution objects outside scene ownership. Follow each API's explicit owner/borrower contract. |

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

At the normal app layer this reduces to: stop callbacks/capture, call `dvz_app_destroy(app)`, then
call `dvz_scene_destroy(scene)`. `DvzFigure`, `DvzPanel`, and `DvzView` pointers must not be used
after their respective scene/app owner is destroyed.

For graphics ownership, only destroy, begin, end, reset, submit, or transition handles that the API
contract says you own.

## See Also

- [Callbacks](callbacks.md)
- [Queries](queries.md)
- [Visual attributes](visual-attributes.md)
- [C API reference](c-api/index.md)
