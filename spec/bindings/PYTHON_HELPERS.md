# Python Host Helper Layer

This note documents the intended Python helper layer that sits beside the generated `ctypes`
binding. It is intentionally narrow: helpers make callbacks and hosted event loops easier to use,
but they do not form a plotting API and they do not replace the C object model.


## Decision

The exact C-shaped call form should live under `datoviz.raw` and should be imported explicitly when
that shape is needed:

```python
import datoviz.raw as dvz
```

The `datoviz` top-level package remains the normal binding import with `dvz_*` names and
policy-declared NumPy adaptation. Raw examples and low-level tests should use `datoviz.raw`
whenever they need exact pointers, counts, bytes, or callbacks.

Python hosting helpers should live under a clearly separate namespace, currently proposed as
`datoviz.host`. The helper layer adapts existing raw handles to Python callback and event-loop
semantics. It must not create scene objects, hide ownership, or grow into a high-level plotting API.

For coding agents, this boundary should be explicit: host helpers are allowed to make event loops,
callbacks, and frame waking easier, but they are not an invitation to invent Python scene, visual,
or plotting objects in Datoviz.


## Namespace Shape

The intended namespace split is:

```text
datoviz
  Top-level direct-engine Python binding. Preserves dvz_* names while accepting NumPy arrays
  for policy-declared data arguments.

datoviz.raw
  Exact generated ctypes call form. Exact dvz_*, Dvz*, and DVZ_* names.

datoviz.host
  Thin Python-hosted integration helpers for callbacks, asyncio, and frame waking.
```

This keeps the exact call form available while still allowing ergonomic Python integration:

```python
import datoviz.raw as dvz
from datoviz.host import Host

host = Host(app)
view = host.view(raw_view)

@view.pointer("click")
async def click(ev):
    view.request_frame()

host.run()
host.close()
```

The context-manager form is optional:

```python
with Host(app) as host:
    view = host.view(raw_view)
    host.run()
```

Explicit `close()` must remain supported so scripts, tests, and notebooks can manage lifetimes
without forcing a `with` block.


## Host

`Host` owns Python-side hosting state for a raw `DvzApp*`:

1. callback keepalive objects needed by `ctypes`;
2. copied event dispatch state;
3. asyncio tasks and wakeups;
4. registered view adapters;
5. cleanup for subscriptions installed through the helper layer.

`Host` does not own the C app. The raw Datoviz API still creates and destroys the app.


## View Adapter

`host.view(raw_view)` returns a Python adapter for an existing raw `DvzView*`. The adapter's
`handle` attribute is the original raw handle:

```python
view = host.view(raw_view)
assert view.handle == raw_view
```

The adapter does not own or replace the C view. It only owns Python-side subscriptions and async
dispatch state installed through the helper layer.

The adapter may expose small host-oriented conveniences such as:

```python
view.request_frame()
view.render_once()
```

These are acceptable because frame waking and render-once scheduling are part of the hosted-loop
problem. The raw handle remains available when exact C calls are preferred:

```python
dvz.dvz_view_request_frame(view.handle)
dvz.dvz_view_render_once(view.handle)
```


## Input Adapters

The helper layer should expose known Datoviz input callback families explicitly instead of a broad
generic event framework. Prefer family-specific registration methods:

```python
@view.pointer("click")
async def click(ev):
    ...

@view.keyboard("press")
def key(ev):
    ...

@view.resize()
def resized(ev):
    ...

@view.scale()
def scaled(ev):
    ...
```

`view.pointer("click")` subscribes through `dvz_input_subscribe_pointer()` on the input router
returned by `dvz_view_input(view.handle)`. It copies the C `DvzPointerEvent` into a Python value
before dispatching user code. Keyboard, resize, scale, and future input-family helpers should follow
the same pattern: copy callback-stack data before returning to C, dispatch sync handlers directly,
and schedule async handlers on the hosted Python loop.

Avoid public names such as `EventSource`, `PointerEvents`, or top-level `View` as the primary API.
They either sound too generic or too much like the beginning of a Python scene model. If small
internal classes are useful, keep them implementation details behind `Host` and view adapters.


## Async Dispatch

Async handlers require a running or explicitly bound `asyncio` loop owned by the host:

```python
host = Host(app)
view = host.view(raw_view)
```

When no loop is available, awaitable handlers are closed and skipped with a `RuntimeWarning`. The
callback does not call `asyncio.run()` from inside the C-dispatched callback path, because that could
block native event delivery.


## Running

`Host.run()` drives an existing raw `DvzApp*` from Python by repeatedly using the raw render-once
primitive:

```python
host = Host(app, fps=30.0)
host.view(raw_view)
host.run()
```

An async form should also be available:

```python
await host.run_async()
```

`Host.run()` adapts to the current Python context:

1. without a running loop, it uses `asyncio.run()`;
2. with a running loop, it schedules and returns a task.

This path is meant for hosted Python contexts such as IPython and notebooks where entering the
blocking C loop with `dvz_app_run(app, 0)` is undesirable.

The host knows registered views because `host.view(raw_view)` registers them. Avoid requiring users
to repeatedly pass `views=[...]` once they are using a host object.


## Raw Callback APIs

Users of `datoviz.raw` must still be able to use callback-bearing C APIs directly:

```python
import datoviz.raw as dvz

def callback(router, event_ptr, user_data):
    event = event_ptr.contents
    print(event.pos[0], event.pos[1])

callback_id = dvz.dvz_input_subscribe_pointer(router, callback, None)
dvz.dvz_input_unsubscribe(router, callback_id)
```

The generated exact-call layer should:

1. emit `ctypes.CFUNCTYPE` definitions for callback typedefs;
2. set function `argtypes` to those callback typedefs;
3. coerce Python callables into the expected callback type;
4. keep callback objects alive according to `spec/bindings/ctypes.yml`;
5. release keepalive entries on matching unsubscribe, setter clear, or global clear paths when the
   policy describes that relationship.

The host layer is optional ergonomics above those exact calls. It copies event structs, handles
async dispatch, and centralizes cleanup, but it should not be required for direct `datoviz.raw` use.


## Callback Lifetime

Generated raw callbacks are kept alive according to `spec/bindings/ctypes.yml`:

1. `subscription` callbacks are keyed by callback type, owner pointer, callback identity, and
   user-data value;
2. `unsubscribe` callbacks remove the matching subscription callback;
3. `setter` callbacks are keyed by function name, callback type, and owner pointer;
4. `global` callbacks are keyed by function name and callback type.

Bound methods use a stable identity based on the instance and underlying function, so a helper can
later unsubscribe `obj.method` even though Python creates a fresh bound-method object on each
attribute access.


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


## Non-Goals

The host layer must not provide:

1. Python scene, visual, figure, panel, or plotting objects;
2. prefixless aliases for raw C symbols;
3. additional array-aware data upload APIs;
4. high-level scientific visualization workflows;
5. compatibility with the v0.3 Python object model.

Array-aware data adaptation belongs in the top-level binding call form described in
[ARRAY_FACADE.md](ARRAY_FACADE.md). High-level scientific workflows belong above Datoviz, currently
in GSP/VisPy2 or application-specific code.

Documentation and examples should mark this clearly so generated Python code does not assume
`datoviz.figure()`, `datoviz.scatter()`, or similar convenience APIs exist unless a future supported
helper layer explicitly adds them.
