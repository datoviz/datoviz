# Get Started

Datoviz is a GPU rendering engine for scientific visualization — built for scientists and
developers who need to explore large datasets interactively. Render millions of points, meshes,
volumes, and time series in 2D and 3D, at interactive frame rates, on the desktop or in the
browser.

**C** and **Python** (via `ctypes`) are the supported languages. There is no `plot()` or `scatter()`
— those belong to [VisPy2/GSP](../explanation/gsp-vispy2-boundary.md), which builds on Datoviz.
An LLM can write the ctypes boilerplate for you — see [AI-assisted workflow](ai-workflow.md).


## I want to display…

<div class="vf-grid">
<div class="vf-group">
<div class="vf-dim-label">0D — point-like</div>
<div class="vf-cards">
<a class="vf-card" href="../reference/visual-families/pixel/"><img src="../assets/gallery/v0.4/thumbs/v_pixel.webp" alt="Pixel"><span>Pixel</span></a>
<a class="vf-card" href="../reference/visual-families/point/"><img src="../assets/gallery/v0.4/thumbs/v_point.webp" alt="Point"><span>Point</span></a>
<a class="vf-card" href="../reference/visual-families/marker/"><img src="../assets/gallery/v0.4/thumbs/v_marker.webp" alt="Marker"><span>Marker</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">1D — line-like</div>
<div class="vf-cards">
<a class="vf-card" href="../reference/visual-families/segment/"><img src="../assets/gallery/v0.4/thumbs/v_segment.webp" alt="Segment"><span>Segment</span></a>
<a class="vf-card" href="../reference/visual-families/path/"><img src="../assets/gallery/v0.4/thumbs/v_path.webp" alt="Path"><span>Path</span></a>
<a class="vf-card" href="../reference/visual-families/"><img src="../assets/gallery/v0.4/thumbs/v_vector.webp" alt="Vector"><span>Vector</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">2D — planar</div>
<div class="vf-cards">
<a class="vf-card" href="../reference/visual-families/image/"><img src="../assets/gallery/v0.4/thumbs/v_image.webp" alt="Image"><span>Image</span></a>
<a class="vf-card" href="../reference/visual-families/glyph/"><img src="../assets/gallery/v0.4/thumbs/v_glyph.webp" alt="Glyph"><span>Glyph</span></a>
<a class="vf-card" href="../reference/visual-families/text/"><img src="../assets/gallery/v0.4/thumbs/v_text.webp" alt="Text"><span>Text</span></a>
<a class="vf-card" href="../reference/visual-families/primitive/"><img src="../assets/gallery/v0.4/thumbs/v_primitive.webp" alt="Primitive"><span>Primitive</span></a>
</div>
</div>
<div class="vf-group">
<div class="vf-dim-label">3D — volumetric</div>
<div class="vf-cards">
<a class="vf-card" href="../reference/visual-families/mesh/"><img src="../assets/gallery/v0.4/thumbs/v_mesh.webp" alt="Mesh"><span>Mesh</span></a>
<a class="vf-card" href="../reference/visual-families/volume/"><img src="../assets/gallery/v0.4/thumbs/v_volume.webp" alt="Volume"><span>Volume</span></a>
<a class="vf-card" href="../reference/visual-families/sphere/"><img src="../assets/gallery/v0.4/thumbs/v_sphere.webp" alt="Sphere"><span>Sphere</span></a>
<a class="vf-card" href="../reference/visual-families/splat/"><img src="../assets/gallery/v0.4/thumbs/v_splat.webp" alt="Splat"><span>Splat</span></a>
</div>
</div>
</div>

[All visual families →](../reference/visual-families/index.md)


## I want to…

<div class="vf-grid">
<div class="vf-cards">
<a class="vf-card" href="../how-to/use-panzoom/"><img src="../assets/gallery/v0.4/thumbs/f_panzoom.webp" alt="Pan and zoom"><span>Pan and zoom</span></a>
<a class="vf-card" href="../how-to/use-arcball/"><img src="../assets/gallery/v0.4/thumbs/f_arcball.webp" alt="Rotate in 3D"><span>Rotate in 3D</span></a>
<a class="vf-card" href="../how-to/add-colorbars/"><img src="../assets/gallery/v0.4/thumbs/f_colorbar.webp" alt="Add a colorbar"><span>Add a colorbar</span></a>
<a class="vf-card" href="../how-to/add-axes/"><img src="../assets/gallery/v0.4/thumbs/f_axes.webp" alt="Add axes"><span>Add axes</span></a>
<a class="vf-card" href="../how-to/pick-and-probe/"><img src="../assets/gallery/v0.4/thumbs/f_pick.webp" alt="Pick and probe"><span>Pick and probe</span></a>
<a class="vf-card" href="../how-to/create-multiple-panels/"><img src="../assets/gallery/v0.4/thumbs/f_panels.webp" alt="Multiple panels"><span>Multiple panels</span></a>
<a class="vf-card" href="../how-to/update-visual-data/"><img src="../assets/gallery/v0.4/thumbs/f_realtime.webp" alt="Update data"><span>Update data</span></a>
<a class="vf-card" href="../how-to/render-offscreen/"><img src="../assets/gallery/v0.4/thumbs/f_capture.webp" alt="Render offscreen"><span>Render offscreen</span></a>
</div>
</div>

[Capture a PNG →](../how-to/capture-an-image.md) · [Run in the browser →](../advanced/webgpu-renderer.md) · [All how-to guides →](../how-to/index.md) · [Feature gallery →](../examples/feature-gallery.md)


## I want to use layer…

| Layer | Use when | Go to |
| --- | --- | --- |
| Scene (C / Python ctypes) | Building visualizations or desktop apps | [Quickstart](quickstart.md) |
| vklite | Writing a custom Vulkan renderer | [vklite](../advanced/vklite.md) |
| WebGPU renderer | Embedding the GPU renderer | [WebGPU renderer](../advanced/webgpu-renderer.md) |
| Canvas + stream | Custom renderer, GLFW/Qt embedding, video | [Canvas and stream](../advanced/canvas.md) |


## Minimal code patterns

Complete standalone examples. See the [Quickstart](quickstart.md) for a fuller walkthrough.

**Scatter plot — 10k random points with pan/zoom**

![Scatter plot — 10 000 random colored points with pan/zoom](../assets/gallery/v0.4/start/start_scatter.webp)

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.uniform(0, 1, (N, 4)).astype(np.float32)
    color[:, 3] = 1.0
    sizes = np.full(N, 5.0, dtype=np.float32)

    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)
    controller = dvz.dvz_panzoom(scene, None)
    dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "size", sizes)
    dvz.dvz_panel_add_visual(panel, visual, None)

    dvz.run(scene, figure, title="Scatter plot")
    ```

=== "C"

    ```c
    #include <stdlib.h>
    #include "datoviz/scene.h"

    int main(void) {
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

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Scatter plot");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

---

**Offscreen render to PNG**

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    N = 1000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.ones((N, 4), dtype=np.float32)
    sizes = np.full(N, 8.0, dtype=np.float32)

    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)

    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, "position", pos)
    dvz.dvz_visual_set_data(visual, "color", color)
    dvz.dvz_visual_set_data(visual, "size", sizes)
    dvz.dvz_panel_add_visual(panel, visual, None)

    dvz.capture(scene, figure, path="output.png")
    ```

=== "C"

    ```c
    #include <stdlib.h>
    #include "datoviz/scene.h"

    int main(void) {
        int N = 1000;
        float pos[N * 3], color[N * 4], size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+2] = 0;
            color[4*i+0] = color[4*i+1] = color[4*i+2] = color[4*i+3] = 1.0f;
            size[i] = 8.0f;
        }

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_offscreen(app, figure, 800, 600);
        dvz_app_run(app, 1);
        dvz_view_capture_png(view, "output.png");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```


## AI-assisted workflow

Datoviz is designed for use with coding agents. Paste this page's URL into your LLM as context,
describe the visualization you want, and ask for Python `ctypes` code using only the documented
v0.4 API. For tips and a prompt template, see [AI-assisted workflow](ai-workflow.md).

<!-- PROMPT_WIDGET_PLACEHOLDER -->
