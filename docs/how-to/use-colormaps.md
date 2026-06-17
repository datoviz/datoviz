# Map Scalar Values with Colormaps

Convert scalar data into colors and expose the scale to readers.

## Task Workflow

Choose the scalar range, choose a colormap, upload the mapped colors or sampled-field data, then add
a colorbar when the visual result needs interpretation.

## Minimal Call Sequence

```c
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", rgba, n);
dvz_panel_add_visual(panel, visual, NULL);
```

For sampled fields, use the image or volume path shown in the field examples instead of manually
expanding every scalar to geometry.


## Important Details

Keep the scalar domain explicit. The colorbar should match the normalization used for the visual,
not just the colormap name.

## Common Mistakes

- Remapping colors after every pan or zoom instead of only when scalar values or normalization
  change.
- Showing a colorbar for categorical colors that have no scalar order.
- Mixing premultiplied and straight alpha expectations in transparent colormaps.

## See Also

- [Use sampled fields and textures](use-sampled-fields.md)
- [Add colorbars, scale bars, and legends](adornments.md)
- [Control depth, blending, and transparency](rendering-techniques.md)

??? example "Related examples"

    - Gallery: [Scalar Color Scale](../examples/gallery/features/colormap_scale.md)
    - Source: `examples/c/features/colormap_scale.c`
    - Gallery: [Colorbar](../examples/gallery/features/colorbar.md)
    - Source: `examples/c/features/colorbar.c`
    - Gallery: [2D Sampled Field](../examples/gallery/features/feature_sampled_field_2d.md)
    - Source: `examples/c/features/sampled_field_2d.c`
