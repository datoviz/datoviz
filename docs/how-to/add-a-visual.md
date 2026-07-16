# Add Visuals to a Panel

Add a visual when you want a panel to draw a dataset: points, lines, an image, a mesh, text labels,
or another visual family.

![Colored circular points rendered in one panel](../assets/gallery/v0.4/visuals/visuals_point.webp)

!!! info "At a glance"

    **Status:** Supported retained-visual workflow · **Languages:** Python NumPy facade and C
    **Prerequisites:** An existing scene and panel; arrays matching one visual-family contract
    **Result:** One batched visual attached to and rendered by the chosen panel

## Task workflow

A visual is a renderable collection of related items. For a point visual, the items are points. For
an image visual, the items are image corners and texture coordinates. For a mesh visual, the items
are vertices, triangles, or instances depending on the mesh setup.

The usual workflow is:

1. Choose the visual family that matches what you want to draw.
2. Create one visual from the scene.
3. Prepare the data arrays required by that visual family.
4. Upload each array to the matching visual attribute.
5. Add the visual to the panel where it should appear.

Uploading data prepares the visual, but it does not draw anything by itself. The visual becomes part
of a figure only after `dvz_panel_add_visual()`.


## Basic point visual

This setup excerpt adds three points to an existing `scene` and `panel`. It is intentionally not a
standalone program; use [Create a scene](create-a-scene.md) for the surrounding lifecycle.

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    # Three points, each with x/y/z coordinates. z is 0, so the points lie in a
    # 2D plane.
    positions = np.asarray(
        [
            [-0.5, -0.25, 0.0],
            [0.0, 0.35, 0.0],
            [0.5, -0.25, 0.0],
        ],
        dtype=np.float32,
        order="C",
    )

    # One RGBA color per point. Alpha is 255, so every point is fully opaque.
    colors = np.asarray(
        [
            [0, 200, 255, 255],
            [255, 210, 80, 255],
            [255, 90, 110, 255],
        ],
        dtype=np.uint8,
        order="C",
    )

    # One size per point, measured in screen pixels.
    diameters = np.asarray([24.0, 36.0, 24.0], dtype=np.float32, order="C")

    # Create one point visual for all three points.
    visual = dvz.dvz_point(scene, 0)

    # Attach each array to the visual attribute with the same name used by the
    # point visual reference.
    dvz.dvz_visual_set_data(visual, "position", positions)
    dvz.dvz_visual_set_data(visual, "color", colors)
    dvz.dvz_visual_set_data(visual, "diameter_px", diameters)

    # Add the prepared visual to the panel so it will be drawn.
    dvz.dvz_panel_add_visual(panel, visual, None)
    ```

=== "C"

    ```c
    const uint32_t n = 3;

    /* Three points, each with x/y/z coordinates. z is 0, so the points lie in a
     * 2D plane. */
    const vec3 positions[3] = {
        {-0.5f, -0.25f, 0.0f},
        {+0.0f, +0.35f, 0.0f},
        {+0.5f, -0.25f, 0.0f},
    };

    /* One RGBA color per point. Alpha is 255, so every point is fully opaque. */
    const DvzColor colors[3] = {
        {0, 200, 255, 255},
        {255, 210, 80, 255},
        {255, 90, 110, 255},
    };

    /* One size per point, measured in screen pixels. */
    const float diameters[3] = {24.0f, 36.0f, 24.0f};

    /* Create one point visual for all three points. */
    DvzVisual* visual = dvz_point(scene, 0);

    /* Attach each array to the visual attribute with the same name used by the
     * point visual reference. */
    dvz_visual_set_data(visual, "position", positions, n);
    dvz_visual_set_data(visual, "color", colors, n);
    dvz_visual_set_data(visual, "diameter_px", diameters, n);

    /* Add the prepared visual to the panel so it will be drawn. */
    dvz_panel_add_visual(panel, visual, NULL);
    ```


## Choose attributes

Each visual family has its own attribute names and array shapes. A point visual commonly uses:

| Attribute | Meaning | Typical shape |
| --- | --- | --- |
| `position` | point coordinates | `(n, 3)` float values |
| `color` | one RGBA color per point | `(n, 4)` 8-bit values |
| `diameter_px` | point diameter in screen pixels | `(n,)` float values |

Other visual families use different attributes. An image visual needs image corners and texture
coordinates. A path visual needs ordered vertices. A mesh visual needs geometry and may also need
per-instance transforms.

Do not assume that an attribute exists for every family. When changing visual families, check the
[visual family reference](../reference/visual-families/index.md).


## Keep arrays aligned

Related per-item arrays should describe the same items in the same order. In the point example,
`positions[0]`, `colors[0]`, and `diameters[0]` all describe the first point.

For point visuals, these arrays should normally have the same item count:

```c
dvz_visual_set_data(visual, "position", positions, count);
dvz_visual_set_data(visual, "color", colors, count);
dvz_visual_set_data(visual, "diameter_px", diameters, count);
```

Use `dvz_visual_set_data_many()` in C when several attributes should be checked and uploaded
together:

```c
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = positions, .item_count = count},
    {.attr_name = "color", .data = colors, .item_count = count},
    {.attr_name = "diameter_px", .data = diameters, .item_count = count},
};
dvz_visual_set_data_many(visual, updates, 3);
```


## Group items into visuals

Prefer one visual that contains many related items over many small visuals with only a few items
each. For example, if 100 points belong to the same dataset and use the same point visual settings,
put them in one point visual with 100 positions, colors, and diameters.

Create a separate visual when there is a real difference in how the data should be drawn:

- a different visual family, such as points versus lines;
- a different panel;
- a different coordinate space or draw layer;
- a different material, alpha mode, or depth behavior;
- a different update schedule, such as static background data versus frequently changing data.

Style differences should stay inside one visual when they are ordinary attributes. Point color,
point size, marker symbol, and per-item transform are examples of styles that can usually vary
within one visual.


## Choose attachment options

Most visuals can use the default attachment:

```c
dvz_panel_add_visual(panel, visual, NULL);
```

The default means: draw in the panel's data coordinate space, let the panel controller affect the
visual, and use draw layer `0`.

Use an explicit `DvzVisualAttachDesc` only when the visual needs a non-default placement. For
example, this attaches a visual in view coordinates and draws it above layer `0` visuals:

```c
DvzVisualAttachDesc attach = dvz_visual_attach_desc();
attach.coord_space = DVZ_VISUAL_COORD_VIEW;
attach.z_layer = 1;
dvz_panel_add_visual(panel, visual, &attach);
```

The common attachment choices are:

| Field | Default | Use when |
| --- | --- | --- |
| `coord_space` | `DVZ_VISUAL_COORD_DATA` | Use a non-data coordinate space for overlays or view-fixed elements. |
| `z_layer` | `0` | Draw one visual in front of or behind another visual in the same panel. |
| `controller_mode` | `DVZ_CONTROLLER_APPLY` | Keep the default for ordinary data visuals. |

Set panel domains before attaching data-coordinate visuals when you want explicit data limits:

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);
dvz_panel_add_visual(panel, visual, NULL);
```

See [Use coordinate systems](coordinate-systems.md) before mixing data, view, and panel coordinate
spaces.


## Instance repeated meshes

Use mesh instancing when many objects share the same triangle geometry and differ only by placement
or orientation. Common cases include repeated cubes, repeated markers built from geometry, and many
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
or update schedules.


## Common mistakes

- Creating a visual but never adding it to a panel.
- Uploading arrays with different item counts for attributes that describe the same items.
- Reusing point attribute names on another visual family without checking that family's reference.
- Creating many small visuals when one visual could hold the same related items.
- Splitting by color or size when color or size can be an ordinary per-item attribute.
- Using view or panel coordinates with the default data-coordinate attachment.


## Next steps

- [Choose a visual family](choose-a-visual-family.md)
- [Update visual data](update-visual-data.md)
- [Transform visual data](transforms-and-scales.md)
- [Use coordinate systems](coordinate-systems.md)
- [Visual family reference](../reference/visual-families/index.md)

??? example "Complete and related examples"

    - Canonical complete example: [Point](../examples/gallery/visuals/visuals_point.md) - Source: `examples/c/visuals/point.c`
    - [Marker](../examples/gallery/visuals/visuals_marker.md) - Source: `examples/c/visuals/marker.c`
    - [Mesh](../examples/gallery/visuals/visuals_mesh.md) - Source: `examples/c/visuals/mesh.c`
    - [Mesh Instance Selection](../examples/gallery/features/features_selection_mesh_instances.md) - Source: `examples/c/features/selection_mesh_instances.c`
