# Update Visual Data

Change a retained visual after its first upload.

## Task Workflow

Keep the same visual object, replace the attributes that changed, and let the next frame draw the
new GPU data. Use partial updates only when the example or API path supports the affected visual and
attribute.

Keep batching intact while updating. Update the arrays of an existing visual whenever possible
instead of destroying the visual or creating one visual per changed item. Datoviz benefits most
when each retained visual contains many items that can be uploaded and drawn together.

## Minimal Call Sequence

```c
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
```

For animation, call the update from a timer, frame callback, or host event path before the next
render.


## Important Details

Datoviz retains the visual object and its GPU resources. You should update attributes through the
visual API instead of destroying and recreating the visual every frame.

If only a subset changes, prefer the supported range or partial-update path for that visual. If the
whole attribute changes, replacing one large array is still usually better than fragmenting the
scene into many small visuals.

## Common Mistakes

- Reallocating the whole scene for every update.
- Splitting frequently updated items into many tiny visuals instead of updating one batched visual.
- Changing an attribute count without updating all dependent attributes.
- Updating CPU arrays after upload and expecting the GPU copy to change automatically.

## See Also

- [Animate a scene](animation.md)
- [Add visuals to a panel](add-a-visual.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - Gallery: [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md)
    - Source: `examples/c/features/update_visual_data.c`
    - Gallery: [Partial Data Update](../examples/gallery/features/update_partial.md)
    - Source: `examples/c/features/update_partial.c`
    - Gallery: [Visual Visibility](../examples/gallery/features/feature_visibility.md)
    - Source: `examples/c/features/visibility.c`
