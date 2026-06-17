# Video Export

Record an animated scene to an MP4 file by attaching a capture sink before running the app loop.

## Overview

Video export works by configuring a capture before calling `dvz_app_run()`. The app renders frames
offscreen and writes each one to the encoder. Call `dvz_view_capture_stop()` after the run to
flush and close the file.

## Example

=== "C"

    ```c
    #include <math.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N       1
    #define FPS     60.0
    #define FRAMES  120u

    int main(void) {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* visual — single animated point */
        float pos[3] = {0.0f, 0.0f, 0.0f};
        uint8_t color[4] = {100, 200, 255, 255};
        float size = 20.0f;
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", &size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* app — offscreen view */
        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_offscreen(app, figure, 800, 600);

        /* configure video capture */
        DvzAppCaptureConfig cap = dvz_app_capture_config();
        cap.fps = FPS;
        cap.basename = "output";
        cap.directory = ".";
        dvz_view_capture_start(view, &cap);

        /* render FRAMES frames */
        dvz_app_run(app, FRAMES);

        /* flush and close the video file */
        dvz_view_capture_stop(view);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create the scene, figure, panel, and visual as usual. The visual data can be updated each frame
via a timer callback — see [Animation](animation.md) for the frame-callback pattern.

Create an offscreen view with `dvz_view_offscreen()`. Video export does not require a visible
window; offscreen rendering is the normal path.

Configure capture with `dvz_app_capture_config()` and set `fps`, `basename`, and `directory`.
Pass the config to `dvz_view_capture_start()` before calling `dvz_app_run()`. The encoder writes
frames as the app loop renders them.

Pass a non-zero `frame_count` to `dvz_app_run()` so the loop terminates automatically after that
many frames. Call `dvz_view_capture_stop()` immediately after to flush and close the output file.

## Common patterns / Variants

**Derive settings from environment variables** — useful for scripted pipelines:

```c
/* DVZ_CAPTURE=mp4 DVZ_CAPTURE_BASENAME=output ./my_app */
dvz_view_capture_from_env(view, "output");
dvz_app_run(app, FRAMES);
dvz_view_capture_stop(view);
```

**Animate data each frame** — register a timer callback before starting capture:

```c
/* called every frame; update visual data here */
static void on_frame(DvzApp* app, DvzId view_id, DvzTimerEvent* ev, void* user) {
    DvzVisual* visual = (DvzVisual*)user;
    float t = (float)ev->time;
    float pos[3] = {0.6f * sinf(t), 0.6f * cosf(t), 0.0f};
    dvz_visual_set_data(visual, "position", pos, 1);
}
dvz_app_timer(app, view_id, 0.0, 1.0 / FPS, on_frame, visual);
```

## See also

- [Animation](animation.md) — frame callbacks and timer-driven data updates
- [Render offscreen](render-offscreen.md) — single-frame PNG capture
- [Debug rendering](debug-rendering.md) — `.dvzr` recording and replay
