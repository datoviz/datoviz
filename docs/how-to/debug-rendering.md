# Debug Rendering

Diagnose rendering problems by recording frames as `.dvzr` files, replaying them in isolation, and exporting scene state as JSON.

## Overview

Datoviz provides two complementary diagnostics: frame recording captures the GPU command stream to a `.dvzr` file that can be replayed without rebuilding the scene; JSON export serializes the retained scene graph to a human-readable document for inspecting visual state.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    #define N 5
    #define WIDTH  640u
    #define HEIGHT 480u

    int main(void) {
        /* build a minimal scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
        DvzPanel* panel = dvz_panel_full(figure);
        DvzVisual* point = dvz_point(scene, 0);

        vec3 pos[N] = {
            {-0.6f, 0.0f, 0.0f}, {-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
            {+0.3f, 0.0f, 0.0f}, {+0.6f, 0.0f, 0.0f},
        };
        float size[N] = {20.0f, 20.0f, 20.0f, 20.0f, 20.0f};
        dvz_visual_set_data(point, "position", pos, N);
        dvz_visual_set_data(point, "diameter", size, N);
        dvz_panel_add_visual(panel, point, NULL);

        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);

        /* record one frame to a .dvzr file */
        dvz_view_record_start(view, "debug.dvzr");
        dvz_view_render_once(view);
        dvz_view_record_stop(view);
        dvz_view_capture_png(view, "debug_original.png");

        /* replay the recording into a second view */
        DvzFigure* replay_fig = dvz_figure(scene, WIDTH, HEIGHT, 0);
        DvzView* replay_view = dvz_view_offscreen(app, replay_fig, WIDTH, HEIGHT);
        dvz_view_replay_start(replay_view, "debug.dvzr");
        dvz_view_replay_set_paced(replay_view, false);
        dvz_view_render_once(replay_view);
        dvz_view_replay_stop(replay_view);
        dvz_view_capture_png(replay_view, "debug_replay.png");

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create a scene and render it offscreen as usual. Call `dvz_view_record_start()` with a path to a `.dvzr` output file before rendering, then `dvz_view_record_stop()` after — the GPU command stream for that frame is saved to disk.

To replay, create a fresh figure and offscreen view, then call `dvz_view_replay_start()` with the `.dvzr` path. Setting `dvz_view_replay_set_paced(view, false)` runs all recorded frames as fast as possible rather than at the original frame rate. Call `dvz_view_render_once()` to execute the first frame, then `dvz_view_replay_stop()`.

Capture both the original and replay views with `dvz_view_capture_png()` and diff them to confirm the recording round-trips correctly.

## Common patterns / Variants

**Export scene state as JSON.** Use `dvz_scene_json()` to get a heap-allocated JSON string describing the retained scene graph — figures, panels, visuals, and their attribute data. Free it with `dvz_scene_json_destroy()`.

```c
char* json = dvz_scene_json(scene);
/* inspect or write json to a file */
dvz_scene_json_destroy(json);
```

**Validate with git diff.** Capture a PNG before and after a change, then compare pixel-by-pixel with `diff` or `compare` (ImageMagick) to catch regressions in an automated test.

```sh
compare -metric AE debug_original.png debug_replay.png diff.png
```

**Check for trailing whitespace.** Before committing documentation changes:

```sh
git diff --check
```

## See also

- [Render offscreen](render-offscreen.md) — offscreen view setup
- [Profile performance](profile-performance.md) — frame timing
