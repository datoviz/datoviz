# Raw Python Helper Layer

This note documents the small Python helper layer that sits above the generated raw `ctypes`
surface. It is intentionally narrow: helpers make callbacks and hosted event loops easier to use,
but they do not form a plotting API and do not hide the C object model.


## Scope

The helper layer currently lives in:

```text
datoviz/events.py
datoviz/loop.py
```

The public import remains:

```python
import datoviz as dvz
```

Raw C symbols keep their exact names, for example `dvz.dvz_scene()` and
`dvz.dvz_view_render_once(view)`.


## EventSource and View

`EventSource` subscribes Python handlers to a raw `DvzInputRouter*`.

```python
source = dvz.EventSource(router)

@source.on("move")
async def move(ev):
    ...
```

`View` is a thin wrapper around a raw `DvzView*`. It obtains the view input router with
`dvz_view_input()`, exposes the same event registration behavior as `EventSource`, and keeps the
raw handle available as `view.handle`.

Handlers receive copied `PointerEvent` dataclasses, not borrowed C event pointers. This avoids
retaining callback-stack memory after Datoviz returns to C.


## Async Dispatch

Async handlers require a running or explicitly bound `asyncio` loop:

```python
wrapped = dvz.View(raw_view)
wrapped.bind_loop()
```

When no loop is available, awaitable handlers are closed and skipped with a `RuntimeWarning`. The
callback does not call `asyncio.run()` from inside the C-dispatched callback path, because that could
block native event delivery.


## AppLoop

`AppLoop` drives an existing raw `DvzApp*` from Python:

```python
loop = dvz.AppLoop(app, views=[raw_view], fps=30.0)
await loop.run_async()
```

`dvz.run(app, views=[...])` adapts to the current Python context:

1. without a running loop, it uses `asyncio.run()`;
2. with a running loop, it schedules and returns a task.

This path is meant for hosted Python contexts such as IPython and notebooks where entering the
blocking C loop with `dvz_app_run(app, 0)` is undesirable.


## Callback Lifetime

Generated raw callbacks are kept alive according to `spec/bindings/ctypes.yml`:

1. `subscription` callbacks are keyed by callback type, owner pointer, callback identity, and
   user-data value;
2. `unsubscribe` callbacks remove the matching subscription callback;
3. `setter` callbacks are keyed by function name, callback type, and owner pointer;
4. `global` callbacks are keyed by function name and callback type.

Bound methods use a stable identity based on the instance and underlying function, so
`source.subscribe(obj.method)` can later be matched by `source.unsubscribe(obj.method)` even though
Python creates a fresh bound-method object on each attribute access.


## Validation

Focused validation for this layer:

```text
just ctypes-check
just ctypes-python-smoke
PYTHONPATH=. python tools/bindings/ctypes_smoke.py
PYTHONPATH=. python tools/bindings/ctypes_render_smoke.py
```

`ctypes-check` validates generated ABI and policy without depending on high-level smoke behavior.
`ctypes-python-smoke` covers the Python helper layer and missing-generated-binding import behavior.
