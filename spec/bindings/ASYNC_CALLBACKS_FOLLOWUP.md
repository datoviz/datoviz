# Async Callback Follow-Up

This note captures the next validation and polish steps after the first async callback slice landed.
The C post/wake primitive, raw `ctypes` callback generation, and thin Python async helpers exist; the
next work should prove the user-facing shape in real interactive Python contexts.


## Goals

1. Validate the new helper surface in plain Python, IPython terminals, and notebooks.
2. Keep examples small and raw-binding-oriented rather than rebuilding a plotting API.
3. Tighten naming and lifecycle behavior before broader users depend on it.


## Next Steps

1. Add a small example under `examples/python/raw/async_click.py`.
   The example should show the intended single-function style:

   ```python
   @view.on("click")
   async def click(ev):
       result = await dvz.run_thread(load_or_compute, ev.x, ev.y)
       # mutate scene/view state on the Python owner loop
       view.request_frame()
   ```

2. Run the example in a plain Python script.
   Use `dvz.run(app, views=[raw_view])` or the current wrapper equivalent so the script path proves
   that Datoviz can be driven by `dvz_app_render_once()` without entering `dvz_app_run(app, 0)`.

3. Run the same workflow in an IPython terminal.
   Confirm that an already-running event loop is handled by scheduling a task rather than blocking
   the console.

4. Run a notebook/Jupyter smoke.
   Confirm that the event loop remains responsive, async handlers run, and frame requests schedule
   render ticks without requiring a native blocking loop.

5. Decide whether the first wrapper name is acceptable.
   The current minimal wrapper is `dvz.View(raw_view)`. If this feels too implicit or conflicts with
   future high-level APIs, add a clearer helper such as `dvz.wrap_view(raw_view)` while keeping the
   raw pointer available.

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
