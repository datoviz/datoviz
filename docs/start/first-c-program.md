# First C program

This page is a compact C walkthrough for the same scene structure used throughout the v0.4
examples. It creates one figure, one panel, one point visual, and one native window.

<div class="dvz-context-strip">
  <span>C11</span>
  <span>Native window</span>
  <span>Complete source linked</span>
  <span>Call-order excerpt below</span>
</div>

The canonical complete program is `examples/docs/quickstart.c`, displayed in the C tab of the
[Quickstart](quickstart.md). It includes data creation, headers, result checks, lifecycle cleanup,
and the bounded-frame option used by automated checks.

The gallery scenario at `examples/c/start/scatter.c` produces the corresponding release screenshot
through the repository's example runner; it is not the standalone teaching fixture shown here.

![The 10,000-point scene produced by the first C program](../assets/gallery/v0.4/start/start_scatter.webp)


## Run the complete program

From a source checkout:

```sh
just quickstart-c
./build/examples/docs/quickstart
```

You should see the same 10,000-point interactive scene as the Python Quickstart. Drag to pan and
scroll to zoom. For a file in your own project, use the exported CMake target or
`datoviz-config` described in [C/C++ integration](../how-to/c-integration.md).


## Program shape

A minimal C program follows this order:

1. prepare C arrays for the data you want to draw;
2. create a scene, figure, and panel;
3. create one visual and attach arrays to its attributes;
4. add the visual to the panel;
5. create an app and window view;
6. run the app;
7. destroy the app before destroying the scene.


## Call-sequence excerpt—not a complete program

The fragment below assumes that `positions`, `colors`, `diameter_px`, and `count` already exist. It
omits headers, data allocation, result checks, and failure cleanup to emphasize object and call
order. Use it to understand the sequence; copy the complete Quickstart program when starting a new
file.

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

The complete source also checks fallible setup calls and destroys `DvzApp` before `DvzScene`. Keep
that cleanup order when adding an error path.


## Next steps

- [Quickstart](quickstart.md) displays the complete source in both Python and C.
- [Core concepts](core-concepts.md) explains the scene, figure, panel, visual, and view model.
- [Use from C or C++](../how-to/c-integration.md) explains how to integrate Datoviz into a native
  project.
- [Add visuals to a panel](../how-to/add-a-visual.md) explains visual attributes and panel
  attachment in more detail.
