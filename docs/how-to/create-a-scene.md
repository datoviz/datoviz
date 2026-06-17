# Create a Scene

Create the retained scene hierarchy used by every v0.4 program.

## Task Workflow

Start with a scene, add one figure, create a panel, attach at least one visual, then choose an
interactive or offscreen view. The scene owns figures, panels, controllers, and visuals; the app
owns the runtime used to render them.

Datoviz is designed for batching. Prefer a small number of visuals with many items in each visual:
one point visual with one million points is the intended shape; one million point visuals with one
item each is not. Group similar elements by visual family, attribute layout, material, and update
cadence so the GPU can draw large batches efficiently.

## Minimal Call Sequence

```c
DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, width, height, 0);
DvzPanel* panel = dvz_panel_full(figure);

DvzVisual* visual = dvz_point(scene, 0);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
dvz_visual_set_data(visual, "diameter", diameter, n);
dvz_panel_add_visual(panel, visual, NULL);

DvzApp* app = dvz_app(scene);
dvz_view_glfw(app, figure, width, height, "Datoviz");
dvz_app_run(app, 0);
dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

Use `dvz_panel_full()` for a single viewport. Use panel-grid helpers only when the figure has
multiple coordinated views.


## Important Details

Create all scene objects before `dvz_app_run()`. Destroy the app before destroying the scene. Keep
CPU arrays alive until the corresponding `dvz_visual_set_data()` call has returned; retained visual
data is then owned by Datoviz.

Minimize visual count aggressively. Add another visual only when elements need a different visual
family, shader/material path, panel attachment, transform, lifetime, or update cadence.

## Common Mistakes

- Creating an app before the scene hierarchy is populated, then wondering why nothing draws.
- Forgetting `dvz_panel_add_visual()`: uploading data does not attach the visual to a panel.
- Mixing pixel coordinates and data coordinates without setting a panel domain or controller.

## See Also

- [Open an interactive window](create-a-window.md)
- [Render offscreen and capture](render-offscreen.md)
- [Add visuals to a panel](add-a-visual.md)

??? example "Related examples"

    - Gallery: [Basic Scene](../examples/gallery/features/feature_basic_scene.md)
    - Source: `examples/c/features/basic_scene.c`
    - First program: [Scatter Plot](../examples/gallery/start/start_scatter.md)
    - Source: `examples/c/start/scatter.c`
