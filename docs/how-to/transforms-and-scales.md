# Transform Visual Data

Apply object transforms or user scales without rewriting source data.

## Task Workflow

Use panel domains for the view's data range. Use visual transforms when one visual needs a local
translation, scale, or rotation relative to the panel. Use user scales when an example explicitly
models non-default coordinate scaling.

## Minimal Call Sequence

```c
mat4 transform = {{0}};
/* Fill the affine transform matrix. */
dvz_visual_set_transform(visual, transform);
dvz_panel_add_visual(panel, visual, NULL);
```

Use the matrix setup pattern from `examples/c/features/visual_transform.c` when adapting this.

## Canonical Examples

- Gallery: [Visual Transform](../examples/gallery/features/feature_visual_transform.md)
- Source: `examples/c/features/visual_transform.c`
- Gallery: [User Scale](../examples/gallery/features/feature_user_scale.md)
- Source: `examples/c/features/user_scale.c`
- Gallery: [Reference Grid](../examples/gallery/features/feature_reference_grid.md)
- Source: `examples/c/features/reference_grid.c`

## Important Details

Transforms are retained scene objects. Keep them alive with the scene and update them through the
scene API rather than baking every camera or scale change into raw positions.

## Common Mistakes

- Using transforms to compensate for a wrong panel domain.
- Applying both data scaling and visual scaling without documenting the final units.
- Expecting WebGPU parity for every native transform example; check the manifest status.

## See Also

- [Use coordinate systems](coordinate-systems.md)
- [Configure cameras](configure-cameras.md)
- [Profile rendering performance](profile-performance.md)
