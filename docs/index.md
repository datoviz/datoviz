# Datoviz

Datoviz is a GPU-powered visualization engine for scientific data. It helps you build fast,
interactive 2D and 3D views when ordinary plotting tools become too slow or too limited.

Use Datoviz when you want to explore many points, images, meshes, volumes, annotations, or custom
scientific scenes, either on the desktop with Vulkan, using C or Python bindings, or in the browser
with experimental WebGPU support.

<a href="examples/gallery/showcases/protein_arcball_viewer/"><img src="assets/gallery/v0.4/showcases/protein_arcball_viewer.webp" alt="Protein Viewer" style="width:100%;border-radius:8px;margin:1.5rem 0 2rem;display:block;"></a>


## Start Here

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install/">
<strong>Install</strong>
<span>Choose the right setup path for Python, C/C++, macOS, Linux, or Windows.</span>
</a>
<a class="dvz-nav-card" href="start/quickstart/">
<strong>Quickstart</strong>
<span>Run one scatter plot and learn the three basic parts: scene, panel, and visual.</span>
</a>
<a class="dvz-nav-card" href="examples/">
<strong>Examples</strong>
<span>Browse working visuals, features, and showcases before writing your own code.</span>
</a>
<a class="dvz-nav-card" href="how-to/create-a-scene/">
<strong>How-To Guides</strong>
<span>Learn focused tasks such as adding axes, colorbars, interaction, and offscreen output.</span>
</a>
<a class="dvz-nav-card" href="reference/">
<strong>Reference</strong>
<span>Look up visual families, attribute names, API status, and platform support.</span>
</a>
</div>


## Quick example

A scatter plot of 10 000 random points with pan/zoom:

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
    # The panel is the drawing area where the scatter plot will appear.
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)

    # Add mouse interaction to the panel. Pan/zoom is limited to X and Y because
    # the points are flat, with z = 0.
    controller = dvz.dvz_panzoom(scene, None)
    dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

    # Create one point visual for the whole dataset. Each data call attaches
    # one array to a named visual attribute.
    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "diameter_px", diameters)

    # Add the visual to the panel so it becomes part of the figure.
    dvz.dvz_panel_add_visual(panel, visual, None)

    # Open a window and keep the app running until the user closes it.
    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    #define N 10000

    int main(void) {
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

        /* Add the visual to the panel so it becomes part of the figure. */
        dvz_panel_add_visual(panel, visual, NULL);

        /* Open a window and keep the app running until the user closes it. */
        DvzApp* app = dvz_app(scene);
        dvz_view_window(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

![10 000 colored dots in an interactive window with pan-and-zoom](assets/gallery/v0.4/start/start_scatter.webp)

See [Quickstart](start/quickstart.md) for a fuller walkthrough, or [AI-assisted workflow](start/ai-workflow.md) to have an LLM generate Datoviz code for you.
