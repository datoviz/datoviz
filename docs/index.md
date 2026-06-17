# Datoviz

**GPU rendering engine for scientific visualization** — built for scientists and developers who need to explore large datasets interactively. Efficiently render **millions of points**, meshes, volumes, and time series in 2D and 3D, at high quality, on the desktop or in the browser.

Datoviz is a **C-first library** with an included auto-generated Python wrapper (using `ctypes`). A higher-level plotting API is coming in Fall 2026 with [**VisPy 2**](explanation/gsp-vispy2-boundary.md). In the meantime, you can use the Datoviz API directly, which involves some boilerplate that an [AI agent](#ai-assisted-workflow) can generate for you.

[Install](start/install.md){ .md-button .md-button--primary } [Examples](examples/index.md){ .md-button }

<div class="dvz-screenshot-row">
<a href="examples/gallery/showcases/showcase_embedding_atlas.md"><img src="assets/gallery/v0.4/showcases/showcase_embedding_atlas.webp" alt="Embedding Atlas"></a>
<a href="examples/gallery/showcases/protein_arcball_viewer.md"><img src="assets/gallery/v0.4/showcases/protein_arcball_viewer.webp" alt="Protein Viewer"></a>
<a href="examples/gallery/showcases/brain_volume.md"><img src="assets/gallery/v0.4/showcases/brain_volume.webp" alt="Brain Volume"></a>
<a href="examples/gallery/showcases/scientific_plotting_workflow.md"><img src="assets/gallery/v0.4/showcases/scientific_plotting_workflow.webp" alt="Scientific Plotting"></a>
</div>


## I want to display…

<div class="vf-grid">
<div class="vf-group">
<div class="vf-dim-label">0D — point-like</div>
<div class="vf-cards">
<a class="vf-card" href="reference/visual-families/pixel/"><img src="assets/gallery/v0.4/thumbs/v_pixel.webp" alt="Pixel"><span>Pixel</span></a>
<a class="vf-card" href="reference/visual-families/point/"><img src="assets/gallery/v0.4/thumbs/v_point.webp" alt="Point"><span>Point</span></a>
<a class="vf-card" href="reference/visual-families/marker/"><img src="assets/gallery/v0.4/thumbs/v_marker.webp" alt="Marker"><span>Marker</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">1D — line-like</div>
<div class="vf-cards">
<a class="vf-card" href="reference/visual-families/segment/"><img src="assets/gallery/v0.4/thumbs/v_segment.webp" alt="Segment"><span>Segment</span></a>
<a class="vf-card" href="reference/visual-families/vector/"><img src="assets/gallery/v0.4/thumbs/v_vector.webp" alt="Vector"><span>Vector</span></a>
<a class="vf-card" href="reference/visual-families/path/"><img src="assets/gallery/v0.4/thumbs/v_path.webp" alt="Path"><span>Path</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">2D — planar</div>
<div class="vf-cards">
<a class="vf-card" href="reference/visual-families/primitive/"><img src="assets/gallery/v0.4/thumbs/v_primitive.webp" alt="Primitive"><span>Primitive</span></a>
<a class="vf-card" href="reference/visual-families/image/"><img src="assets/gallery/v0.4/thumbs/v_image.webp" alt="Image"><span>Image</span></a>
<a class="vf-card" href="reference/visual-families/glyph/"><img src="assets/gallery/v0.4/thumbs/v_glyph.webp" alt="Glyph"><span>Glyph</span></a>
<a class="vf-card" href="reference/visual-families/text/"><img src="assets/gallery/v0.4/thumbs/v_text.webp" alt="Text"><span>Text</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">3D — volumetric</div>
<div class="vf-cards">
<a class="vf-card" href="reference/visual-families/mesh/"><img src="assets/gallery/v0.4/thumbs/v_mesh.webp" alt="Mesh"><span>Mesh</span></a>
<a class="vf-card" href="reference/visual-families/sphere/"><img src="assets/gallery/v0.4/thumbs/v_sphere.webp" alt="Sphere"><span>Sphere</span></a>
<a class="vf-card" href="reference/visual-families/volume/"><img src="assets/gallery/v0.4/thumbs/v_volume.webp" alt="Volume"><span>Volume</span></a>
<a class="vf-card" href="reference/visual-families/splat/"><img src="assets/gallery/v0.4/thumbs/v_splat.webp" alt="Splat"><span>Splat</span></a>
</div>
</div>
</div>

[All visual families →](reference/visual-families/index.md)


## I want to…

<div class="vf-grid">
<div class="vf-cards">
<a class="vf-card" href="how-to/use-panzoom/"><img src="assets/gallery/v0.4/thumbs/f_panzoom.webp" alt="Pan and zoom"><span>Pan and zoom</span></a>
<a class="vf-card" href="how-to/use-arcball/"><img src="assets/gallery/v0.4/thumbs/f_arcball.webp" alt="Rotate in 3D"><span>Rotate in 3D</span></a>
<a class="vf-card" href="how-to/add-colorbars/"><img src="assets/gallery/v0.4/thumbs/f_colorbar.webp" alt="Add a colorbar"><span>Add a colorbar</span></a>
<a class="vf-card" href="how-to/add-axes/"><img src="assets/gallery/v0.4/thumbs/f_axes.webp" alt="Add axes"><span>Add axes</span></a>
<a class="vf-card" href="how-to/pick-and-probe/"><img src="assets/gallery/v0.4/thumbs/f_pick.webp" alt="Pick and probe"><span>Pick and probe</span></a>
<a class="vf-card" href="how-to/create-multiple-panels/"><img src="assets/gallery/v0.4/thumbs/f_panels.webp" alt="Multiple panels"><span>Multiple panels</span></a>
<a class="vf-card" href="how-to/update-visual-data/"><img src="assets/gallery/v0.4/thumbs/f_realtime.webp" alt="Update data"><span>Update data</span></a>
<a class="vf-card" href="how-to/render-offscreen/"><img src="assets/gallery/v0.4/thumbs/f_capture.webp" alt="Render offscreen"><span>Render offscreen</span></a>
</div>
</div>

[All how-to guides →](how-to/index.md) · [All feature examples →](examples/feature-gallery.md)


## I want to use layer…

| Layer | Use when | Go to |
| --- | --- | --- |
| Scene (C / Python ctypes) | Building visualizations or desktop apps | [Quickstart](start/quickstart.md) |
| vklite | Writing a custom Vulkan renderer | [vklite](advanced/vklite.md) |
| WebGPU renderer | Embedding the GPU renderer | [WebGPU renderer](advanced/webgpu-renderer.md) |
| Canvas + stream | Custom renderer, GLFW/Qt embedding, video | [Canvas and stream](advanced/canvas.md) |


## Minimal code patterns

Complete standalone examples. See the [Quickstart](start/quickstart.md) for a fuller walkthrough.

**Scatter plot — 10k random points with pan/zoom**

The scene setup is identical for interactive and offscreen use — only the last call differs.

![Scatter plot — 10 000 random colored points with pan/zoom](assets/gallery/v0.4/start/start_scatter.webp)

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    # --- data ---
    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.uniform(0, 1, (N, 4)).astype(np.float32)
    color[:, 3] = 1.0
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
    # interactive window — or replace with dvz.capture(scene, figure, path="output.png") for offscreen PNG
    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    ```c
    #include <stdlib.h>
    #include "datoviz/scene.h"

    int main(void) {
        /* data */
        int N = 10000;
        float pos[N * 3], color[N * 4], size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+2] = 0;
            color[4*i+0] = (float)rand()/RAND_MAX;
            color[4*i+1] = (float)rand()/RAND_MAX;
            color[4*i+2] = (float)rand()/RAND_MAX;
            color[4*i+3] = 1.0f;
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

        /* run — interactive window
           or replace with offscreen PNG:
             DvzView* view = dvz_view_offscreen(app, figure, 800, 600);
             dvz_app_run(app, 1);
             dvz_view_capture_png(view, "output.png"); */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```


## AI-assisted workflow

Datoviz works well with coding agents. Give your LLM this page as context and describe what you want:

```
Using the Datoviz v0.4 Python API (https://datoviz.org/),
write ctypes code for a scatter plot of 10k random points with pan/zoom.
```

For a fuller prompt template and tips, see [AI-assisted workflow](start/ai-workflow.md).

<!-- PROMPT_WIDGET_PLACEHOLDER -->
