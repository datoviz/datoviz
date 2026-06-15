# Start Here

Datoviz v0.4 is a GPU scientific visualization engine written in C, designed for rendering
millions of data points in 2D and 3D at interactive frame rates on desktop and in the browser.
Its primary user-facing layer is the **scene API** (`dvz_scene`, `dvz_figure`, `dvz_panel`,
`dvz_visual`, `dvz_app`), accessible from C and from Python via raw `ctypes` bindings generated
directly from the C headers. Datoviz is not a high-level plotting library: it does not provide
`plot()`, `scatter()`, or `imshow()` calls. Those belong to VisPy2/GSP, which uses Datoviz as
one rendering backend. Use Datoviz directly when you want GPU-level control, offscreen rendering,
custom visual data pipelines, or browser deployment via WebGPU.


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

These four patterns cover the most common starting points. All use Python `ctypes` via
`import datoviz.raw as dvz`. For the C equivalents, see the [Quickstart](quickstart.md).

**Create a scene and open a window**

```python
import ctypes
import datoviz.raw as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
app = dvz.dvz_app(scene)
view = dvz.dvz_view_glfw(app, figure, 800, 600, b"Datoviz")
dvz.dvz_app_run(app, 0)
dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```

**Add a visual and set data**

```python
import ctypes
import numpy as np
import datoviz.raw as dvz

N = 1000
pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
pos[:, 2] = 0.0
sizes = np.full(N, 8.0, dtype=np.float32)

visual = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(
    visual, b"position", ctypes.cast(pos.ctypes.data, ctypes.c_void_p), N)
dvz.dvz_visual_set_data(
    visual, b"diameter", ctypes.cast(sizes.ctypes.data, ctypes.c_void_p), N)
dvz.dvz_panel_add_visual(panel, visual, None)
```

**Update data in a timer callback**

```python
import ctypes
import datoviz.raw as dvz

# Assumes scene, figure, panel, visual, and app are already created.
# Use Host from datoviz.host for async event-loop integration.
# For a simple frame-driven update, call dvz_visual_set_data() before dvz_app_run().
```

**Capture offscreen to PNG**

```python
import ctypes
import datoviz.raw as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
# ... add visuals and data ...
app = dvz.dvz_app(scene)
view = dvz.dvz_view_offscreen(app, figure, 800, 600)
dvz.dvz_app_run(app, 1)
dvz.dvz_view_capture_png(view, b"output.png")
dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```


## AI-assisted workflow

Datoviz is designed for use with coding agents. Paste this page's URL into your LLM as context,
describe the visualization you want, and ask for Python `ctypes` code using only the documented
v0.4 API. For tips and a prompt template, see [AI-assisted workflow](ai-workflow.md).

<!-- PROMPT_WIDGET_PLACEHOLDER -->
