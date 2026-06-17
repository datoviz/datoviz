# Select and Highlight Data

Show which items are selected without rebuilding the scene.

## Task Workflow

Use picking or application state to compute selected ids, then update a visual attribute or a
separate highlight visual. Keep the base visual stable and make selection a small retained update.

## Minimal Call Sequence

```c
/* selected[id] -> color, size, visibility, or highlight geometry */
dvz_visual_set_data(visual, "color", color, n);
```

For dense point or pixel data, a color or alpha update is usually cheaper than creating one visual
per selected item.

## Canonical Examples

- Gallery: [Pixel Selection](../examples/gallery/features/feature_selection_pixel.md)
- Source: `examples/c/features/selection_pixel.c`
- Gallery: [Sphere Selection](../examples/gallery/features/feature_selection_sphere.md)
- Source: `examples/c/features/selection_sphere.c`
- Gallery: [Mesh Instance Selection](../examples/gallery/features/feature_selection_mesh_instances.md)
- Source: `examples/c/features/selection_mesh_instances.c`

## Important Details

Selection state belongs to the application. Datoviz renders the highlight state you upload; it does
not own your semantic selection model.

## Common Mistakes

- Destroying and recreating visuals for every click.
- Losing the mapping between pick ids and application ids after sorting data.
- Highlighting with alpha while depth and blending settings hide the selected item.

## See Also

- [Pick items](pick-and-probe.md)
- [Update visual data](update-visual-data.md)
- [Control depth, blending, and transparency](rendering-techniques.md)
