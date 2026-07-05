# Datoviz

Datoviz is a GPU-powered visualization engine for scientific data. It helps you build fast,
interactive 2D and 3D views when ordinary plotting tools become too slow or too limited.

Use Datoviz when you want to explore many points, images, meshes, volumes, annotations, or custom
scientific scenes on a desktop GPU. The v0.4 documentation focuses on the current API: Python
calls that stay close to the C API, native Vulkan rendering, and a small experimental WebGPU
browser path.

<a href="examples/gallery/showcases/protein_arcball_viewer.md"><img src="assets/gallery/v0.4/showcases/protein_arcball_viewer.webp" alt="Protein Viewer" style="width:100%;border-radius:8px;margin:1.5rem 0 2rem;display:block;"></a>


## Start Here

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install.md">
<strong>Install</strong>
<span>Choose the right setup path for Python, C/C++, macOS, Linux, or Windows.</span>
</a>
<a class="dvz-nav-card" href="start/quickstart.md">
<strong>Quickstart</strong>
<span>Run one scatter plot and learn the three basic parts: scene, panel, and visual.</span>
</a>
<a class="dvz-nav-card" href="examples/index.md">
<strong>Examples</strong>
<span>Browse working visuals, features, and showcases before writing your own code.</span>
</a>
<a class="dvz-nav-card" href="how-to/create-a-scene.md">
<strong>How-To Guides</strong>
<span>Learn focused tasks such as adding axes, colorbars, interaction, and offscreen output.</span>
</a>
<a class="dvz-nav-card" href="reference/index.md">
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
    #include "datoviz/app.h"
    #include "datoviz/scene.h"

    int main(void) {
        /* Create random 2D positions, one RGBA color per point, and a point size in pixels. */
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

        /* Open a window and keep it open until the user closes it. */
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
