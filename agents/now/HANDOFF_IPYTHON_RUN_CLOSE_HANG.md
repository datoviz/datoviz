# IPython Hosted Window Close Hang

Status: resolved by `9c1e60912`; retain as the investigation and validation record.

The hosted loop now reaps closed views through `dvz_app_reap_closed_views()` before session
teardown, emits opt-in `DVZ_PYTHON_RUN_DEBUG=1` lifecycle tracing, and has focused C and Python
tests. Keep macOS terminal-IPython close/reopen in physical-machine RC validation, but do not list
the former hang as an active known issue.

This handoff records the current investigation state for the terminal IPython integration added on
the v0.4 development branch. The user can open a live Datoviz window from IPython and regain the
prompt, but closing the native window on macOS leaves the window visible with a spinning/loading
pointer and the window no longer responds.


## Current Behavior

Recent commits added the supported path:

1. `datoviz.run(scene, figure)` defaults to nonblocking in terminal IPython.
2. The returned `RunSession` borrows the scene/figure so the scene remains in memory after the
   hosted app/window is closed.
3. The terminal IPython input hook drives `dvz_window_host_poll()`,
   `dvz_app_should_exit()`, and `dvz_app_render_once()`.
4. `dvz_app_should_exit()` is now a public C helper bound in Python.
5. The input hook now returns when there are no running Datoviz sessions.

The user reports that this still hangs on macOS after closing the native window from IPython.


## Working Hypothesis

The hosted IPython path does not currently mirror the normal interactive C app close path.

`dvz_app_run(app, 0)` does this around native window close:

```text
poll/wait host events
if app should exit:
    reap closed views
    break
device wait
return to caller
```

The Python hosted path currently does this:

```text
poll host events
if app should exit:
    destroy app
    destroy borrowed window host
```

The missing step is the internal `_app_reap_closed_views(app)` cleanup used by `dvz_app_run()`.
That cleanup disconnects figure input routing, waits for the device, destroys GUI/canvas/recorder
runtime resources, destroys the native window, and marks the view noninteractive. Skipping it is a
plausible explanation for a macOS window remaining half-alive during teardown.


## Preferred Investigation Plan

1. Add a narrow public hosted-loop helper in the app API, for example:

   ```c
   DVZ_EXPORT bool dvz_app_reap_closed_views(DvzApp* app);
   ```

   It should call the same internal close cleanup used by `dvz_app_run(app, 0)` and return whether
   any views were reaped. Keep the helper focused on hosted integrations; do not create a parallel
   app loop.

2. Update `datoviz/run.py` so `RunSession.render_once()` handles close as:

   ```text
   poll host
   if app should exit:
       reap closed views
       close session
       return
   render once
   ```

   `RunSession.close()` should remain idempotent. The scene and figure should remain borrowed and
   valid for reopening with a new `dvz.run(scene, figure)` call.

3. Add opt-in Python tracing before and after every hosted close stage:

   ```text
   poll
   should_exit
   reap_closed_views
   app_destroy
   window_host_destroy
   inputhook_return
   ```

   Gate it behind an environment variable such as `DVZ_PYTHON_RUN_DEBUG=1` and write compact lines
   to stderr. This is essential because if the user's next macOS test still hangs, the trace should
   show whether the stall is in polling, reaping closed views, `dvz_app_destroy()`, or
   `dvz_window_host_destroy()`.

4. Add focused tests:

   - Python mock test that close order is `poll -> should_exit -> reap -> app_destroy ->
     window_host_destroy`.
   - Python test that `_datoviz_inputhook()` still returns once all sessions close.
   - Narrow C test that the new public helper is callable and no-ops safely when there are no
     closed views. A full native-window close test may be environment-sensitive and can remain
     manual if CI cannot drive GLFW close reliably.

5. Regenerate and validate bindings because this touches exported API:

   ```sh
   just ctypes
   just ctypes-check
   just ctypes-python-smoke
   just test app
   git diff --check
   ```

6. Ask the user to retest from terminal IPython on macOS with:

   ```sh
   DVZ_PYTHON_RUN_DEBUG=1 ipython
   ```

   If the hang remains, use the last emitted trace line as the next concrete breakpoint.


## Debug Branching

If the trace stops before `should_exit`, the close request is not being observed after
`dvz_window_host_poll()`. Investigate GLFW close event delivery in the input-hook scheduling path.

If the trace stops in `reap_closed_views`, compare `_view_close_runtime_resources()` behavior under
`dvz_app_run(app, 0)` versus the hosted IPython loop, especially device wait and GUI/canvas/window
destruction ordering on macOS.

If the trace stops in `dvz_app_destroy()`, the app destructor is still touching resources that the
native close path normally releases earlier. Audit duplicate teardown and borrowed host ownership.

If the trace reaches `inputhook_return` but the native window remains visible, the input hook is no
longer the blocking point. Focus on GLFW/Cocoa window destruction and whether the final host poll or
main-thread constraints are required on macOS.


## Policy To Preserve

Closing a window should destroy only the hosted app/window runtime resources owned by that
`RunSession`. It should not destroy the borrowed scene or figure. Reopening should create a fresh
app/window around the still-live scene state. This matches the intended v0.4 policy: data and scene
state are durable user objects; presentation runtime state is disposable.
