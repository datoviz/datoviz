# Profile Performance

Measure frame timing, cap throughput, and identify rendering bottlenecks in a datoviz application.

## Overview

Datoviz renders on demand or continuously depending on the app scheduler mode. Profiling relies on
wall-clock measurements around `dvz_app_run()` and per-frame callbacks. The key primitives are
`dvz_time_monotonic_ns()` for high-resolution timing, `dvz_view_set_frame_callback()` for
per-frame hooks, and `DvzAppConfig.fps_cap` to throttle throughput during benchmarks.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdio.h>
    #include "datoviz/app.h"
    #include "datoviz/common/functions.h"
    #include "datoviz/scene.h"

    #define N       10000
    #define FRAMES  300

    typedef struct { uint64_t frame_count; uint64_t t0_ns; } PerfState;

    static void on_frame(DvzView* view, void* user_data)
    {
        PerfState* s = (PerfState*)user_data;
        s->frame_count++;
    }

    int main(void)
    {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* minimal visual */
        float pos[N * 3] = {0};
        float size[N];
        uint8_t color[N * 4];
        for (int i = 0; i < N; i++) {
            size[i] = 4.0f;
            color[4*i+3] = 255;
        }
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "size",     size, N);
        dvz_visual_set_data(visual, "color",    color, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* offscreen view for headless benchmarking */
        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_offscreen(app, figure);

        /* register per-frame callback */
        PerfState state = {0, 0};
        dvz_view_set_frame_callback(view, on_frame, &state);

        /* timed run */
        state.t0_ns = dvz_time_monotonic_ns();
        dvz_app_run(app, FRAMES);
        uint64_t elapsed_ns = dvz_time_monotonic_ns() - state.t0_ns;

        double fps = (double)state.frame_count / (elapsed_ns * 1e-9);
        printf("rendered %llu frames in %.1f ms — %.1f FPS\n",
               (unsigned long long)state.frame_count,
               elapsed_ns / 1e6,
               fps);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create a scene and a minimal visual as usual. For headless benchmarking, open an offscreen view
with `dvz_view_offscreen()` so no window appears and no vsync limits throughput.

Register a frame callback with `dvz_view_set_frame_callback()`. The callback runs after each
successfully submitted frame, making it the right place to count frames or accumulate per-frame
timing data.

Snapshot the monotonic clock with `dvz_time_monotonic_ns()` before and after `dvz_app_run()`.
Pass a finite `frame_count` to `dvz_app_run()` so the loop terminates automatically after the
target number of frames.

Divide the frame count by the elapsed wall-clock time to get average FPS. For finer granularity,
record a timestamp inside the callback on each frame and compute per-frame intervals.

## Common patterns / Variants

**Cap frame rate to reduce CPU load.**  Set `fps_cap` in `DvzAppConfig` before creating the app:

```c
DvzAppConfig cfg = dvz_app_config();
cfg.fps_cap = 60.0;
DvzApp* app = dvz_app_ex(scene, &cfg);
```

**Set a fixed animation rate.**  For animations that should run at a known cadence regardless of
render speed, call `dvz_scene_set_fps()` before `dvz_app_run()`:

```c
dvz_scene_set_fps(scene, 60.0);
```

**Per-frame interval logging.**  In the frame callback, snapshot the clock each time and print
the delta to identify individual slow frames:

```c
static void on_frame(DvzView* view, void* user_data)
{
    uint64_t* last_ns = (uint64_t*)user_data;
    uint64_t now = dvz_time_monotonic_ns();
    printf("frame dt = %.2f ms\n", (*last_ns ? (now - *last_ns) / 1e6 : 0.0));
    *last_ns = now;
}
```

## See also

- [Animation](animation.md) — timer-driven data updates each frame
- [Video export](video-export.md) — recording frames to a video file
- [Render offscreen](render-offscreen.md) — headless rendering without a window
