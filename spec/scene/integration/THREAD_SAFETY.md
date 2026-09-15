# Thread Safety And Async Data Handoff

Status: normative for the implemented v0.4 owner-thread handoff.

## Ownership

The scene and render path are single-threaded. The view owner thread owns event dispatch, retained scene mutation, invalidation, frame planning, DRP2 emission, GPU submission, and destruction. Background threads must not access live scene, visual, panel, GUI, DRP2, or GPU objects.

## Implemented Handoff

`dvz_view_post(view, callback, user_data)` may be called from another thread. It copies the callback and `user_data` pointer into a mutex-protected queue, calls `dvz_view_wake()`, and runs the callback on the view owner thread near the start of the next `dvz_view_render_once()`.

The queue is bounded by `DVZ_APP_VIEW_POST_CAPACITY`. Posting returns an error when the queue is full; it does not block and does not provide back-pressure. The callback pointer and `user_data` remain borrowed, so the caller must keep them valid until the callback runs. There is no completion handle for knowing when the callback's mutations have reached the GPU.

`dvz_view_wake()` is also safe from another thread. It requests scheduler attention but does not run callbacks or render a frame. Hosted applications remain responsible for servicing the request and calling `dvz_view_render_once()` on the owner thread.

## Producer Pattern

1. On the owner thread, copy the immutable input required by the work.
2. Perform file I/O, decompression, or scientific computation on a worker without accessing Datoviz-owned state.
3. Store the owned result in application memory whose lifetime extends through the posted callback.
4. Post a small apply callback with `dvz_view_post()`.
5. In that owner-thread callback, call ordinary retained scene APIs and release the application-owned result.

Applications that can produce results faster than the owner thread consumes them must implement their own cancellation, latest-wins coalescing, queue limits, and result lifetime management before posting. A high-frequency producer should normally post at most one apply callback for the newest complete result.

## Unsupported Claims

v0.4 does not expose `DvzTransferQueue`, `DvzTransfer`, `dvz_scene_submit_transfer()`, zero-copy background uploads, transfer completion handles, or scene-managed producer back-pressure. Those names are not an implemented API and must not appear in public usage guidance.

## Related Documents

- [`ASYNC_CALLBACKS.md`](../proposals/active/ASYNC_CALLBACKS.md) records broader prepare/work/apply design work; implemented behavior is limited to the owner-thread post primitive above.
- [`REACTIVE_APPLICATION_STATE.md`](../../architecture/REACTIVE_APPLICATION_STATE.md) defines transaction ownership for reactive application state.
- [`FRAME_LIFECYCLE.md`](../pipeline/FRAME_LIFECYCLE.md) defines the owner-thread frame stages.
