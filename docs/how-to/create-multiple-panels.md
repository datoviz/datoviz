# Create Multiple Panels

Split a figure into coordinated viewports.

## Task Workflow

Create one figure, define a panel grid or multiple panels, attach visuals to each panel, and bind
controllers per panel or share controllers when views should move together.

## Minimal Call Sequence

```c
DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
DvzPanel* left = dvz_grid_panel(grid, 0, 0);
DvzPanel* right = dvz_grid_panel(grid, 0, 1);
dvz_panel_add_visual(left, visual_a, NULL);
dvz_panel_add_visual(right, visual_b, NULL);
```

Use `dvz_grid_set_margins()` and `dvz_grid_set_gutter()` when the panels need reserved space or
consistent spacing.

## Canonical Examples

- Gallery: [Panel Grid](../examples/gallery/features/feature_panel_grid.md)
- Source: `examples/c/features/panel_grid.c`
- Gallery: [Multiple Panels](../examples/gallery/features/feature_panel_multi.md)
- Source: `examples/c/features/panel_multi.c`
- Gallery: [Linked Panels](../examples/gallery/features/feature_panel_linked.md)
- Source: `examples/c/features/panel_linked.c`

## Important Details

Panels are viewports, not separate scenes. Share data and controllers deliberately; do not duplicate
the whole scene unless the runtime really needs separate lifetimes.

## Common Mistakes

- Adding one visual to the wrong panel and debugging the controller instead.
- Assuming every panel shares a controller automatically.
- Recreating figures to make subplots.

## See Also

- [Link panels and controllers](link-panels.md)
- [Add axes](axes.md)
- [Use panzoom](use-panzoom.md)
