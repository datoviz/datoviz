# Link Panels and Controllers

Make multiple panels pan, zoom, or probe together.

## Task Workflow

Create the panels first. Bind the same controller object to every panel that should share view
state. Keep separate controllers for panels that should move independently.

## Minimal Call Sequence

```c
DvzController* shared = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel_a, shared, DVZ_DIM_MASK_XY);
dvz_panel_bind_controller(panel_b, shared, DVZ_DIM_MASK_XY);
```

Use a narrower dimension mask to link only one axis.


## Important Details

Controller sharing links interaction state. It does not copy visuals, axes, colorbars, or data
between panels.

## Common Mistakes

- Creating two identical controllers instead of sharing one controller pointer.
- Linking panels with incompatible domains.
- Treating showcase workflows as the minimal linked-panel API.

## See Also

- [Create multiple panels](create-multiple-panels.md)
- [Use panzoom](use-panzoom.md)
- [Probe image or field values](probe-fields.md)

??? example "Related examples"

    - Gallery: [Linked Panels](../examples/gallery/features/feature_panel_linked.md)
    - Source: `examples/c/features/panel_linked.c`
    - Gallery: [Linked Panels With Axes](../examples/gallery/showcases/linked_panels_axes_panzoom.md)
    - Source: `examples/c/showcases/panel_linked_axes.c`
    - Gallery: [Linked Probe With Colorbar](../examples/gallery/showcases/linked_panels_probe_colorbar.md)
    - Source: `examples/c/showcases/linked_probe_colorbar.c`
