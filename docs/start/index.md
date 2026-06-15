# Start Here

Datoviz is a GPU rendering engine for scientific visualization — built for scientists and
developers who need to explore large datasets interactively. Render millions of points, meshes,
volumes, and time series in 2D and 3D, at interactive frame rates, on the desktop or in the
browser.

You use it from **C** or **Python** (via `ctypes`). There is no `plot()` or `scatter()` — those
belong to [VisPy2/GSP](../explanation/gsp-vispy2-boundary.md), which builds on Datoviz.
If you want GPU-level control, offscreen rendering, or browser deployment, you are in the right
place. If you are waiting for a high-level Python plotting API, VisPy2 is coming this Fall — and in the
meantime, an LLM can write the ctypes code for you. See [AI-assisted workflow](ai-workflow.md).


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


## I want to…

| | I want to… | Go to |
| --- | --- | --- |
| ![](../assets/gallery/v0.4/thumbs/f_panzoom.webp){width=120} | Pan and zoom | [Use panzoom](../how-to/use-panzoom.md) |
| ![](../assets/gallery/v0.4/thumbs/f_arcball.webp){width=120} | Rotate in 3D | [Use arcball](../how-to/use-arcball.md) |
| ![](../assets/gallery/v0.4/thumbs/f_capture.webp){width=120} | Render offscreen / headless | [Render offscreen](../how-to/render-offscreen.md) |
| ![](../assets/gallery/v0.4/thumbs/f_capture.webp){width=120} | Capture a PNG | [Capture an image](../how-to/capture-an-image.md) |
| ![](../assets/gallery/v0.4/thumbs/f_colorbar.webp){width=120} | Add a colorbar | [Add colorbars](../how-to/add-colorbars.md) |
| ![](../assets/gallery/v0.4/thumbs/f_axes.webp){width=120} | Add axes | [Add axes](../how-to/add-axes.md) |
| ![](../assets/gallery/v0.4/thumbs/f_pick.webp){width=120} | Pick / probe data | [Pick and probe](../how-to/pick-and-probe.md) |
| ![](../assets/gallery/v0.4/thumbs/f_realtime.webp){width=120} | Update data in real time | [Update visual data](../how-to/update-visual-data.md) |
| ![](../assets/gallery/v0.4/thumbs/f_panels.webp){width=120} | Add multiple panels | [Create multiple panels](../how-to/create-multiple-panels.md) |
| | Run in the browser | [WebGPU / WASM](../advanced/webgpu-renderer.md) |


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
