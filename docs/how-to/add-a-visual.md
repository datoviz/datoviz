# Add Visuals to a Panel

Create a visual family, upload its attributes, and attach it to a panel.

## Task Workflow

Choose the visual family that matches the primitive you want to draw. Create the visual from the
scene, upload all required attributes with the same item count, then add the visual to the target
panel.

## Minimal Call Sequence

```c
DvzVisual* visual = dvz_point(scene, 0);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
dvz_visual_set_data(visual, "diameter", diameter, n);
dvz_panel_add_visual(panel, visual, NULL);
```

The third argument to `dvz_panel_add_visual()` is an optional transform override. Use `NULL` for the
panel default.

## Canonical Examples

- Gallery: [Point](../examples/gallery/visuals/point_2d.md)
- Source: `examples/c/visuals/point.c`
- Gallery: [Marker](../examples/gallery/visuals/visual_marker.md)
- Source: `examples/c/visuals/marker.c`
- Gallery: [Mesh](../examples/gallery/visuals/visual_mesh.md)
- Source: `examples/c/visuals/mesh.c`

## Important Details

Attribute names and array shapes are visual-specific. Use the visual family reference when moving
between point, image, mesh, text, and volume data.

## Common Mistakes

- Uploading different counts for related attributes.
- Reusing point attributes on another visual family without checking that family's reference page.
- Creating a visual but never attaching it to a panel.

## See Also

- [Choose a visual family](choose-a-visual-family.md)
- [Update visual data](update-visual-data.md)
- [Transform visual data](transforms-and-scales.md)
