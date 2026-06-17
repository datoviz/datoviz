# Add Axes

Add 2D axes and axis labels to a panel.

## Task Workflow

Set the panel domain, attach the visual data, then add axes that reflect the same coordinate system.
Use labels when the axis needs named units or titles.

## Minimal Call Sequence

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);
/* Add axes using the helper path shown in examples/c/features/axes_2d.c. */
```

Keep axes in the same panel as the data they describe.


## Important Details

Axes are adornments tied to a panel domain. If you change the domain or link a controller, verify
the ticks and labels still match the visible data range.

## Common Mistakes

- Adding axis labels without matching the data units.
- Treating a composed scientific plotting showcase as the minimal axes example.
- Drawing custom lines for axes when the axes helper already provides the intended behavior.

## See Also

- [Use coordinate systems](coordinate-systems.md)
- [Add colorbars, scale bars, and legends](adornments.md)
- [Link panels and controllers](link-panels.md)

??? example "Related examples"

    - Gallery: [Path With 2D Axes](../examples/gallery/features/path_axes_2d.md)
    - Source: `examples/c/features/axes_2d.c`
    - Gallery: [Axis Labels](../examples/gallery/features/feature_axis_labels.md)
    - Source: `examples/c/features/axis_labels.c`
    - Gallery: [Scientific Plotting Workflow](../examples/gallery/showcases/scientific_plotting_workflow.md)
    - Source: `examples/c/showcases/scientific_plotting.c`
