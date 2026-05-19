# App Frame Scheduling Refactor

> **Execution Status**
> - **Status:** `IMMEDIATE NEXT TASK`
> - **Updated on:** `2026-05-19`
> - **Purpose:** replace the unconditional interactive app loop with explicit scheduling policy,
>   while preserving run-as-fast-as-possible benchmark paths.

This is the active plan for reducing unnecessary CPU/GPU work in interactive `dvz_app_run()` loops
without weakening immediate-present benchmarks, replay, animation, or hosted-toolkit integration.


## Problem

`dvz_app_run(app, 0)` currently behaves like a continuous game loop:

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

1. `dvz_app_window_render_once()` remains scheduler-free. Hosted integrations such as Qt, SDL, Tk,
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

Likely configuration surface:

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

Suggested environment overrides:

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

Track scheduling state in `DvzAppWindow`, not in `DvzCanvas`:

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
2. pointer, wheel, and keyboard events routed through the app-window,
3. `dvz_app_window_request_frame()`,
4. replay start/advance/loop,
5. pick/probe/readback requests that need a render or request pass,
6. app-window enable/disable or surface updates,
7. scene mutations once a reliable scene dirty signal exists.

Do not rely only on scene-level dirty state. Some frame demand is app-window-specific: swapchain
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
            dvz_app_window_render_once(win);

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

1. Do not cap `dvz_app_window_render_once()`.
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

Status: `Next`

Add the common monotonic timestamp helper and small tests that verify monotonic non-decreasing
behavior and plausible elapsed intervals.


### 2. Backend Wait Hooks

Status: `Next`

Add `wait` and `wait_timeout` to the window backend table. Implement GLFW with native waits and add
safe fallbacks for non-GLFW backends.


### 3. App Config And Environment

Status: `Next`

Add scheduling mode and FPS cap to `DvzAppConfig`, defaults, and environment overrides. Keep current
behavior available through explicit continuous mode.


### 4. App-Window Invalidation

Status: `Next`

Add per-window `dirty` and `frame_requested` state. Route existing app-window resize, input, request
frame, replay, and external-surface paths through invalidation.


### 5. Event-Aware `dvz_app_run()`

Status: `Next`

Replace the unconditional interactive render loop with on-demand waiting, continuous work detection,
and optional FPS pacing.


### 6. Scene Dirty Integration

Status: `Parallel`

Add or reuse scene mutation/animation signals once the app-window scheduler is in place. Keep this
slice separate if broad scene mutation hooks would make the first scheduler patch too large.


### 7. Benchmarks And Smoke Coverage

Status: `Parallel`

Add focused validation for:

1. static interactive app idles without continuous renders,
2. input/resize wakes the loop and renders,
3. animation/replay continues rendering,
4. immediate + continuous remains uncapped,
5. immediate + FPS cap paces frames.


## Sub-Agent Work Split

This task can use parallel agents because the write scopes are naturally separable:

1. **Timing/backend worker:** `src/common/_time_utils.h`, `src/window/*`,
   `include/datoviz/window.h`, window tests.
2. **App scheduler worker:** `include/datoviz/app.h`, `src/app/app.c`, app tests.
3. **Scene/request explorer:** read-only audit of scene mutation, animation, replay, pick/probe,
   and request paths that should invalidate app windows.
4. **Validation worker:** focused tests and manual smoke commands after the first implementation
   slices land.

Avoid parallel edits to `src/app/app.c` unless the ownership boundaries are assigned very
explicitly.


## Validation

Minimum validation for documentation-only updates:

1. `git diff --check`

Minimum validation for scheduler code changes:

1. `git diff --check`
2. `just build`
3. focused `just test app` or the closest available app/window/canvas filters
4. GLFW smoke test for interactive waiting and wakeup when the environment supports it

Manual validation targets:

1. static scene with default scheduler idles without visible CPU burn,
2. mouse drag/wheel/resize wakes rendering,
3. active scene animation keeps rendering,
4. replay keeps rendering and honors pacing,
5. `DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous` preserves benchmark behavior,
6. `DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous DVZ_FPS_CAP=60` reduces CPU usage.


## Open Decisions

Recommended defaults are listed here so implementation can proceed without blocking, unless the
project owner chooses otherwise.

1. Default interactive scheduler: recommend `on_demand`.
2. Continuous benchmark opt-in: recommend `DVZ_APP_SCHEDULE=continuous`.
3. FPS cap default: recommend `0` unlimited, with no implicit cap.
4. Environment variable name: recommend `DVZ_FPS_CAP`.
5. Public API surface: recommend adding schedule mode and FPS cap to `DvzAppConfig` first, and only
   adding runtime setters if examples or hosted integrations need them.
