# Quickstart: Rendering in 10 Minutes

**Prerequisites:** Datoviz built from source or installed from a local v0.4 package — see
[Install](install.md).

This page builds one complete visualization: 10,000 random points in an interactive window. You can
drag to pan and scroll to zoom. No data files are needed.


## Full example

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    # Create random 2D positions, one RGBA color per point, and a point size in pixels.
    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.randint(0, 256, (N, 4), dtype=np.uint8)
    color[:, 3] = 255
    diameters = np.full(N, 5.0, dtype=np.float32)

    # Create one figure with one drawing area.
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)

    # Let the user drag to pan and scroll to zoom in the X/Y plane.
    controller = dvz.dvz_panzoom(scene, None)
    dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

    # A point visual draws all points in one GPU batch.
    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "diameter_px", diameters)
    dvz.dvz_panel_add_visual(panel, visual, None)

    # Open a window and keep it open until the user closes it.
    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include <time.h>
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    #define N 10000

    int main(void) {
        srand((unsigned)time(NULL));

        /* Create random 2D positions, one RGBA color per point, and a point size in pixels. */
        float pos[N * 3], diameter_px[N];
        uint8_t color[N * 4];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+2] = 0;
            color[4*i+0] = rand() % 256;
            color[4*i+1] = rand() % 256;
            color[4*i+2] = rand() % 256;
            color[4*i+3] = 255;
            diameter_px[i] = 5.0f;
        }

        /* Create one figure with one drawing area. */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* Let the user drag to pan and scroll to zoom in the X/Y plane. */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        /* A point visual draws all points in one GPU batch. */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "diameter_px", diameter_px, N);
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        /* Open a window and keep it open until the user closes it. */
        dvz_view_window(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```


## Build and run

=== "Python"

    Save as `scatter.py` and run:

    ```sh
    python scatter.py
    ```

=== "C"

    From a source checkout, build and run the canonical quickstart example:

    ```sh
    just example-c start/scatter
    ./build/examples/c/start/scatter --live
    ```

    For a standalone C file outside the repository, use an installed Datoviz package and its
    exported CMake package or `datoviz-config` helper. See [Use from C or C++](../how-to/c-integration.md).


## What you should see

A dark window containing 10,000 colored dots. Drag to pan, scroll to zoom.

![10 000 colored dots in an interactive window with pan-and-zoom](../assets/gallery/v0.4/start/start_scatter.webp)


## How it works

**Data arrays** - The example creates three arrays with one row per point. `position` stores
`x`, `y`, and `z` coordinates. `color` stores red, green, blue, and alpha values. `diameter_px`
stores the point size in screen pixels.

**Scene, figure, panel** - A `scene` is the whole visualization. A `figure` is the image area, here
800 by 600 pixels. A `panel` is the part of the figure where the scatter plot is drawn. This
quickstart uses one full-size panel.

**Controller** - `dvz_panzoom` adds mouse interaction. `dvz_panel_bind_controller` connects it to
the panel and limits the interaction to the X and Y axes.

**Visual** - A visual is a renderable collection, such as points, lines, an image, a mesh, or text
labels. Here, `dvz_point` creates one point visual, and `dvz_visual_set_data` gives it the arrays it
needs: positions, colors, and point sizes.

**Run** - `dvz.run(scene, figure)` opens the window in the Python example. The C example uses the
longer app/view calls directly because C does not have the same quickstart helper.


## Next steps

- Browse the [Examples gallery](../examples/index.md) for visual families, features, and showcase scenes.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add 3D rotation instead of panzoom, see [Use 3D controllers](../how-to/3d-navigation.md).
