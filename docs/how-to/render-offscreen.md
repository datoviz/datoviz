# Render Offscreen

Render a scene to a PNG file without opening a window.

## Overview

Offscreen rendering is useful for headless servers, automated testing, and batch image generation.
Instead of `dvz_view_glfw`, create a `dvz_view_offscreen` view, render one frame, and capture it
to a PNG with `dvz_view_capture_png`.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    #define N 4
    #define WIDTH  800u
    #define HEIGHT 600u

    int main(void) {
        /* positions and colors for four points */
        float pos[N * 3] = {
            -0.5f, -0.3f, 0.0f,
            -0.2f,  0.4f, 0.0f,
             0.2f, -0.2f, 0.0f,
             0.5f,  0.3f, 0.0f,
        };
        uint8_t color[N * 4] = {
            255,  80,  80, 255,
             80, 200,  80, 255,
             80, 120, 255, 255,
            240, 200,  60, 255,
        };
        float diameter[N] = {40.0f, 55.0f, 45.0f, 60.0f};

        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* visual */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "diameter", diameter, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* offscreen rendering */
        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
        dvz_view_render_once(view);
        dvz_view_capture_png(view, "output.png");

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Build the scene normally.** Create `dvz_scene()`, `dvz_figure()`, `dvz_panel_full()`, add
visuals, and upload data exactly as you would for an interactive window. The offscreen path
diverges only at the view creation step.

**Create an offscreen view.** After `dvz_app(scene)`, call `dvz_view_offscreen(app, figure, width,
height)` instead of `dvz_view_glfw`. This allocates a GPU framebuffer at the requested pixel
dimensions without opening any window.

**Render one frame.** `dvz_view_render_once(view)` submits a single render pass and waits for the
GPU to finish. It returns `DVZ_CANVAS_FRAME_READY` on success.

**Write the PNG.** `dvz_view_capture_png(view, "output.png")` reads the framebuffer back to the
CPU and writes a PNG at the path you specify.

**Destroy resources.** Call `dvz_app_destroy` then `dvz_scene_destroy` as usual.

## Common patterns

**Check the framebuffer dimensions.** When pixel-exact output matters (e.g. test comparisons),
verify the framebuffer matches your request:

```c
uint32_t fw = 0, fh = 0;
dvz_view_framebuffer_size(view, &fw, &fh);
/* fw and fh should equal WIDTH and HEIGHT */
```

**Loop variant.** You can also use `dvz_app_run(app, 1)` to run exactly one event-loop iteration,
then call `dvz_view_capture_png`. This is equivalent for a single frame but fits naturally into
code that also supports the interactive path:

```c
DvzApp* app = dvz_app(scene);
/* interactive: dvz_view_glfw(app, figure, WIDTH, HEIGHT, "title"); dvz_app_run(app, 0); */
/* offscreen: */
DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
dvz_app_run(app, 1);
dvz_view_capture_png(view, "output.png");
```

## See also

- [Create a scene](create-a-scene.md) — scene and figure setup
- [Add a visual](add-a-visual.md) — uploading data to a visual
- [Profile performance](profile-performance.md) — measuring frame time in headless mode
