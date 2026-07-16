# Create a Scene

Create the basic structure used by most Datoviz visualizations.

![Three colored points in a basic Datoviz scene](../assets/gallery/v0.4/features/features_basic_scene.webp)

!!! info "At a glance"

    **Status:** Supported scene workflow · **Languages:** Python NumPy facade and C
    **Prerequisites:** A working Datoviz installation and native graphics runtime
    **Result:** One 800 × 600 window containing three colored points

## Task workflow

Start by creating the scene structure: one scene, one figure, one panel, and at least one visual.
Then attach data arrays to the visual attributes, add the visual to the panel, create a window or
offscreen target, and run or capture the result. This page uses a small point example so you can see
the main pieces without extra plotting features.

Use the same object model from C and Python. In Python, import the main `datoviz` package:

```python
import datoviz as dvz
```

The Python calls use the same `dvz_*` function names as the C examples and accept NumPy arrays for
common visual-data uploads. Use `datoviz.raw` only when you need the exact C-shaped pointer/count
call form of the same generated binding.

| Object | Role | Created by |
| --- | --- | --- |
| Scene | The whole visualization: figures, visuals, controllers, scales, and related state. | `dvz_scene()` |
| Figure | The image area and its panel layout. A figure can later be shown in a window or rendered offscreen. | `dvz_figure()` |
| Panel | A drawing area inside a figure. A simple plot usually starts with one full-size panel. | `dvz_panel_full()`, grid helpers, or custom panel descriptors. |
| Visual | A renderable collection such as points, lines, an image, a mesh, or text labels. | Visual-family constructors such as `dvz_point()`. |
| Controller | Mouse or camera interaction, such as pan/zoom for 2D or arcball rotation for 3D. | `dvz_panzoom()`, `dvz_arcball()`, and related helpers. |
| Adornment | Extra visual context such as axes, labels, colorbars, legends, or scale bars. | Panel/adornment helpers. |
| App and view | The part that opens a window, renders frames, or captures an image. | `dvz_app()`, `dvz_view_window()`, or offscreen view helpers. |

Build the scene first, upload visual data, attach the visual to a panel, then decide whether to show
the result in a window or render it offscreen.

Basic checklist:

1. Create the scene.
2. Create a figure and one or more panels.
3. Create visuals, set their data, and configure visual state.
4. Attach visuals, adornments, and controllers to the intended panel.
5. Create an interactive app/view or an offscreen rendering target.
6. Run the app or render/capture the frame.
7. Clean up the app and scene when the program exits.

## Minimal call sequence

The Python tab is a complete success-path script. The C tab is a function-body excerpt: add
`#include <datoviz/datoviz.h>` and a `main()` function, and check pointer and result values in
production code. Both tabs intentionally show explicit app ownership; the shorter managed Python
path is `dvz.run(scene, figure)`.

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

Use `dvz_panel_full()` for a single drawing area. Use panel-grid helpers only when the figure needs
multiple coordinated panels.


The result is an 800 × 600 native window containing three colored points. To add another dataset
to an existing panel, continue with [Add visuals to a panel](add-a-visual.md).

## Important details

Create the initial scene structure before `dvz_app_run()`. You can update data later, but the first
frame is easier to understand when the figure, panels, visuals, controllers, and adornments are
already in place.

Destroy the app before destroying the scene. The app uses the scene while it renders, so the scene
must remain valid until rendering has stopped.

In C, keep CPU arrays alive until the corresponding `dvz_visual_set_data()` call has returned.
After the call returns, Datoviz has stored the data needed for rendering. In Python, pass
C-contiguous NumPy arrays with the dtype and shape required by the visual attribute; the main
`datoviz` package infers the item count for supported visual-data uploads.

Uploading data does not make it visible. A visual appears only after `dvz_panel_add_visual()`
attaches it to a panel.

## Choose visual granularity

Use a small number of visuals, each with many related items. The authoritative grouping rules and
examples are in [Add visuals to a panel](add-a-visual.md#group-items-into-visuals); scene creation
only needs one visual to establish the object model.

## Common mistakes

- Treating `DvzFigure` as a native window instead of an image area that can be shown in a window.
- Creating an app before the scene has a figure, panel, and visual to draw.
- Forgetting `dvz_panel_add_visual()`: uploading data does not attach the visual to a panel.
- Creating many tiny visuals when one batched visual would describe the same data.
- Mixing pixel coordinates and data coordinates without setting a panel domain or controller.
- Destroying the scene while an app is still rendering it.

## Next steps

- [Open an interactive window](create-a-window.md)
- [Render offscreen](render-offscreen.md)
- [Add visuals to a panel](add-a-visual.md)
- [Use from Python](use-python.md)
- [Create multiple panels](multiple-panels.md)
- [Scene building blocks](../explanation/figure-panel-visual-model.md)

??? example "Complete and related examples"

    - Canonical complete example: [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Quickstart scatter plot](../start/quickstart.md) - Source: `examples/c/start/scatter.c`
