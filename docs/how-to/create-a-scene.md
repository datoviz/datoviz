# Create a Scene

Create the retained scene hierarchy used by every v0.4 program.

![Basic Scene](../assets/gallery/v0.4/features/feature_basic_scene.webp)

## Task Workflow

Start with a scene, add one figure, create a panel, attach at least one visual, then choose an
interactive or offscreen view. The scene owns figures, panels, controllers, and visuals; the app
owns the runtime used to render them.

Use the same object model from C and Python. In Python, prefer the top-level direct-engine facade
with C-shaped names:

```python
import datoviz as dvz
```

That facade adapts NumPy arrays for declared dense-data calls. Use `datoviz.raw` only when you need
exact generated `ctypes` pointer/count behavior.

| Object | Role | Created by |
| --- | --- | --- |
| Scene | Root retained state: figures, visuals, controllers, scales, sampled fields, and diagnostics. | `dvz_scene()` |
| Figure | Renderable surface size and panel layout. It is not itself a native window. | `dvz_figure()` |
| Panel | Viewport with a domain, transforms, controller bindings, visuals, and adornments. | `dvz_panel_full()`, grid helpers, or custom panel descriptors. |
| Visual | Homogeneous retained batch of renderable items. | Visual-family constructors such as `dvz_point()`. |
| Controller | Navigation state bound to one or more panel dimensions. | `dvz_panzoom()`, `dvz_arcball()`, and related helpers. |
| Adornment | Scene-level context such as axes, labels, colorbars, legends, or scale bars. | Panel/adornment helpers. |
| App and view | Runtime presentation path for an existing scene and figure. | `dvz_app()`, `dvz_view_window()`, or offscreen view helpers. |

Keep those layers separate. Build retained scene state first; then choose how to present or capture
it.

Lifecycle checklist:

1. Create the scene.
2. Create a figure and one or more panels.
3. Create visuals, set their data, and configure visual state.
4. Attach visuals, adornments, and controllers to the intended panel.
5. Create an interactive app/view or an offscreen rendering target.
6. Run the app or render/capture the frame.
7. Destroy runtime objects before destroying the scene.

## Minimal Call Sequence

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    width, height = 800, 600

    positions = np.asarray(
        [
            [-0.45, -0.25, 0.0],
            [0.00, 0.34, 0.0],
            [0.45, -0.25, 0.0],
        ],
        dtype=np.float32,
        order="C",
    )
    colors = np.asarray(
        [
            [0, 200, 255, 255],
            [255, 210, 80, 255],
            [255, 90, 110, 255],
        ],
        dtype=np.uint8,
        order="C",
    )
    diameter_px = np.asarray([42.0, 58.0, 42.0], dtype=np.float32, order="C")

    scene = dvz.dvz_scene()
    app = None
    try:
        figure = dvz.dvz_figure(scene, width, height, 0)
        panel = dvz.dvz_panel_full(figure)

        visual = dvz.dvz_point(scene, 0)
        dvz.dvz_visual_set_data(visual, "position", positions)
        dvz.dvz_visual_set_data(visual, "color", colors)
        dvz.dvz_visual_set_data(visual, "diameter_px", diameter_px)
        dvz.dvz_panel_add_visual(panel, visual, None)

        app = dvz.dvz_app(scene)
        dvz.dvz_view_window(app, figure, width, height, "Datoviz")
        dvz.dvz_app_run(app, 0)
    finally:
        if app is not None:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
    ```

=== "C"

    ```c
    const uint32_t width = 800;
    const uint32_t height = 600;
    const uint32_t n = 3;

    const vec3 positions[3] = {
        {-0.45f, -0.25f, 0.0f},
        {+0.00f, +0.34f, 0.0f},
        {+0.45f, -0.25f, 0.0f},
    };
    const DvzColor colors[3] = {
        {0, 200, 255, 255},
        {255, 210, 80, 255},
        {255, 90, 110, 255},
    };
    const float diameter_px[3] = {42.0f, 58.0f, 42.0f};

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, width, height, 0);
    DvzPanel* panel = dvz_panel_full(figure);

    DvzVisual* visual = dvz_point(scene, 0);
    dvz_visual_set_data(visual, "position", positions, n);
    dvz_visual_set_data(visual, "color", colors, n);
    dvz_visual_set_data(visual, "diameter_px", diameter_px, n);
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    dvz_view_window(app, figure, width, height, "Datoviz");
    dvz_app_run(app, 0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    ```

Use `dvz_panel_full()` for a single viewport. Use panel-grid helpers only when the figure has
multiple coordinated views.


## Important Details

Create the initial scene hierarchy before `dvz_app_run()`. Retained updates are allowed later, but
the first frame is easier to reason about when the figure, panels, visuals, controllers, and
adornments already exist.

Destroy the app before destroying the scene. The app/runtime borrows scene and figure state while
it renders; the scene is the longer-lived semantic owner.

In C, keep CPU arrays alive until the corresponding `dvz_visual_set_data()` call has returned.
Retained visual data is then owned by Datoviz. In Python, pass C-contiguous NumPy arrays with the
dtype and shape required by the visual attribute; the direct-engine facade infers the item count for
declared dense-data uploads.

Uploading data does not make it visible. A visual appears only after `dvz_panel_add_visual()`
attaches it to a panel.

## Visual Granularity

Minimize visual count aggressively. Add another visual only when elements need a different visual
family, shader/material path, panel attachment, transform, lifetime, or update cadence.

Datoviz is designed for batching. Prefer a small number of visuals with many items in each visual:
one point visual with one million points is the intended shape; one million point visuals with one
item each is not. Group similar elements by visual family, attribute layout, material, and update
cadence so the GPU can draw large batches efficiently.

## Common Mistakes

- Treating `DvzFigure` as a native window instead of a retained renderable surface.
- Creating an app before the scene hierarchy is populated, then wondering why nothing draws.
- Forgetting `dvz_panel_add_visual()`: uploading data does not attach the visual to a panel.
- Creating many tiny visuals when one batched visual would describe the same data.
- Mixing pixel coordinates and data coordinates without setting a panel domain or controller.
- Destroying the scene before the app/runtime that is rendering it.

## See Also

- [Open an interactive window](create-a-window.md)
- [Render offscreen and capture](render-offscreen.md)
- [Add visuals to a panel](add-a-visual.md)
- [Use from Python](use-python.md)
- [Create multiple panels](create-multiple-panels.md)
- [Scene model](../explanation/scene-model.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Scatter Plot](../examples/gallery/start/start_scatter.md) - Source: `examples/c/start/scatter.c`
