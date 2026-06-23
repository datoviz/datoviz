# Datoviz

**GPU rendering engine for scientific visualization** — built for scientists and developers who need to explore large datasets interactively. Render millions of points, meshes, volumes, and time series in 2D and 3D at high quality, on the desktop or in the browser.

<a href="examples/gallery/showcases/protein_arcball_viewer.md"><img src="assets/gallery/v0.4/showcases/protein_arcball_viewer.webp" alt="Protein Viewer" style="width:100%;border-radius:8px;margin:1.5rem 0 2rem;display:block;"></a>


## Documentation

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install.md">
<strong>Get Started</strong>
<span>Install Datoviz and run your first visualization in Python or C.</span>
</a>
<a class="dvz-nav-card" href="how-to/create-a-scene.md">
<strong>How-To Guides</strong>
<span>Task-focused recipes: axes, colorbars, panzoom, picking, offscreen rendering, and more.</span>
</a>
<a class="dvz-nav-card" href="reference/index.md">
<strong>Reference</strong>
<span>C and Python API, visual families, feature status, and platform support.</span>
</a>
<a class="dvz-nav-card" href="examples/index.md">
<strong>Examples</strong>
<span>Gallery of visualizations organized by visual type, feature, and technique.</span>
</a>
</div>


## Quick example

A scatter plot of 10 000 random points with pan/zoom:

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
    diameters = np.full(N, 5.0, dtype=np.float32)

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
    dvz.dvz_visual_set_data(visual, "diameter_px", diameters)
    dvz.dvz_panel_add_visual(panel, visual, None)

    # --- run ---
    # or replace with dvz.capture(scene, figure, path="output.png") for offscreen PNG
    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    int main(void) {
        /* data */
        int N = 10000;
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
        dvz_visual_set_data(visual, "diameter_px", diameter_px, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* run; for offscreen PNG use dvz_view_offscreen() and dvz_view_capture_png(). */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

![10 000 colored dots in an interactive window with pan-and-zoom](assets/gallery/v0.4/start/start_scatter.webp)

See [Quickstart](start/quickstart.md) for a fuller walkthrough, or [AI-assisted workflow](start/ai-workflow.md) to have an LLM generate Datoviz code for you.
