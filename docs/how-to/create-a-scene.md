# Create a Scene

Set up the minimal scene, figure, panel, visual, and teardown sequence needed to render anything in datoviz.

## Overview

Every datoviz program follows the same hierarchy: a `scene` is the root container; a `figure` holds one or more panels at a given pixel size; a `panel` is a viewport that owns a controller and one or more visuals. You must create and destroy these in order.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 3

    int main(void) {
        /* positions in normalized [-1, 1] coordinates, z=0 for 2D */
        float pos[N * 3] = {
            -0.5f, -0.3f, 0.0f,
             0.0f,  0.5f, 0.0f,
             0.5f, -0.3f, 0.0f,
        };
        uint8_t color[N * 4] = {
            255,  64,  64, 255,
             64, 200,  64, 255,
             64,  64, 255, 255,
        };
        float diameter[N] = {40.0f, 40.0f, 40.0f};

        /* create the scene hierarchy */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* create a point visual and upload data */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "diameter", diameter, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* open a window and run until closed */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Hello datoviz");
        dvz_app_run(app, 0);

        /* destroy in reverse order */
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

`dvz_scene()` allocates the root GPU context. Everything else hangs off this object; it must outlive all figures, panels, and visuals.

`dvz_figure(scene, width, height, 0)` registers a logical render target at the given pixel size. The last argument is a flags field; pass `0` for defaults.

`dvz_panel_full(figure)` creates a panel that occupies the entire figure. A figure can hold multiple panels (see [Multiple panels](create-multiple-panels.md)), but for a single-viewport scene one full panel is sufficient.

`dvz_point(scene, 0)` creates a GPU-accelerated point visual. `dvz_visual_set_data` uploads each named attribute — `"position"` (float32 xyz), `"color"` (uint8 RGBA), `"diameter"` (float32 pixels) — to the GPU. Call `dvz_panel_add_visual` to attach the visual to the panel.

`dvz_app(scene)` initializes the GPU backend. `dvz_view_glfw` opens an interactive GLFW window; `dvz_app_run(app, 0)` enters the event loop and blocks until the window is closed (pass a positive integer to limit frame count).

Teardown must happen in reverse order: destroy the app before the scene. The scene destructor cleans up all figures, panels, and visuals that were created from it.

## Common patterns

**Offscreen PNG instead of a window:**

```c
DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_offscreen(app, figure, 800, 600);
dvz_app_run(app, 1);
dvz_view_capture_png(view, "output.png");
dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

**Add pan-and-zoom navigation:**

```c
DvzController* ctrl = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, ctrl, DVZ_DIM_MASK_XY);
```

Call this before `dvz_app`.

## See also

- [Add a visual](add-a-visual.md) — attach more visual types to a panel
- [Render offscreen](render-offscreen.md) — headless PNG and video output
- [Multiple panels](create-multiple-panels.md) — subdivide a figure into panels
