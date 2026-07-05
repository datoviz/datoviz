# Quickstart: Rendering in 10 Minutes

**Prerequisites:** Datoviz installed with `pip install datoviz`, installed from a v0.4 pre-release
package, or built from source — see [Install](install.md).

This page builds one complete visualization: 10,000 random points in an interactive window. You can
drag to pan and scroll to zoom. No data files are needed.

Read the example in five blocks: create the data arrays, create the scene layout, add interaction,
upload the arrays to one point visual, then open the window.


## Full example

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    # Each point is described by three arrays with the same length.
    # - pos: one x/y/z position per point. z is 0, so this is a 2D scatter plot.
    # - color: one red/green/blue/alpha color per point, stored as 8-bit RGBA values.
    # - diameters: one point size per point, measured in screen pixels.
    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.randint(0, 256, (N, 4), dtype=np.uint8)
    color[:, 3] = 255
    diameters = np.full(N, 5.0, dtype=np.float32)

    # Create the scene structure: one scene, one figure, and one full-size panel.
    # The scene contains the visualization, the figure has a pixel size, and the
    # panel is the drawing area where the scatter plot will appear.
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)

    # Add mouse interaction to the panel. Pan/zoom is limited to X and Y because
    # the points are flat, with z = 0.
    controller = dvz.dvz_panzoom(scene, None)
    dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

    # Create one point visual for the whole dataset. The three calls to
    # dvz_visual_set_data() attach the arrays to named visual attributes.
    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "diameter_px", diameters)

    # Uploading arrays is not enough by itself: the visual must be added to a
    # panel before it becomes part of the figure.
    dvz.dvz_panel_add_visual(panel, visual, None)

    # Open a window, render the figure, and keep the app running until the user
    # closes the window.
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

        /* Each point is described by three arrays with the same length.
         * pos stores x/y/z positions. z is 0, so this is a 2D scatter plot.
         * color stores one 8-bit RGBA color per point.
         * diameter_px stores one point size per point, measured in screen pixels. */
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

        /* Create the scene structure: one scene, one figure, and one full-size panel.
         * The panel is the drawing area where the scatter plot will appear. */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* Add mouse interaction to the panel. Pan/zoom is limited to X and Y because
         * the points are flat, with z = 0. */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        /* Create one point visual for the whole dataset. Each data call attaches
         * one C array to a named visual attribute. */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "diameter_px", diameter_px, N);

        /* Uploading arrays is not enough by itself: the visual must be added to a
         * panel before it becomes part of the figure. */
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        /* Open a window, render the figure, and keep the app running until the user
         * closes the window. */
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

**Data arrays** - The example creates three arrays with the same length. `position` stores `x`,
`y`, and `z` coordinates for each point. `color` stores red, green, blue, and alpha values.
`diameter_px` stores the point size in screen pixels. In Python, these are NumPy arrays. In C, they
are ordinary C arrays.

**Scene, figure, panel** - A `scene` is the whole visualization. A `figure` is the image area, here
800 by 600 pixels. A `panel` is the part of the figure where the scatter plot is drawn. This
quickstart uses one full-size panel.

**Controller** - `dvz_panzoom` adds mouse interaction. `dvz_panel_bind_controller` connects it to
the panel and limits the interaction to the X and Y axes.

**Visual** - A visual is a renderable collection, such as points, lines, an image, a mesh, or text
labels. Here, `dvz_point` creates one point visual for all 10,000 points. Each
`dvz_visual_set_data` call fills one named attribute of that visual: `"position"`, `"color"`, or
`"diameter_px"`.

**Panel attachment** - Data upload prepares the visual, but it does not place it in the figure.
`dvz_panel_add_visual` attaches the visual to the panel so it will be drawn.

**Run** - `dvz.run(scene, figure)` opens the window in the Python example. The C example uses the
longer app/view calls directly because C does not have the same quickstart helper.


## Next steps

- Browse the [Examples gallery](../examples/index.md) for visual families, features, and showcase scenes.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add 3D rotation instead of panzoom, see [Use 3D controllers](../how-to/3d-navigation.md).
