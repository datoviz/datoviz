# Thread Safety And Async Data Handoff

This document defines the threading model for the scene layer: which operations are
safe to call from background threads, how computation results are handed off safely
to the scene, and how the scene integrates async uploads into the frame lifecycle.


## Threading Model

The scene layer is **single-threaded on the render path**.

The render thread owns:
- the frame lifecycle (event dispatch, state update, invalidation, planning, DRP2 emission),
- all direct mutations of scene-owned semantic state,
- GPU submission.

Background threads may not call scene mutation functions directly.
The safe interface for background threads is the **transfer queue** described below.


## Transfer Queue

The scene owns a thread-safe transfer queue: `DvzTransferQueue`.

Background threads enqueue `DvzTransfer` commands via:

```text
dvz_scene_submit_transfer(scene, &transfer)
```

This call is safe from any thread at any time.
Internally it uses a lock-free ring buffer so it does not block the render thread.

The render thread drains the queue once per frame during stage 2
(App State Update, see `FRAME_LIFECYCLE.md`) before invalidation resolution.
All transfers drained in a given frame are applied atomically from the scene's
perspective — they either all land in the current frame or are deferred to the next.


## Transfer Types

### Data Upload Transfer

Upload new attribute data for a visual:

```text
DvzTransfer t = {
    .type    = DVZ_TRANSFER_DATA,
    .visual  = visual,
    .attr    = DVZ_ATTR_POSITION,
    .data    = ptr,           // must remain valid until the transfer is consumed
    .size    = n_bytes,
    .offset  = byte_offset,   // 0 for full replace
}
dvz_scene_submit_transfer(scene, &t)
```

The scene marks the affected attribute dirty when it drains this transfer.
The `data` pointer must remain valid until the render thread has consumed the transfer
(i.e., until the `DvzTransfer` is drained from the queue).
Use `dvz_transfer_set_copy(transfer)` to request that the scene copy the data
immediately at submit time, releasing the caller from the lifetime constraint.


### Scalar / Uniform Update Transfer

Update a visual uniform or a scene parameter:

```text
DvzTransfer t = {
    .type   = DVZ_TRANSFER_UNIFORM,
    .visual = visual,
    .slot   = slot_index,
    .data   = ptr,
    .size   = n_bytes,
}
dvz_scene_submit_transfer(scene, &t)
```


### Callback Transfer

Schedule a callback to run on the render thread at the next safe drain point:

```text
DvzTransfer t = {
    .type     = DVZ_TRANSFER_CALLBACK,
    .callback = my_fn,
    .user_data = ptr,
}
dvz_scene_submit_transfer(scene, &t)
```

The callback runs on the render thread during stage 2, before invalidation.
It may call any scene mutation function safely.
This is the general-purpose escape hatch for operations that do not fit the typed
transfer variants above.


## Background Computation Pattern

The intended pattern for background computation → scene update:

```text
// Background thread:
result = heavy_computation(input)   // runs freely, no scene interaction
dvz_scene_submit_transfer(scene, &(DvzTransfer){
    .type   = DVZ_TRANSFER_DATA,
    .visual = visual,
    .attr   = DVZ_ATTR_POSITION,
    .data   = result.positions,
    .size   = result.n * sizeof(vec3),
})
// or use DVZ_TRANSFER_CALLBACK for complex multi-attribute updates
```

The render thread picks up the transfer at the next frame boundary.
The background thread does not need to synchronize further — there are no shared
mutexes on the scene state.


## Data Lifetime Rules

| Transfer mode | Caller's lifetime obligation |
|---|---|
| Default (pointer) | keep `data` valid until drain; check `dvz_transfer_consumed(transfer)` |
| Copy (`dvz_transfer_set_copy`) | free `data` immediately after `dvz_scene_submit_transfer` returns |

`dvz_transfer_consumed` returns true once the render thread has drained and applied
the transfer.
It is safe to poll from any thread.

Transfers that request a copy allocate from a scene-internal staging pool.
The pool is bounded; if it is full, `dvz_scene_submit_transfer` blocks briefly until
space is available.
This back-pressure is intentional — it prevents unbounded memory growth if the
background thread produces faster than the render thread consumes.


## Async Upload (Large Data)

For very large uploads (e.g., 100M-item position arrays) where staging-pool copying
is impractical, the transfer queue supports a zero-copy path:

```text
DvzTransfer t = {
    .type     = DVZ_TRANSFER_DATA,
    .visual   = visual,
    .attr     = DVZ_ATTR_POSITION,
    .data     = ptr,
    .size     = n_bytes,
    .flags    = DVZ_TRANSFER_FLAG_ZERO_COPY,  // pointer mode, no staging copy
}
```

In zero-copy mode the caller must keep `data` valid until `dvz_transfer_consumed`
returns true.
The scene uploads directly from the caller's pointer at drain time.


## Frame Synchronization

If the application needs to know when a submitted transfer has been rendered
(i.e., is visible on screen), it can register a completion callback:

```text
dvz_transfer_on_rendered(transfer, my_rendered_callback, user_data)
```

The callback fires after the frame containing the transfer has been submitted to the GPU.
It runs on the render thread.

This is useful for double-buffering workflows where the background thread should
not overwrite a source buffer until the GPU has finished reading it.


## What Background Threads Must Not Do

Background threads must not:

1. call `dvz_panel_add_visual`, `dvz_visual_destroy`, or any structural scene mutation,
2. call `dvz_visual_set_data` directly (use `dvz_scene_submit_transfer` instead),
3. access or mutate scene-owned semantic state (cameras, selection, scales, controllers),
4. call DRP2 or GPU functions directly.

Structural mutations (add/remove visuals, create panels, change scale parameters) must
happen on the render thread.
The safe path from a background thread is a `DVZ_TRANSFER_CALLBACK` transfer — the
callback runs on the render thread during stage 2 and may perform any structural
mutation except direct GPU or DRP2 calls.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `FRAME_LIFECYCLE.md` | transfer queue is drained in stage 2 (App State Update) |
| `INVALIDATION_AND_CACHING.md` | drained transfers mark affected attributes dirty |
| `RESOURCE_MODEL.md` | staging pool and zero-copy upload path |
| `FRAME_PLAN_IR.md` | dirty attributes resolved into UploadNodes in the same frame |


## Deferred Questions

1. exact staging pool size and back-pressure policy,
2. `DVZ_TRANSFER_CALLBACK` may perform structural mutations (add/remove visuals, create
   panels, change scale parameters) — resolved: yes, allowed, since the callback runs on
   the render thread during stage 2 before invalidation,
3. multi-scene resource sharing across threads (deferred; out of scope for v0.4).
