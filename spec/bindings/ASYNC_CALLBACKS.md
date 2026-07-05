# Async Callback and Event Loop Plan

This note records the planned v0.4 Python callback, worker, and event-loop integration for the raw
`ctypes` binding and the first thin convenience layer above it. Datoviz remains C-first; the Python
surface should make native events easy to consume without recreating the v0.3 plotting API.


## Goals

1. Expose callback-bearing C APIs through generated `ctypes` bindings.
2. Keep Python callbacks non-blocking by default.
3. Let users write one async event handler instead of callback/done-callback pairs.
4. Support IPython terminals, Python consoles, notebooks, and plain scripts.
5. Make thread and process workers easy while keeping Datoviz mutation on the owner thread.


## Non-Goals

1. Do not make the raw binding a high-level plotting API.
2. Do not run long user code inside C-dispatched callbacks.
3. Do not make scene, view, canvas, or Vulkan mutation generally thread-safe.
4. Do not require notebooks or Python consoles to enter `dvz_app_run(app, 0)`.


## User-Facing Shape

The convenience layer should allow this pattern:

```python
@view.pointer("click")
async def click(ev):
    result = await host.run_process(expensive_compute, ev.x, ev.y)
    visual.set_data(result)
```

The callback copies the C event, schedules the coroutine, returns immediately, then resumes on the
Python/event owner thread after awaited work completes. Scene/view mutation happens after the
`await`, not in a worker thread or process.

Thread and process helpers should be explicit:

```python
result = await host.run_thread(load_large_array, path)
result = await host.run_process(segment_volume, volume_id)
```

`run_thread()` is appropriate for I/O or native work that releases the GIL. `run_process()` is the
default for pure-Python CPU-bound work or stronger isolation.


## C Runtime Support

Python can own queues, `asyncio`, thread pools, and process pools, but it cannot reliably wake a
Datoviz-owned native wait loop or safely marshal worker results back to the Datoviz owner thread
without a C primitive.

Add a minimal public app/view scheduling API:

```c
typedef void (*DvzViewPostCallback)(DvzView* view, void* user_data);

int dvz_view_post(DvzView* view, DvzViewPostCallback callback, void* user_data);
DvzResult dvz_view_wake(DvzView* view);
```

The post queue is thread-safe. Posted callbacks run on the Datoviz view/app owner thread, near the
start of `dvz_view_render_once()` and within `dvz_app_run()` wake cycles. `dvz_view_wake()` wakes a
blocked native window wait and marks the view as needing scheduler attention without requiring
Python to know the backend.


## Raw `ctypes` Support

The generator should emit callback typedefs as `ctypes.CFUNCTYPE` definitions instead of treating
function-pointer parameters as `ctypes.c_void_p`. This unblocks APIs such as:

1. `dvz_input_subscribe_pointer()`
2. `dvz_input_subscribe_event()`
3. `dvz_view_set_frame_callback()`
4. `dvz_view_set_request_frame_callback()`
5. `dvz_view_post()`

The generated/raw Python layer must keep callback objects alive until the matching token-based
unsubscribe, clear, or destroy path runs. A small registry keyed by returned subscription id is
sufficient for the first slice.


## Python Event Loop Integration

Python-hosted integration should drive Datoviz with render-once primitives owned by an explicit
host adapter:

```python
from datoviz.host import Host

host = Host(app)
host.view(raw_view)

await host.run_async()
host.run()
```

`Host.run()` adapts to the current Python context:

1. If no `asyncio` loop is running, call `asyncio.run(host.run_async())`.
2. If a loop is already running, as in IPython or notebooks, schedule a task and return it.

The adapter registers `dvz_view_set_request_frame_callback()` so Datoviz invalidation becomes an
`asyncio.Event`. On-demand mode waits for invalidation. Continuous mode uses an async timer and still
honors explicit frame requests.

The blocking C loop remains available as the raw path:

```python
dvz.dvz_app_run(app, 0)
```

It should not be the recommended path for notebooks, consoles, or hosted Python event loops.


## Validation Plan

1. Test raw callback typedef generation and function unskipping.
2. Test callback lifetime and unsubscribe with synthetic `DvzInputRouter` events.
3. Test `dvz_view_post()` ordering and wake behavior with app/view focused tests.
4. Test async handler dispatch from copied synthetic input events.
5. Test thread and process helpers returning results to the owner loop.
6. Add an IPython-style smoke that drives `dvz_app_render_once()` without entering
   `dvz_app_run(app, 0)`.
