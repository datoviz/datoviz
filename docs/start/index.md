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

| I want to display… | Go to |
| --- | --- |
| Scatter plot / point cloud | [Point visual](../reference/visual-families/point.md) |
| Line / path / trajectory | [Path visual](../reference/visual-families/path.md) |
| Mesh / surface | [Mesh visual](../reference/visual-families/mesh.md) |
| Volume / 3D scalar field | [Volume visual](../reference/visual-families/volume.md) |
| Image / texture | [Image visual](../reference/visual-families/image.md) |
| Text / labels | [Text visual](../reference/visual-families/text.md) |
| Markers / symbols | [Marker visual](../reference/visual-families/marker.md) |
| Spheres (3D impostor) | [Sphere visual](../reference/visual-families/sphere.md) |
| All visual families | [Visual families reference](../reference/visual-families/index.md) |


## I want to…

| I want to… | Go to |
| --- | --- |
| Pan and zoom | [Use panzoom](../how-to/use-panzoom.md) |
| Rotate in 3D | [Use arcball](../how-to/use-arcball.md) |
| Render offscreen / headless | [Render offscreen](../how-to/render-offscreen.md) |
| Capture a PNG | [Capture an image](../how-to/capture-an-image.md) |
| Add a colorbar | [Add colorbars](../how-to/add-colorbars.md) |
| Add axes | [Add axes](../how-to/add-axes.md) |
| Pick / probe data | [Pick and probe](../how-to/pick-and-probe.md) |
| Update data in real time | [Update visual data](../how-to/update-visual-data.md) |
| Add multiple panels | [Create multiple panels](../how-to/create-multiple-panels.md) |
| Run in the browser | [WebGPU / WASM](../advanced/webgpu-renderer.md) |


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

=== "Python"

    ```python
    import ctypes
    import numpy as np
    import datoviz.raw as dvz

    N = 10_000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.random.uniform(0, 1, (N, 4)).astype(np.float32)
    color[:, 3] = 1.0
    sizes = np.full(N, 5.0, dtype=np.float32)

    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)
    dvz.dvz_panel_panzoom(panel)

    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, b"position", ctypes.cast(pos.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_visual_set_data(visual, b"color", ctypes.cast(color.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_visual_set_data(visual, b"size", ctypes.cast(sizes.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_panel_add_visual(panel, visual, None)

    app = dvz.dvz_app(scene)
    view = dvz.dvz_view_glfw(app, figure, 800, 600, b"Scatter plot")
    dvz.dvz_app_run(app, 0)
    dvz.dvz_app_destroy(app)
    dvz.dvz_scene_destroy(scene)
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
        dvz_panel_panzoom(panel);

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

**Offscreen render to PNG**

=== "Python"

    ```python
    import ctypes
    import numpy as np
    import datoviz.raw as dvz

    N = 1000
    pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
    pos[:, 2] = 0.0
    color = np.ones((N, 4), dtype=np.float32)
    sizes = np.full(N, 8.0, dtype=np.float32)

    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)

    visual = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data(visual, b"position", ctypes.cast(pos.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_visual_set_data(visual, b"color", ctypes.cast(color.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_visual_set_data(visual, b"size", ctypes.cast(sizes.ctypes.data, ctypes.c_void_p), N)
    dvz.dvz_panel_add_visual(panel, visual, None)

    app = dvz.dvz_app(scene)
    view = dvz.dvz_view_offscreen(app, figure, 800, 600)
    dvz.dvz_app_run(app, 1)
    dvz.dvz_view_capture_png(view, b"output.png")
    dvz.dvz_app_destroy(app)
    dvz.dvz_scene_destroy(scene)
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
