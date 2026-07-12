# First C program

This page is a compact C walkthrough for the same scene structure used throughout the v0.4
examples. It creates one figure, one panel, one point visual, and one native window.

The canonical complete source is `examples/docs/quickstart.c`, also displayed in the
[Quickstart](quickstart.md).
It includes data creation, headers, lifecycle cleanup, and the bounded-frame option used by checks.
From a source checkout, build and run it with:

```sh
just quickstart-c
./build/examples/docs/quickstart
```

The gallery scenario at `examples/c/start/scatter.c` produces the corresponding release screenshot
through the repository's example runner; it is not the standalone teaching fixture shown here.


## Program shape

A minimal C program follows this order:

1. prepare C arrays for the data you want to draw;
2. create a scene, figure, and panel;
3. create one visual and attach arrays to its attributes;
4. add the visual to the panel;
5. create an app and window view;
6. run the app;
7. destroy the app before destroying the scene.


## Call-sequence excerpt

The excerpt below assumes that `positions`, `colors`, `diameter_px`, and `count` already exist. It
omits includes, allocation, return-value checks, and failure cleanup to emphasize the object and call
order. Do not copy it as a complete program; use the canonical source linked above.

```c
DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);

DvzVisual* visual = dvz_point(scene, 0);
dvz_visual_set_data(visual, "position", positions, count);
dvz_visual_set_data(visual, "color", colors, count);
dvz_visual_set_data(visual, "diameter_px", diameter_px, count);
dvz_panel_add_visual(panel, visual, NULL);

DvzApp* app = dvz_app(scene);
dvz_view_window(app, figure, 800, 600, "Datoviz");
dvz_app_run(app, 0);

dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

The important rule is that uploading arrays prepares the visual, but the visual appears only after
`dvz_panel_add_visual()` attaches it to a panel.


## Next steps

- [Quickstart](quickstart.md) shows the same example in Python and C.
- [Use from C or C++](../how-to/c-integration.md) explains how to integrate Datoviz into a native
  project.
- [Add visuals to a panel](../how-to/add-a-visual.md) explains visual attributes and panel
  attachment in more detail.
