# App Frame Scheduling Refactor

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** completed record for the app-loop scheduling and scene-wakeup refactor.

This record captures the completed work that reduced unnecessary CPU/GPU work in interactive
`dvz_app_run()` loops without weakening immediate-present benchmarks, replay, animation, or
hosted-toolkit integration.


## Completion Summary

The refactor is closed for the v0.4 scheduler slice:

1. `dvz_view_render_once()` remains scheduler-free for hosted integrations.
2. `dvz_app_run()` owns Datoviz scheduling policy for built-in interactive apps.
3. The default app scheduler is on-demand; explicit continuous mode keeps benchmark behavior.
4. `DVZ_APP_SCHEDULE=continuous` and `DVZ_FPS_CAP=<positive-number>` control continuous/capped
   built-in scheduling independently from `DVZ_PRESENT_MODE`.
5. Window backends expose poll/wait/wait-timeout/request-frame hooks, with GLFW using native event
   wait/wakeup behavior.
6. Views carry instance-scoped dirty/frame-request/deadline state.
7. Scene mutations, request paths, animations, replay paths, and hosted-surface paths wake the
   owning views when a new frame is needed.
8. Figure resize notifications are emitted only on real size changes, so app-side per-frame size
   synchronization does not keep on-demand windows permanently dirty.

Implementation commits:

1. `70f2dee4` closed the remaining scene-mutation wakeups and regression tests.


## Original Problem

Before this refactor, `dvz_app_run(app, 0)` behaved like a continuous game loop:

```c
while (open)
{
    dvz_window_host_poll(app->window_host);
    render_all_windows();
}
```

That is useful for stress tests and immediate-present benchmarks, but it conflates three different
responsibilities:

1. frame production: scene -> DRP2 -> runtime -> canvas submit,
2. frame demand: input, resize, animation, replay, request processing, data mutation,
3. scheduling policy: idle wait, continuous render, capped continuous render, or benchmark mode.

With `VK_PRESENT_MODE_IMMEDIATE_KHR`, presentation does not naturally block on vblank. A static
interactive scene can therefore spin at 100% CPU even when nothing visible changes.


## Target Contract

Keep rendering primitives separate from scheduling:

1. `dvz_view_render_once()` remains scheduler-free. Hosted integrations such as Qt, SDL, Tk,
   and notebook adapters own their own event loop and call this when they decide to render.
2. `dvz_canvas_frame()` and `dvz_canvas_submit()` remain low-level primitives. They should not gain
   hidden sleeps, caps, or GUI scheduling policy.
3. `dvz_app_run()` becomes the Datoviz-owned scheduler for apps that use the built-in event loop.
4. Present mode and scheduler mode are independent. `VK_PRESENT_MODE_IMMEDIATE_KHR` means
   "presentation does not wait for vblank"; it does not imply that static scenes must render
   forever.


## Scheduling Modes

Introduce explicit app scheduling policy:

```c
typedef enum DvzAppScheduleMode
{
    DVZ_APP_SCHEDULE_ON_DEMAND,
    DVZ_APP_SCHEDULE_CONTINUOUS,
} DvzAppScheduleMode;
```

Recommended defaults:

1. `DVZ_APP_SCHEDULE_ON_DEMAND` for interactive `dvz_app_run(app, 0)`.
2. Continuous rendering only when requested by configuration, active animations, replay playback,
   live streaming, or benchmark tools.
3. `fps_cap = 0` means unlimited. A positive cap paces active continuous rendering.

Implemented configuration surface:

```c
struct DvzAppConfig
{
    uint32_t instance_extension_count;
    const char* const* instance_extensions;
    bool enable_canvas_extensions;
    bool enable_glfw_extensions;
    DvzAppScheduleMode schedule_mode;
    double fps_cap;
};
```

Implemented environment overrides:

1. `DVZ_APP_SCHEDULE=on_demand|continuous`
2. `DVZ_FPS_CAP=<positive-number>`

Keep `DVZ_PRESENT_MODE=fifo|mailbox|immediate` as the swapchain present-mode override only.


## Timing Foundation

Add a shared monotonic timestamp helper before implementing pacing:

```c
uint64_t dvz_time_monotonic_ns(void);
```

Preferred implementation:

1. POSIX: `clock_gettime(CLOCK_MONOTONIC, ...)`
2. macOS: `mach_absolute_time()`
3. Windows: `QueryPerformanceCounter()`

Use this for frame pacing and wait deadlines. Do not use wall-clock `gettimeofday()` for scheduler
deadlines because wall-clock corrections can create bad sleeps or deadline jumps.


## Window Backend Contract

Extend the window backend API so the built-in app loop can block when idle:

```c
poll()
wait()
wait_timeout(double seconds)
request_frame()
```

GLFW mapping:

1. `poll` -> `glfwPollEvents()`
2. `wait` -> `glfwWaitEvents()`
3. `wait_timeout` -> `glfwWaitEventsTimeout(timeout)`
4. `request_frame` -> `glfwPostEmptyEvent()`

Initial fallback behavior for simple/headless/backends without native waiting can be conservative:

1. `wait` may fall back to a small sleep plus poll,
2. `wait_timeout` may sleep up to the timeout plus poll,
3. `request_frame` may be a no-op when no blocking wait exists.


## Per-Window State

Track scheduling state in `DvzView`, not in `DvzCanvas`:

```c
bool frame_requested;
bool dirty;
uint64_t next_frame_ns;
```

Semantics:

1. `dirty` means the last submitted frame no longer represents the current app/window/scene state.
2. `frame_requested` means the event loop should wake and attempt a frame.
3. active animations, replay, streaming, or explicit continuous mode can require frames even when
   the dirty bit is clear.
4. clear `dirty` and `frame_requested` only after a successful submitted frame.

Set invalidation from:

1. resize events,
2. pointer, wheel, and keyboard events routed through the view,
3. `dvz_view_request_frame()`,
4. replay start/advance/loop,
5. pick/probe/readback requests that need a render or request pass,
6. view enable/disable or surface updates,
7. scene mutations once a reliable scene dirty signal exists.

Do not rely only on scene-level dirty state. Some frame demand is view-specific: swapchain
recreation, external surface availability, GUI state, replay target attachment, capture/readback,
and request callbacks.


## Event Loop Shape

Interactive `dvz_app_run(app, 0)` should become:

```c
while (any_interactive_window_open)
{
    bool continuous = app_has_continuous_work(app);
    bool pending = app_has_requested_or_dirty_windows(app);

    if (continuous)
        wait_or_poll_until_next_frame_deadline(app);
    else if (!pending)
        dvz_window_host_wait(app->window_host);
    else
        dvz_window_host_poll(app->window_host);

    for each window:
        if (should_render_window(win))
            dvz_view_render_once(win);

    pace_active_continuous_work_if_needed(app);
}
```

`should_render_window(win)` should return true when:

1. the window is render-enabled,
2. the surface is available or needs one render step to observe release/recreate state,
3. the window is dirty or frame-requested,
4. the app has active animations,
5. replay is active,
6. the configured schedule mode is continuous.


## FPS Cap Policy

The FPS cap is only a scheduler policy for active work:

1. Do not cap `dvz_view_render_once()`.
2. Do not cap offscreen render-once/capture paths.
3. Do not cap low-level canvas functions.
4. Apply the cap in `dvz_app_run()` when continuous work is active.
5. Preserve explicit continuous unlimited mode for immediate-present performance testing.

Pacing should avoid accumulated deadline debt:

```c
next_frame_ns += period_ns;
if (next_frame_ns > now_ns)
    sleep_until(next_frame_ns);
else
    next_frame_ns = now_ns;
```


## Implementation Slices

### 1. Monotonic Time

Status: `Done`

Added the common monotonic timestamp helper used by scheduler deadlines. Focused time tests cover
monotonic behavior and the app scheduler's offline timer paths.


### 2. Backend Wait Hooks

Status: `Done`

Added `wait`, `wait_timeout`, and request-frame wakeup hooks to the window backend table. GLFW uses
native event waiting and wakeups, with conservative fallbacks for non-GLFW/headless backends.


### 3. App Config And Environment

Status: `Done`

Added scheduling mode and FPS cap to `DvzAppConfig`, defaults, and environment overrides. Previous
continuous behavior remains available through explicit continuous mode.


### 4. App-Window Invalidation

Status: `Done`

Added per-window `dirty` and `frame_requested` state. App-window resize, input, request-frame,
replay, render-enable, GUI, and external-surface paths route through invalidation.


### 5. Event-Aware `dvz_app_run()`

Status: `Done`

Replaced the unconditional interactive render loop with on-demand waiting, continuous work
detection, and optional FPS pacing.


### 6. Scene Dirty Integration

Status: `Done`

Scene request-frame integration now covers visual data/style/visibility mutations, scene buffers,
fields, scales, panel layout/technique/controller state, axes, annotations, pick/probe requests,
and active animations.


### 7. Benchmarks And Smoke Coverage

Status: `Done`

Validation covered:

1. static interactive app idles without continuous renders,
2. input/resize wakes the loop and renders,
3. animation/replay continues rendering,
4. immediate + continuous remains uncapped,
5. immediate + FPS cap paces frames.


## Sub-Agent Work Split

This task used subagents because the write scopes were naturally separable:

1. **Timing/backend worker:** `src/common/_time_utils.h`, `src/window/*`,
   `include/datoviz/window.h`, window tests.
2. **App scheduler worker:** `include/datoviz/app.h`, `src/app/app.c`, app tests.
3. **Scene/request explorer:** read-only audit of scene mutation, animation, replay, pick/probe,
   and request paths that should invalidate views.
4. **Validation worker:** focused tests and manual smoke commands after the first implementation
   slices land.

Avoid parallel edits to `src/app/app.c` unless the ownership boundaries are assigned very
explicitly.


## Validation

Recorded validation for closure on `2026-05-19`:

1. `git diff --check` passed.
2. `just build` passed.
3. `just test app` passed: `99/99`.
4. `just test window` passed: `15/15`.
5. `just test time` passed: `113/113`.
6. `just test scene` passed: `335/335`.
7. Bounded GLFW scheduler lab launches completed without startup or validation errors:
   - `timeout 5s env DVZ_FPS=1 direnv exec . ./build/examples/c/techniques/scheduler_lab`
   - `timeout 5s env DVZ_APP_SCHEDULE=continuous DVZ_FPS=1 direnv exec . ./build/examples/c/techniques/scheduler_lab`
   - `timeout 5s env DVZ_APP_SCHEDULE=continuous DVZ_PRESENT_MODE=immediate DVZ_FPS=1 direnv exec . ./build/examples/c/techniques/scheduler_lab`
   - `timeout 5s env DVZ_APP_SCHEDULE=continuous DVZ_FPS_CAP=60 DVZ_FPS=1 direnv exec . ./build/examples/c/techniques/scheduler_lab`
8. Hosted GLFW smoke passed:
   `direnv exec . ./build/examples/c/tools/hosted_glfw_smoke 120`
   rendered `120` frames and observed `126` frame requests.

The bounded GLFW launches prove startup and event-loop viability in this environment. They do not
replace human mouse/resize observation for visual ergonomics, but they are sufficient to keep this
implementation record closed.


## Closed Decisions

1. Default interactive scheduler: `on_demand`.
2. Continuous benchmark opt-in: `DVZ_APP_SCHEDULE=continuous`.
3. FPS cap default: `0` unlimited, with no implicit cap.
4. Environment variable name: `DVZ_FPS_CAP`.
5. Public API surface: schedule mode and FPS cap live in `DvzAppConfig`; runtime setters can be
   added later only if examples or hosted integrations need them.
