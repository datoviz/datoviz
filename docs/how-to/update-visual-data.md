# Update Visual Data

Change a retained visual after its first upload.

![Visual Data Update](../assets/gallery/v0.4/features/feature_update_visual_data.webp)

## Task Workflow

Keep the same visual object, replace the attributes that changed, and let the next frame draw the
new GPU data. Use partial updates only when the example or API path supports the affected visual and
attribute.

Keep batching intact while updating. Update the arrays of an existing visual whenever possible
instead of destroying the visual or creating one visual per changed item. Datoviz benefits most
when each retained visual contains many items that can be uploaded and drawn together.

Choose the update path by what changed:

| Change | Use | Notes |
| --- | --- | --- |
| Replace one complete attribute without changing item count. | `dvz_visual_set_data()` | Best for simple updates such as positions, colors, sizes, or transforms stored as attributes. |
| Replace several complete attributes together. | `dvz_visual_set_data_many()` | Preferred when item count changes, because all updates are validated before existing payloads are replaced. |
| Replace a contiguous subset of one existing attribute. | `dvz_visual_set_data_range()` | Requires a previous full allocation with `dvz_visual_set_data()`; only the dirty range is uploaded on the next emit. |
| Change visibility, transform, material, depth, blending, or other visual state. | The specific visual setter. | Do not upload attribute data just to toggle retained visual state. |
| Update image or volume field contents. | Sampled-field APIs. | Keep grid dimensions, format, row pitch, and semantic role explicit. See [Use sampled fields and textures](use-sampled-fields.md). |

## Minimal Call Sequence

```c
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
```

When the item count changes, update all dense per-item attributes together:

```c
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = pos, .item_count = n},
    {.attr_name = "color", .data = color, .item_count = n},
    {.attr_name = "diameter_px", .data = diameter_px, .item_count = n},
};
dvz_visual_set_data_many(visual, updates, 3);
```

When only a contiguous range changes:

```c
dvz_visual_set_data_range(visual, "color", color + first, first, count);
```

For animation, call the update from a timer, frame callback, or host event path before the next
render.


## Important Details

Datoviz retains the visual object and its GPU resources. You should update attributes through the
visual API instead of destroying and recreating the visual every frame.

All dense per-item attributes configured on one visual must use the same item count. If a point
visual has `position`, `color`, and `diameter_px`, growing from `n` to `m` points means all three
attributes must be updated to `m` items.

`dvz_visual_set_data_many()` is the safer API for count changes because it validates the whole batch
before replacing any existing payload. Use separate `dvz_visual_set_data()` calls only when the
count is stable or when you are certain no dependent dense attribute is left behind.

`dvz_visual_set_data_range()` updates a contiguous subrange of an attribute that already exists. It
does not allocate a new attribute and it does not change the visual's item count.

Updates affect later frames. If a frame artifact has already been emitted, mutating the retained
visual changes a later artifact, not the one already handed to the runtime.

The input arrays are copied into retained visual storage before the update call returns. Mutating
the caller-owned arrays afterward does not change the visual until you call the visual API again.

If only a subset changes, prefer the supported range or partial-update path for that visual. If the
whole attribute changes, replacing one large array is still usually better than fragmenting the
scene into many small visuals.

## Common Mistakes

- Reallocating the whole scene for every update.
- Splitting frequently updated items into many tiny visuals instead of updating one batched visual.
- Changing an attribute count without updating all dependent attributes.
- Updating CPU arrays after upload and expecting the GPU copy to change automatically.
- Using `dvz_visual_set_data_range()` before the attribute has been fully allocated.
- Updating an already emitted frame artifact and expecting that artifact to change.
- Reuploading data to express visibility, selection, transform, or material changes that have
  dedicated retained-state setters.

## See Also

- [Animate a scene](animation.md)
- [Add visuals to a panel](add-a-visual.md)
- [Use sampled fields and textures](use-sampled-fields.md)
- [Select and highlight data](select-items.md)
- [Transform visual data](transforms-and-scales.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md) - Source: `examples/c/features/update_visual_data.c`
    - [Partial Data Update](../examples/gallery/features/update_partial.md) - Source: `examples/c/features/update_partial.c`
    - [Visual Visibility](../examples/gallery/features/feature_visibility.md) - Source: `examples/c/features/visibility.c`
