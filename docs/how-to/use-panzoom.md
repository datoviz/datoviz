# Use Panzoom

Enable 2D mouse pan and wheel zoom on a panel.

Use panzoom for 2D data views where pointer drag and wheel input should change the visible X/Y
range. Use custom input callbacks only for application behavior that is not ordinary navigation.

## Task Workflow

Set a panel domain for the visible data range, create a panzoom controller, then bind it to the
panel with the axes that should respond to interaction.

## Minimal Call Sequence

When you already have a live app view, use the view helper. It creates the controller, binds it to
the panel, and connects it to the view input router:

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzPanzoom* panzoom = dvz_view_panzoom(view, panel, NULL);
```

When you need one-axis or shared-controller binding in a live app, create the controller explicitly
and bind it through the view so the panel is also connected to input:

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzController* controller = dvz_panzoom(scene, NULL);
dvz_view_bind_controller(view, panel, controller, DVZ_DIM_MASK_XY);
```

Use `DVZ_DIM_MASK_X` or `DVZ_DIM_MASK_Y` for one-axis navigation.


## Domains And Extents

Panel domains define the base data range. Panzoom state is applied on top of that domain to compute
the current visible range. Query the resolved visible range when axes, labels, overlays, or host UI
need to follow navigation:

```c
double visible_xmin = 0.0;
double visible_xmax = 0.0;
if (dvz_panel_visible_domain(panel, DVZ_DIM_X, &visible_xmin, &visible_xmax))
{
    /* Update external UI or diagnostics from the visible X range. */
}
```

Set the domain before adding data-space visuals. If the domain changes later, retained visuals do
not need rewritten positions; the scene remaps the same source data through the new visible range.


## Options

Pass a descriptor when the default 2D interaction policy is not enough:

```c
DvzPanzoomDesc desc = dvz_panzoom_desc();
desc.flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;

DvzController* controller = dvz_panzoom(scene, &desc);
dvz_view_bind_controller(view, panel, controller, DVZ_DIM_MASK_XY);
```

Use `DVZ_PANZOOM_FLAGS_FIXED_X` or `DVZ_PANZOOM_FLAGS_FIXED_Y` for a controller that should ignore
one axis. Prefer binding with `DVZ_DIM_MASK_X` or `DVZ_DIM_MASK_Y` when the axis restriction is part
of the panel relationship rather than the controller behavior itself.


## Linked Panels

Sharing one controller handle links the bound state for every panel that uses it:

```c
DvzController* shared_x = dvz_panzoom(scene, NULL);

dvz_view_bind_controller(view, top_panel, shared_x, DVZ_DIM_MASK_X);
dvz_view_bind_controller(view, bottom_panel, shared_x, DVZ_DIM_MASK_X);
```

Use separate controllers when panels should navigate independently. For linked X with independent
Y ranges, bind one X controller to both panels and bind separate Y controllers to each panel. The
`examples/c/features/panel_linked.c` example shows the fuller pattern with explicit controller
links.


## Important Details

Panzoom is a 2D controller. Use 3D controllers for orbit, turntable, fly, or arcball navigation.
Controllers live with the scene that created them.

`dvz_view_panzoom()` always binds both X and Y dimensions. Use `dvz_panzoom()` plus
`dvz_view_bind_controller()` when you need one-axis binding, shared controllers, or custom link
topology.

## Common Mistakes

- Binding panzoom before deciding the panel domain.
- Creating separate controllers for panels that should stay linked.
- Using panzoom for a 3D mesh scene.
- Expecting `dvz_view_panzoom()` to create an X-only or Y-only binding.
- Reading the base panel domain when the application needs the current visible domain after
  navigation.

## See Also

- [Link panels and controllers](link-panels.md)
- [Use 3D controllers](3d-navigation.md)
- [Use coordinate systems](coordinate-systems.md)
- [Handle input events](input-events.md)
- [Controller reference](../reference/controllers.md)

??? example "Related examples"

    - [Panzoom](../examples/gallery/features/feature_panzoom.md) - Source: `examples/c/features/panzoom.c`
    - [Linked Panels](../examples/gallery/features/feature_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Panel View 2D](../examples/gallery/features/feature_panel_view2d.md) - Source: `examples/c/features/panel_view2d.c`
