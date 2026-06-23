# Add Visuals to a Panel

Create a visual family, upload its attributes, and attach it to a panel.

![Point](../assets/gallery/v0.4/visuals/point_2d.webp)

## Task Workflow

Choose the visual family that matches the primitive you want to draw. Create the visual from the
scene, upload all required attributes with the same item count, then add the visual to the target
panel.

Batch aggressively. A visual is a batch of similar items, not a single mark. Prefer one visual with
large `position`, `color`, `diameter`, or equivalent attribute arrays over many visuals with one or
a few items each.

## Basic Call Sequence

```c
DvzVisual* visual = dvz_point(scene, 0);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
dvz_visual_set_data(visual, "diameter", diameter, n);
dvz_panel_add_visual(panel, visual, NULL);
```

Use `NULL` for the default panel attachment: draw layer `0`, controller mode
`DVZ_CONTROLLER_APPLY`, and coordinate space `DVZ_COORD_DATA`.

## Attachment Options

The third argument to `dvz_panel_add_visual()` is an optional `DvzVisualAttachDesc`. Use it when the
visual needs a non-default draw layer, coordinate interpretation, or controller behavior.

```c
DvzVisualAttachDesc attach = dvz_visual_attach_desc();
attach.coord_space = DVZ_COORD_VIEW;
attach.z_layer = 1;
dvz_panel_add_visual(panel, visual, &attach);
```

The most common attachment choices are:

| Field | Default | Use when |
| --- | --- | --- |
| `coord_space` | `DVZ_COORD_DATA` | Set `DVZ_COORD_VIEW` for pre-normalized view positions, or `DVZ_COORD_PANEL` for panel-fixed overlays. |
| `z_layer` | `0` | Draw one visual in front of or behind another visual in the same panel. |
| `controller_mode` | `DVZ_CONTROLLER_APPLY` | Keep the default for ordinary data visuals; use specialized modes only when the example or reference page requires them. |

Set panel domains before attaching data-coordinate visuals:

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

dvz_panel_add_visual(panel, visual, NULL);
```

See [Use coordinate systems](coordinate-systems.md) before mixing data, view, and panel coordinate
spaces.

## Attributes

Attribute names, item counts, and defaults are visual-specific. The point visual uses `position`,
`color`, and `diameter`; mesh, image, text, volume, and path visuals use different attribute names
and data layouts.

Keep related per-item arrays aligned:

```c
dvz_visual_set_data(visual, "position", positions, count);
dvz_visual_set_data(visual, "color", colors, count);
dvz_visual_set_data(visual, "diameter", diameters, count);
```

Do not assume an attribute exists just because another visual family has a similar field. Check the
visual family reference when moving between families.

Use `dvz_visual_set_data_many()` when several attributes should become visible as one coherent
update:

```c
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = positions, .item_count = count},
    {.attr_name = "color", .data = colors, .item_count = count},
    {.attr_name = "diameter", .data = diameters, .item_count = count},
};
dvz_visual_set_data_many(visual, updates, 3);
```

## When to Split Visuals

Split one logical dataset into several visuals only when there is a real rendering or lifetime
boundary:

- different visual family;
- different material, alpha mode, depth behavior, or technique path;
- different panel attachment, coordinate space, or draw layer;
- different transform, lifetime, or update cadence;
- different shared geometry for mesh instancing.

Keep style differences inside one visual when the style is already an attribute, such as point
color, diameter, symbol, radius, or per-item transform.

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

Adding a visual to a panel does not copy it into that panel as an independent object. The scene owns
the visual; panel attachment records where and how it is drawn. If the same visual is attached to
multiple panels, keep its data layout compatible with every attachment.

## Common Mistakes

- Uploading different counts for related attributes.
- Reusing point attributes on another visual family without checking that family's reference page.
- Creating many tiny visuals instead of batching items into one visual.
- Creating a visual but never attaching it to a panel.
- Uploading pre-normalized view coordinates with the default `DVZ_COORD_DATA` coordinate space.
- Splitting by color or size when those are ordinary per-item attributes.

## See Also

- [Choose a visual family](choose-a-visual-family.md)
- [Update visual data](update-visual-data.md)
- [Transform visual data](transforms-and-scales.md)
- [Use coordinate systems](coordinate-systems.md)
- [Visual family reference](../reference/visual-families/index.md)

??? example "Related examples"

    - [Point](../examples/gallery/visuals/point_2d.md) - Source: `examples/c/visuals/point.c`
    - [Marker](../examples/gallery/visuals/visual_marker.md) - Source: `examples/c/visuals/marker.c`
    - [Mesh](../examples/gallery/visuals/visual_mesh.md) - Source: `examples/c/visuals/mesh.c`
    - [Mesh Instance Selection](../examples/gallery/features/feature_selection_mesh_instances.md) - Source: `examples/c/features/selection_mesh_instances.c`
