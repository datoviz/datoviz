# Create Multiple Panels

Split a figure into coordinated viewports.

![Multiple Panels](../assets/gallery/v0.4/features/features_panel_multi.webp)

## Task Workflow

Create one figure, define a panel grid or multiple panels, attach visuals to each panel, and bind
controllers per panel or share controllers when views should move together.

Choose the layout path first:

| Need | Use |
| --- | --- |
| One full-canvas view | `dvz_panel_full()` |
| Regular rows and columns | `dvz_figure_grid()` with `dvz_grid_panel()` |
| A panel spanning grid cells | `dvz_grid_panel_span()` |
| Custom inset or irregular layout | `dvz_panel()` with a normalized `DvzPanelDesc` |

Panels are viewports inside one figure and scene. They do not create separate scenes, data stores,
or controller state automatically.

## Grid Call Sequence

Prerequisite: create one `figure` and two prepared visuals. The result is a one-row, two-column
figure with one visual per panel. These are fragments; see the complete
[Multiple Panels example](../examples/gallery/features/features_panel_multi.md).

=== "Python"

    ```python
    import datoviz as dvz

    grid = dvz.dvz_figure_grid(figure, 1, 2)
    left = dvz.dvz_grid_panel(grid, 0, 0)
    right = dvz.dvz_grid_panel(grid, 0, 1)
    if not grid or not left or not right:
        raise RuntimeError("panel grid creation failed")
    dvz.dvz_panel_add_visual(left, visual_a, None)
    dvz.dvz_panel_add_visual(right, visual_b, None)
    ```

=== "C"

    ```c
    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    DvzPanel* left = dvz_grid_panel(grid, 0, 0);
    DvzPanel* right = dvz_grid_panel(grid, 0, 1);
    dvz_panel_add_visual(left, visual_a, NULL);
    dvz_panel_add_visual(right, visual_b, NULL);
    ```

Use `dvz_grid_set_margins()` and `dvz_grid_set_gutter()` when the panels need reserved space or
consistent spacing.

```c
dvz_grid_set_margins(
    grid,
    &(DvzPanelReserve){
        .left_px = 48.0f, .right_px = 32.0f, .top_px = 32.0f, .bottom_px = 48.0f});
dvz_grid_set_gutter(grid, 24.0f, 20.0f);
```

Rows and columns default to equal weight. Use fixed-pixel or weighted row/column sizes when one
panel is a sidebar, overview, or shared control strip:

```c
dvz_grid_set_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 2.0f);
dvz_grid_set_col_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 1.0f);
dvz_grid_set_row_size(grid, 0, DVZ_GRID_SIZE_FIXED_PX, 160.0f);
```

## Spanning Panels

Use a spanning panel when one plot should occupy several cells:

```c
DvzGrid* grid = dvz_figure_grid(figure, 2, 2);
DvzPanel* top_left = dvz_grid_panel(grid, 0, 0);
DvzPanel* top_right = dvz_grid_panel(grid, 0, 1);
DvzPanel* bottom = dvz_grid_panel_span(grid, 1, 0, 1, 2);
```

Spans must cover contiguous grid cells. Use manual panels when the layout is truly irregular.

## Manual Panels

For inset panels or layouts that do not fit a grid, create panels directly in normalized figure
coordinates:

```c
DvzPanel* left = dvz_panel(
    figure, &(DvzPanelDesc){.x = 0.08f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
DvzPanel* right = dvz_panel(
    figure, &(DvzPanelDesc){.x = 0.52f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
```

Use `dvz_panel_set_desc()` when a retained panel needs a new normalized rectangle after creation.

## Plot Reserves and Padding

Grid margins and gutters position panel rectangles. Panel reserves and padding affect the content
inside one panel.

Use panel reserves for axes, labels, colorbars, scale bars, or readouts that need stable
logical-pixel bands around the plot area:

```c
dvz_panel_set_reserve(
    panel,
    &(DvzPanelReserve){.left_px = 72.0f, .right_px = 20.0f, .top_px = 16.0f, .bottom_px = 56.0f});
```

Use panel padding to inset the whole panel content before reserves are resolved. `dvz_panel_full()`
is still a good choice for single-panel scenes; add reserve or padding only when adornments need it.

## Controllers

Bind controllers per panel. Separate controllers make panels move independently:

```c
DvzController* left_panzoom = dvz_panzoom(scene, NULL);
DvzController* right_panzoom = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(left, left_panzoom, DVZ_DIM_MASK_XY);
dvz_panel_bind_controller(right, right_panzoom, DVZ_DIM_MASK_XY);
```

Share one controller pointer when views should move together:

```c
DvzController* shared = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(left, shared, DVZ_DIM_MASK_XY);
dvz_panel_bind_controller(right, shared, DVZ_DIM_MASK_XY);
```

Use a narrower dimension mask, such as `DVZ_DIM_MASK_X`, when only one axis should be linked. See
[Link panels and controllers](link-panels.md) for linked-controller patterns.


## Important Details

Panels are viewports in one figure and scene, not separate scenes. Share data and controllers
deliberately; do not duplicate the whole scene unless the runtime really needs separate lifetimes.

Attaching a visual to one panel does not attach it to every panel. Call `dvz_panel_add_visual()` for
each panel that should draw that visual, and use an explicit `DvzVisualAttachDesc` when the same
visual needs data-coordinate attachment, a different draw layer, or panel-fixed coordinates.

Panel domains, cameras, backgrounds, borders, techniques, axes, colorbars, and annotations are
panel-specific. Configure each panel explicitly unless a helper or shared object is meant to link
state.

Destroying a grid detaches grid-owned panels at their last resolved positions; it does not destroy
the panels. Prefer keeping the grid alive for ordinary retained layouts.

## Common Mistakes

- Adding one visual to the wrong panel and debugging the controller instead.
- Assuming every panel shares a controller automatically.
- Recreating figures to make subplots.
- Using manual normalized panels for a regular grid that should use `dvz_figure_grid()`.
- Expecting grid margins to reserve space for axes inside each panel; use panel reserves for that.
- Linking panels with incompatible domains or cameras.
- Forgetting to attach a shared visual to every panel that should draw it.

## See Also

- [Link panels and controllers](link-panels.md)
- [Add visuals to a panel](add-a-visual.md)
- [Use coordinate systems](coordinate-systems.md)
- [Add axes](axes.md)
- [Use panzoom](use-panzoom.md)

??? example "Related examples"

    - [Panel Grid](../examples/gallery/features/features_panel_grid.md) - Source: `examples/c/features/panel_grid.c`
    - [Multiple Panels](../examples/gallery/features/features_panel_multi.md) - Source: `examples/c/features/panel_multi.c`
    - [Linked Panels](../examples/gallery/features/features_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
