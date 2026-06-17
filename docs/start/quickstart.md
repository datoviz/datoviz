# Quickstart: Rendering in 10 Minutes

**Prerequisites:** Datoviz installed — see [Install](install.md).

Render 10,000 random scatter points in an interactive window with pan-and-zoom. No external data files needed.


## Full example

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    # --- data ---
    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.randint(0, 256, (N, 4), dtype=np.uint8)
    color[:, 3] = 255
    sizes = np.full(N, 5.0, dtype=np.float32)

    # --- scene ---
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)
    # enable pan/zoom on XY axes
    controller = dvz.dvz_panzoom(scene, None)
    dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

    # --- visual ---
    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "size", sizes)
    dvz.dvz_panel_add_visual(panel, visual, None)

    # --- run ---
    # replace with dvz.capture(scene, figure, path="output.png") for offscreen PNG
    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    Build and run with the bundled example:

    ```sh
    just example-c visuals/point
    ./build/examples/c/visuals/point --live
    ```

    Or as a self-contained program:

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    int main(void) {
        /* data */
        int N = 10000;
        float pos[N * 3], size[N];
        uint8_t color[N * 4];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+2] = 0;
            color[4*i+0] = rand() % 256;
            color[4*i+1] = rand() % 256;
            color[4*i+2] = rand() % 256;
            color[4*i+3] = 255;
            size[i] = 5.0f;
        }

        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);
        /* enable pan/zoom on XY axes */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        /* visual */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* run — or replace with dvz_capture(scene, figure, "output.png") for offscreen PNG */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```


## What you should see

A dark window containing 10,000 colored dots. Drag to pan, scroll to zoom.

![10 000 colored dots in an interactive window with pan-and-zoom](../assets/gallery/v0.4/start/start_scatter.webp)


## How it works

**Data arrays** — Positions are `(N, 3)` float32 arrays in normalized coordinates `[-1, 1]`. Colors are `(N, 4)` uint8 RGBA in `[0, 255]`. Sizes are per-point pixel diameters.

**Scene hierarchy** — A `scene` is the root container. A `figure` holds one or more panels at a given pixel size. A `panel` is a viewport that owns a controller and one or more visuals.

**Controller** — `dvz_panzoom` attaches pan-and-zoom navigation to the panel. `dvz_panel_bind_controller` connects it, optionally restricting which axes respond.

**Visual** — `dvz_point` is the GPU-accelerated scatter renderer. `dvz_visual_set_data` uploads each named attribute (position, color, size) to the GPU.

**Run vs. capture** — `dvz.run(...)` opens a GLFW window and blocks until the user closes it. Replace it with `dvz.capture(scene, figure, path="output.png")` to render a single frame to disk without opening a window.


## Next steps

- Browse the [Examples gallery](../examples/index.md) for visual families, features, and showcase scenes.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add 3D rotation instead of panzoom, see [Use arcball](../how-to/use-arcball.md).
