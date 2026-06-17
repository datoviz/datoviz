# Use Panzoom

Enable 2D mouse pan and wheel zoom on a panel.

## Task Workflow

Set a panel domain for the visible data range, create a panzoom controller, then bind it to the
panel with the axes that should respond to interaction.

## Minimal Call Sequence

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzController* controller = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);
```

Use `DVZ_DIM_MASK_X` or `DVZ_DIM_MASK_Y` for one-axis navigation.


## Important Details

Panzoom is a 2D controller. Use 3D controllers for orbit, turntable, fly, or arcball navigation.
Sharing one panzoom controller across panels links their view state.

## Common Mistakes

- Binding panzoom before deciding the panel domain.
- Creating separate controllers for panels that should stay linked.
- Using panzoom for a 3D mesh scene.

## See Also

- [Link panels and controllers](link-panels.md)
- [Use 3D controllers](3d-navigation.md)
- [Use coordinate systems](coordinate-systems.md)

??? example "Related examples"

    - Gallery: [Panzoom](../examples/gallery/features/feature_panzoom.md)
    - Source: `examples/c/features/panzoom.c`
    - Gallery: [Linked Panels](../examples/gallery/features/feature_panel_linked.md)
    - Source: `examples/c/features/panel_linked.c`
    - Gallery: [Panel View 2D](../examples/gallery/features/feature_panel_view2d.md)
    - Source: `examples/c/features/panel_view2d.c`
