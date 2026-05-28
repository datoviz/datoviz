# Async Callback Follow-Up

This note captures the next validation and polish steps after the first async callback slice landed.
The C post/wake primitive, raw `ctypes` callback generation, and thin Python async helpers exist; the
next work should prove the user-facing shape in real interactive Python contexts.


## Goals

1. Validate the new helper surface in plain Python, IPython terminals, and notebooks.
2. Keep examples small and raw-binding-oriented rather than rebuilding a plotting API.
3. Tighten naming and lifecycle behavior before broader users depend on it.


## Next Steps

1. Keep the small raw async example under `examples/python/raw/async_click.py` aligned with the
   host-helper shape. The intended style is:

   ```python
   import datoviz.raw as dvz
   from datoviz.host import Host

   host = Host(app)
   view = host.view(raw_view)

   @view.pointer("click")
   async def click(ev):
       result = await host.run_thread(load_or_compute, ev.x, ev.y)
       # mutate scene/view state on the Python owner loop
       view.request_frame()

   host.run()
   ```

2. Run the example in a plain Python script.
   Use `Host.run()` so the script path proves that Datoviz can be driven by
   `dvz_app_render_once()` without entering `dvz_app_run(app, 0)`.

3. Run the same workflow in an IPython terminal.
   Confirm that an already-running event loop is handled by scheduling a task rather than blocking
   the console.

4. Run a notebook/Jupyter smoke.
   Confirm that the event loop remains responsive, async handlers run, and frame requests schedule
   render ticks without requiring a native blocking loop.

5. Replace the initial top-level helper names with the explicit host namespace.
   The target public shape is `datoviz.host.Host`, `host.view(raw_view)`,
   `view.pointer(...)`, `view.keyboard(...)`, `view.resize(...)`, and `view.scale(...)`. Avoid
   top-level `dvz.View`, generic `EventSource`, and blanket top-level raw re-exports.

6. Add a process-worker example only after there is a meaningful workflow.
   Good candidates are pick/probe post-processing, large file/data loading, or CPU-heavy conversion.
   Do not add a toy process example that obscures the Datoviz integration point.


## Validation

1. Keep `testing/test_python_async_helpers.py` as the synthetic fast loop.
2. Keep `tools/bindings/ctypes_render_smoke.py` covering `dvz_view_post()`.
3. Add one manual or automated notebook-oriented smoke once the execution environment is stable.
4. Re-run:

   ```text
   PYTHONPATH=. pytest -q testing/test_python_async_helpers.py testing/test_ctypes_raw_smoke.py
   direnv exec . python tools/bindings/ctypes_render_smoke.py
   ```


## 2026-05-28 Running-Loop Smoke

A quick hosted-loop smoke passed in a plain Python process with an already-running `asyncio` loop.
The smoke created a tiny offscreen point scene, wrapped the raw `DvzView*` with `Host.view()`, then
called `host.run()` from inside `asyncio.run()`. `Host.run()` returned an `asyncio.Task` instead of
blocking, `view.request_frame()` woke the hosted render loop, and `host.stop()` shut it down cleanly.

Command shape:

```text
PYTHONPATH=. python - <<'PY'
import asyncio
...
task = host.run()
assert isinstance(task, asyncio.Task)
view.request_frame()
host.stop()
await task
PY
```

Result:

```text
host running-loop visual smoke: OK
```

This proves the core IPython/notebook requirement that the helper can schedule work when a Python
event loop is already active. A real IPython or Jupyter smoke is still useful later for environment
integration, but the blocking-loop risk is covered by this focused check.
