# Add Visuals to a Panel

Create a visual family, upload its attributes, and attach it to a panel.

## Task Workflow

Choose the visual family that matches the primitive you want to draw. Create the visual from the
scene, upload all required attributes with the same item count, then add the visual to the target
panel.

Batch aggressively. A visual is a batch of similar items, not a single mark. Prefer one visual with
large `position`, `color`, `diameter`, or equivalent attribute arrays over many visuals with one or
a few items each.

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

## Instanced Meshes

Use mesh instancing when many objects share the same triangle geometry and differ only by placement
or orientation. Common cases include cube fields, repeated markers built from geometry, and many
copies of the same imported mesh.

Create one mesh visual, set the shared geometry once, then upload one `"instance_transform"` matrix
per copy:

```c
DvzVisual* mesh = dvz_mesh(scene, 0);
dvz_mesh_set_geometry(mesh, cube_geometry);
dvz_visual_set_data(mesh, "instance_transform", transforms, instance_count);
dvz_panel_add_visual(panel, mesh, NULL);
```

Use separate mesh visuals only when copies need different materials, techniques, panel attachments,
lifetimes, or update schedules.


## Important Details

Attribute names and array shapes are visual-specific. Use the visual family reference when moving
between point, image, mesh, text, and volume data.

Create separate visuals only for real boundaries: a different visual family, material or technique,
panel attachment, transform, lifetime, or update frequency. Style differences that are represented
as attributes should stay inside the same visual.

## Common Mistakes

- Uploading different counts for related attributes.
- Reusing point attributes on another visual family without checking that family's reference page.
- Creating many tiny visuals instead of batching items into one visual.
- Creating a visual but never attaching it to a panel.

## See Also

- [Choose a visual family](choose-a-visual-family.md)
- [Update visual data](update-visual-data.md)
- [Transform visual data](transforms-and-scales.md)

??? example "Related examples"

    - [Point](../examples/gallery/visuals/point_2d.md) - Source: `examples/c/visuals/point.c`
    - [Marker](../examples/gallery/visuals/visual_marker.md) - Source: `examples/c/visuals/marker.c`
    - [Mesh](../examples/gallery/visuals/visual_mesh.md) - Source: `examples/c/visuals/mesh.c`
    - [Mesh Instance Selection](../examples/gallery/features/feature_selection_mesh_instances.md) - Source: `examples/c/features/selection_mesh_instances.c`
