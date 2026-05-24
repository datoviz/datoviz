> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-18`
> - **Purpose:** define the intended event-callback dispatch model for long-running interaction
>   work without implying general retained-scene thread safety.

# Async Callbacks And Main-Thread Dispatch

This proposal defines the target model for long-running user callbacks that are triggered by
Datoviz input or scene events. The goal is to let C, Python, GLFW-hosted apps, and Qt-hosted apps
start slow work from interactions such as clicks without blocking UI event processing or rendering.

The proposal is intentionally about **dispatch semantics**, not a promise that every scene object is
thread-safe.


## Motivation

Interactive applications often need to react to an event with slow work:

1. a click starts a database query or filesystem read,
2. a hover starts a delayed probe computation,
3. a selection change launches a scientific calculation,
4. a Python callback calls `time.sleep(5)` or runs blocking user code,
5. a Qt tool panel dispatches work through a `QThreadPool` and later updates the figure.

The default synchronous callback model is still useful for cheap work, but it is not enough for
these cases. A blocking callback should not freeze pointer handling, window repaint, frame
execution, or hosted UI integration.


## Core Rule

Datoviz should expose safe async callback ergonomics without describing the retained scene as
freely thread-safe.

The render/app thread owns:

1. direct scene mutations,
2. direct scene reads that inspect live retained objects,
3. frame planning and DRP2 emission,
4. GPU/runtime calls,
5. hosted-window rendering and frame scheduling hooks.

Background workers may perform slow computation, I/O, sleeping, and pure CPU processing. They must
not dereference live Datoviz scene objects, retained visual buffers, runtime objects, DRP2 streams,
or GPU handles.


## Three-Phase Async Callback Model

The preferred async callback shape has up to three phases:

```text
prepare(event, scene) -> snapshot   # main/render thread, optional
work(snapshot)        -> result     # worker/background thread
apply(result, scene)  -> void       # main/render thread
```

`prepare` is optional. If no explicit prepare callback is provided, bindings may pass an owned copy
of the event object to `work`.

`work` receives only owned data: copied event fields, copied arrays, scalar parameters, handles that
are explicitly safe to use off-thread, or user-owned objects that do not require Datoviz scene
access.

`apply` runs at a safe point on the Datoviz app/render thread. It may mutate scene state through
normal scene/app APIs and request another frame.


## Scene Access From Background Work

If the worker needs scene data for its computation, the data must be captured as an immutable
snapshot before the worker starts.

Bad pattern:

```python
def work(event):
    # Not allowed: this reads a live scene object from the worker.
    positions = points.positions
    time.sleep(5)
    return compute(positions, event.pos)
```

Preferred pattern:

```python
def prepare(event):
    # Runs on the Datoviz/UI thread.
    return {
        "event_pos": event.pos,
        "positions": points.positions.copy(),
    }


def work(snapshot):
    # Runs on a worker thread.
    time.sleep(5)
    return compute(snapshot["positions"], snapshot["event_pos"])


def apply(result):
    # Runs on the Datoviz/UI thread.
    points.colors[:] = result.colors
```

This makes the ownership and lifetime boundary explicit: `work` consumes copied data, and `apply`
updates live Datoviz state.


## Main-Thread Dispatch Primitive

The foundational C primitive should be a thread-safe way to enqueue app/render-thread work:

```c
typedef void (*DvzViewTaskCallback)(DvzView* win, void* user_data);
typedef void (*DvzViewTaskDestroyCallback)(void* user_data);

int dvz_view_post(
    DvzView* win,
    DvzViewTaskCallback callback,
    void* user_data,
    DvzViewTaskDestroyCallback destroy);
```

Target semantics:

1. may be called from any thread,
2. never runs `callback` immediately on the calling worker thread,
3. enqueues `callback` into a per-window or app-owned thread-safe dispatch queue,
4. wakes or requests the hosted window when possible,
5. drains queued callbacks on the app/render thread at a documented safe point,
6. allows the callback to read or mutate scene state through normal APIs,
7. invokes `destroy` for unrun tasks during shutdown or after callback completion according to the
   final ownership contract.

This primitive is the smallest useful API because C examples, Python bindings, and Qt adapters can
all build higher-level async callback conveniences on top of it.


## Optional Worker Convenience Primitive

After main-thread dispatch exists, Datoviz may provide an optional convenience helper:

```c
typedef void* (*DvzAsyncWorkCallback)(void* input, void* user_data);
typedef void (*DvzAsyncDoneCallback)(DvzView* win, void* result, void* user_data);
typedef void (*DvzAsyncDestroyCallback)(void* ptr);

int dvz_view_submit(
    DvzView* win,
    DvzAsyncWorkCallback work,
    void* input,
    DvzAsyncDoneCallback done,
    void* user_data,
    DvzAsyncDestroyCallback destroy_input,
    DvzAsyncDestroyCallback destroy_result);
```

Target semantics:

1. `work` runs on a Datoviz-managed worker or user-provided executor,
2. `done` is posted back to the app/render thread,
3. `input` and `result` ownership is explicit,
4. cancellation and app/window destruction either run destructors or skip callbacks safely,
5. scene access remains forbidden in `work` and allowed in `done`.

This helper is not required for the first implementation if examples can use `dvz_view_post()`
plus user-managed worker threads.


## C User Experience

The beginner C model should be:

```text
input callback
  -> copy event and any required scene snapshot
  -> enqueue worker job
  -> return immediately

worker thread
  -> do slow work
  -> call dvz_view_post(win, apply, result, destroy)

app/render thread
  -> run apply(result)
  -> mutate scene
  -> request or render next frame
```

The input callback itself stays cheap. The worker does not call scene, DRP2, Vulkan, GLFW, or Qt APIs
unless those APIs are explicitly documented as worker-safe.


## Python User Experience

Python bindings should expose both a beginner-friendly blocking-function mode and an asyncio mode.

For blocking functions, use `async_=True` because `async` is a Python keyword:

```python
@window.on_click(async_=True)
def on_click(event):
    time.sleep(5)
    return {"color": "red"}


@on_click.done
def on_click_done(result):
    points.color = result["color"]
```

Semantics:

1. the event passed to `on_click` is a Python-owned copy,
2. `on_click` runs in a thread pool or executor,
3. `on_click_done` runs on the Datoviz/UI thread,
4. scene mutation is allowed in `done`, not in the worker function.

For callbacks that need scene data, provide an explicit prepare hook:

```python
@window.on_click(async_=True)
def on_click(snapshot):
    time.sleep(5)
    return expensive_compute(snapshot["positions"], snapshot["event_pos"])


@on_click.prepare
def on_click_prepare(event):
    return {
        "event_pos": event.pos,
        "positions": points.positions.copy(),
    }


@on_click.done
def on_click_done(result):
    points.colors[:] = result.colors
```

Python bindings may also expose lower-level primitives:

```python
window.post(callable, *args, **kwargs)       # callable runs on Datoviz/UI thread
future = window.submit(func, *args, **kwargs)
future.then(done)                           # done runs on Datoviz/UI thread
```


## Python `async def` Semantics

Bindings may additionally support coroutine callbacks:

```python
@window.on_click
async def on_click(event):
    await asyncio.sleep(5)
    points.color = "red"
```

This has different semantics from `async_=True`:

1. `async_=True` means a regular blocking function runs in an executor,
2. `async def` means a coroutine is scheduled on an asyncio loop,
3. `await asyncio.sleep(5)` is non-blocking when the loop is integrated with the UI/app loop,
4. `time.sleep(5)` inside `async def` still blocks the event loop thread,
5. scene mutation is allowed only when the coroutine resumes on the Datoviz/UI thread.

For blocking work inside an `async def` callback, users should write:

```python
@window.on_click
async def on_click(event):
    result = await asyncio.to_thread(blocking_compute, event.pos)
    points.update(result)
```

The first Python implementation may choose to support `async_=True` before full asyncio app-loop
integration because it directly solves the `time.sleep(5)` use case.


## GLFW Integration

When Datoviz owns the loop through a GLFW view, posted tasks should be drained inside the
Datoviz app/render loop at a documented safe point. Posting a task should request another frame or
otherwise wake the loop when the backend supports it.

A result posted from a worker should become visible without requiring another input event.


## Qt And Hosted UI Integration

When Qt or another host owns the event loop, Datoviz should not create a parallel UI loop. Hosted
adapters should continue to:

1. forward native input to Datoviz,
2. render through hosted render-once APIs,
3. map Datoviz frame requests to native repaint scheduling,
4. use host-native worker systems such as `QThreadPool` or queued signals when appropriate.

`dvz_view_post()` should integrate with hosted scheduling by triggering the registered request
frame callback or equivalent host hook. In Qt, the adapter should map that request to
`QWindow::requestUpdate()` or `QWidget::update()`.


## GPU Readback And Picking Inputs

If the computation needs data that only exists on the GPU, the main thread should first schedule or
process the relevant pick, probe, or readback request. The worker should receive the resolved
CPU-side result as a snapshot, not a live GPU object.

Recommended flow:

```text
click
  -> main thread schedules pick/probe/readback
  -> frame processing resolves CPU-side result
  -> prepare builds snapshot
  -> worker computes
  -> apply mutates scene on main thread
```


## Non-Goals

This proposal does not require:

1. making all scene objects internally locked,
2. allowing worker threads to mutate retained scene state directly,
3. allowing worker threads to emit DRP2 commands,
4. allowing Python ctypes callbacks to retain borrowed C event pointers,
5. replacing Qt, asyncio, or application-provided executor systems.


## Open Questions

1. Should the first C implementation expose only `dvz_view_post()` or also
   `dvz_view_submit()`?
2. Is the dispatch queue owned by `DvzApp`, `DvzView`, or a lower-level hosted-window object?
3. What exact lifecycle point should drain posted callbacks for interactive, offscreen, and hosted
   windows?
4. How should cancellation behave if a window is destroyed while worker jobs are still running?
5. Should Python `async_=True` default to one shared thread pool, a per-window executor, or a
   user-provided executor?
6. How should full asyncio integration coexist with GLFW-owned and Qt-owned loops?


## Promotion Targets

If accepted, this proposal should be promoted into:

1. `../../interaction/EVENT_CALLBACKS.md` for callback semantics,
2. `../../integration/THREAD_SAFETY.md` for worker access rules and main-thread dispatch,
3. `../../integration/EXTERNAL_UI.md` and `../../integration/HOSTED_BACKENDS.md` for Qt/hosted loop
   behavior,
4. future Python binding documentation for `async_=True`, `async def`, `prepare`, `done`, `post`, and
   `submit`.
