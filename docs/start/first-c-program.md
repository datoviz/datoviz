# First C Program

This page is a compact C walkthrough for the same scene structure used throughout the v0.4
examples. It creates one figure, one panel, one point visual, and one native window.

For a first runnable C example from a source checkout, use:

```sh
just example-c start/scatter
./build/examples/c/start/scatter --live
```

The source is `examples/c/start/scatter.c`.


## Program Shape

A minimal C program follows this order:

1. prepare C arrays for the data you want to draw;
2. create a scene, figure, and panel;
3. create one visual and attach arrays to its attributes;
4. add the visual to the panel;
5. create an app and window view;
6. run the app;
7. destroy the app before destroying the scene.


## Core Calls

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


## Next Steps

- [Quickstart](quickstart.md) shows the same example in Python and C.
- [Use from C or C++](../how-to/c-integration.md) explains how to integrate Datoviz into a native
  project.
- [Add visuals to a panel](../how-to/add-a-visual.md) explains visual attributes and panel
  attachment in more detail.
