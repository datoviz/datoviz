# Update Visual Data

Change a retained visual after its first upload.

## Task Workflow

Keep the same visual object, replace the attributes that changed, and let the next frame draw the
new GPU data. Use partial updates only when the example or API path supports the affected visual and
attribute.

## Minimal Call Sequence

```c
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
```

For animation, call the update from a timer, frame callback, or host event path before the next
render.

## Canonical Examples

- Gallery: [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md)
- Source: `examples/c/features/update_visual_data.c`
- Gallery: [Partial Data Update](../examples/gallery/features/update_partial.md)
- Source: `examples/c/features/update_partial.c`
- Gallery: [Visual Visibility](../examples/gallery/features/feature_visibility.md)
- Source: `examples/c/features/visibility.c`

## Important Details

Datoviz retains the visual object and its GPU resources. You should update attributes through the
visual API instead of destroying and recreating the visual every frame.

## Common Mistakes

- Reallocating the whole scene for every update.
- Changing an attribute count without updating all dependent attributes.
- Updating CPU arrays after upload and expecting the GPU copy to change automatically.

## See Also

- [Animate a scene](animation.md)
- [Add visuals to a panel](add-a-visual.md)
- [Profile rendering performance](profile-performance.md)
