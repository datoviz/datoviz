# Use Coordinate Systems

Map your data coordinates into panel space.

## Task Workflow

Decide the domain of each panel, upload positions in that domain, then attach a controller that
operates on the same axes. For normalized examples, positions often already live in `[-1, 1]`.

## Minimal Call Sequence

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzController* controller = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);
```

For 3D scenes, use a 3D controller and keep object positions in the model coordinate system chosen
by the example.

## Canonical Examples

- Gallery: [Coordinate System](../examples/gallery/features/feature_coordinate_system.md)
- Source: `examples/c/features/coordinate_system.c`
- Gallery: [Panel View 2D](../examples/gallery/features/feature_panel_view2d.md)
- Source: `examples/c/features/panel_view2d.c`
- Gallery: [User Scale](../examples/gallery/features/feature_user_scale.md)
- Source: `examples/c/features/user_scale.c`

## Important Details

Panel domains describe the visible data range. Controllers modify the view over that domain. Visual
transforms are for object-level placement and should not replace a panel domain when the task is
ordinary data navigation.

## Common Mistakes

- Uploading pixel coordinates to a data-space panel.
- Setting only one axis domain and expecting aspect-preserving 2D navigation.
- Mixing 2D panzoom with 3D orbit/arcball camera control.

## See Also

- [Use panzoom](use-panzoom.md)
- [Configure cameras](configure-cameras.md)
- [Transform visual data](transforms-and-scales.md)
