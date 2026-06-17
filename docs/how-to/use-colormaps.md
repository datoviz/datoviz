# Map Scalar Values with Colormaps

Convert scalar data into colors and expose the scale to readers.

## Task Workflow

Choose the scalar range, choose a colormap, upload the mapped colors or sampled-field data, then add
a colorbar when the visual result needs interpretation.

## Minimal Call Sequence

```c
float values[N] = {0};

DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
DvzScale* scale = dvz_scale(
    scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
               .kind = DVZ_SCALE_CONTINUOUS,
               .label = "scalar value",
           });
dvz_scale_set_domain(scale, 0.0, 1.0);
dvz_scale_set_colormap(scale, colormap);

dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
dvz_visual_set_scale(visual, "color", scale);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", values, n);
dvz_panel_add_visual(panel, visual, NULL);
```

If you already have RGBA colors, upload them directly to `"color"` and skip the scale and colormap.
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

    - [Scalar Color Scale](../examples/gallery/features/colormap_scale.md) - Source: `examples/c/features/colormap_scale.c`
    - [Colorbar](../examples/gallery/features/colorbar.md) - Source: `examples/c/features/colorbar.c`
    - [2D Sampled Field](../examples/gallery/features/feature_sampled_field_2d.md) - Source: `examples/c/features/sampled_field_2d.c`
