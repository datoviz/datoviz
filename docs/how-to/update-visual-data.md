# Update Visual Data

Change the data shown by an existing visual.

![Point positions changing while the existing visual remains on screen](../assets/gallery/v0.4/features/features_update_visual_data.webp)

Use this when the same plot should display new values: a time series advances, a simulation step
finishes, a slider changes a threshold, or a selection changes the colors of some items. In most
cases, keep the visual and update its data arrays. Do not rebuild the whole scene for every change.

## Basic workflow

Most updates follow the same shape:

1. Create the scene, figure, panel, and visual.
2. Upload the initial arrays for the visual attributes, such as positions, colors, or sizes.
3. Add the visual to a panel.
4. Open a window or create an offscreen target.
5. When your data changes, call the visual data update function again.
6. Let the next frame draw the updated visual.

For example, a point visual might have one array for `"position"`, one array for `"color"`, and one
array for `"diameter_px"`. Updating the visual means replacing one or more of those arrays, not
creating a new point visual for every point.

## Choose the update method

Choose the update method from what changed in your data:

| What changed | Use | When it is useful |
| --- | --- | --- |
| One complete attribute changed, and the item count stayed the same. | `dvz_visual_set_data()` | Simple updates such as moving points, changing colors, or changing point sizes. |
| Several attributes changed together, or the item count changed. | `dvz_visual_set_data_many()` | Safer when arrays depend on each other, because Datoviz checks the group before replacing the old data. |
| Only one contiguous range changed inside an existing attribute. | `dvz_visual_set_data_range()` | Useful for editing items 200 to 350 without uploading the whole color or position array again. |
| Visibility, transform, material, depth, or blending changed. | The matching visual setter. | These are visual settings, not data arrays. |
| Image or volume values changed. | Sampled-field APIs. | Use the field or texture path rather than treating each pixel or voxel as a separate item. |

## Replace one attribute

This is the simplest case. The visual already exists, and you replace one array:

Prerequisite: `visual` already has its initial point attributes. The result appears on the next
rendered frame. This is a fragment; see the complete
[Visual Data Update example](../examples/gallery/features/features_update_visual_data.md).

=== "Python"

    ```python
    import numpy as np
    import datoviz as dvz

    positions = np.asarray(positions, dtype=np.float32, order="C")
    colors = np.asarray(colors, dtype=np.uint8, order="C")
    dvz.dvz_visual_set_data(visual, "position", positions)
    dvz.dvz_visual_set_data(visual, "color", colors)
    ```

=== "C"

    ```c
    dvz_visual_set_data(visual, "position", pos, n);
    dvz_visual_set_data(visual, "color", color, n);
    ```

Here `n` is the number of items in the visual. For a point visual, that means the number of points.
For a segment visual, it means the number of segments. The attribute name, such as `"position"` or
`"color"`, must be supported by that visual family.

Datoviz copies the array when you call the function. If you later modify `pos` or `color` in your
own program, the visual does not change automatically. Call the update function again when you want
the new values to appear.

## Replace several attributes together

When the number of items changes, update all per-item arrays that share that item count. For a point
visual, positions, colors, and diameters usually all have one value per point. If you grow from
1,000 points to 1,200 points, those arrays need to agree on the new count.

```c
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = pos, .item_count = n},
    {.attr_name = "color", .data = color, .item_count = n},
    {.attr_name = "diameter_px", .data = diameter_px, .item_count = n},
};
dvz_visual_set_data_many(visual, updates, 3);
```

This form is also useful when several attributes change together even if the item count stays the
same. It keeps related updates in one place and avoids temporary mismatches between arrays.

In Python, the NumPy facade accepts a mapping and infers each item count:

```python
dvz.dvz_visual_set_data_many(
    visual, {"position": positions, "color": colors, "diameter_px": diameters}
)
```

## Update part of one attribute

If only a continuous slice of one existing attribute changed, update just that range:

```c
dvz_visual_set_data_range(visual, "color", first, color + first, count);
```

The equivalent NumPy range upload infers `count` from the contiguous slice:

```python
changed = np.asarray(colors[first:first + count], dtype=np.uint8, order="C")
dvz.dvz_visual_set_data_range(visual, "color", first, changed)
```

Use this after the attribute has already been fully allocated with `dvz_visual_set_data()` or
`dvz_visual_set_data_many()`. A range update edits existing data; it does not create a new
attribute, and it does not change the number of items.

Range updates are a good fit for hover or selection feedback when you know which consecutive items
changed. If the changed items are scattered throughout the array, replacing the full attribute may
be simpler and still fast enough.

## Animation and interaction

For animation, call the update from a timer, frame callback, or host event before the next frame is
drawn. The visual remains the same object; only its data changes.

For interaction, keep your application data and your visual data connected by a stable index or id.
For example, if item 37 in your application is point 37 in the visual, a pick result or selection
state can update the right color entry. If you reorder the visual data, update that mapping too.

## Keep related items in one visual

Keep the grouping chosen during initial upload; an update should not split one visual into many
objects. See [Group items into visuals](add-a-visual.md#group-items-into-visuals) for the
authoritative batching and separation rules.

## Details that matter

All dense per-item attributes on one visual must agree on item count. If a point visual has
`"position"`, `"color"`, and `"diameter_px"`, changing the number of points means updating all three
arrays to the same new count.

`dvz_visual_set_data_many()` is the safer choice for count changes because Datoviz validates the
whole group before replacing the old data. Use separate `dvz_visual_set_data()` calls when the item
count is stable or when only one independent attribute changes.

Updates affect later frames. If a frame has already been prepared for drawing, changing the visual
will affect a later frame, not the frame that was already handed to the runtime.

Image and volume data use sampled fields and textures. Keep the grid dimensions, format, and value
range explicit, and use the same scale for colorbars or probes. See [Use sampled fields and
textures](use-sampled-fields.md).

## Common mistakes

- Rebuilding the whole scene for every data change.
- Splitting related items into many tiny visuals instead of updating one grouped visual.
- Changing an attribute count without updating all dependent attributes.
- Editing your own array after upload and expecting Datoviz to notice automatically.
- Calling `dvz_visual_set_data_range()` before the full attribute exists.
- Using data uploads for visual settings such as visibility, transform, or material.
- Forgetting that pick or selection ids depend on a stable mapping between visual items and
  application data.

## See also

- [Animate a scene](animation.md)
- [Add visuals to a panel](add-a-visual.md)
- [Use sampled fields and textures](use-sampled-fields.md)
- [Select and highlight data](select-items.md)
- [Transform visual data](transforms-and-scales.md)
- [Profile rendering performance](profile-performance.md)

??? example "Complete and related examples"

    - Canonical complete example: [Visual Data Update](../examples/gallery/features/features_update_visual_data.md) - Source: `examples/c/features/update_visual_data.c`
    - [Partial Data Update](../examples/gallery/features/features_update_partial.md) - Source: `examples/c/features/update_partial.c`
    - [Visual Visibility](../examples/gallery/features/features_visibility.md) - Source: `examples/c/features/visibility.c`
