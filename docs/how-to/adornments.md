# Add Colorbars, Scale Bars, and Legends

Add visual context without turning the adornment into primary data.

## Task Workflow

Choose the adornment that explains the visual encoding: colorbars for scalar color, scale bars for
spatial units, legends for categorical encodings. Add it after the data visual and keep its range or
labels synchronized with the visual.

## Minimal Call Sequence

```c
/* Create the data visual first. */
dvz_panel_add_visual(panel, visual, NULL);
/* Add the colorbar, scale bar, or legend using the matching feature example. */
```

The exact helper depends on the adornment type.

## Canonical Examples

- Gallery: [Colorbar](../examples/gallery/features/colorbar.md)
- Source: `examples/c/features/colorbar.c`
- Gallery: [Scale Bar](../examples/gallery/features/scale_bar.md)
- Source: `examples/c/features/scalebar.c`
- Gallery: [Categorical Legend](../examples/gallery/features/feature_legend_categorical.md)
- Source: `examples/c/features/legend_categorical.c`

## Important Details

Adornments should describe the current visual encoding. If the scalar normalization, unit scale, or
category set changes, update the adornment at the same time.

## Common Mistakes

- Showing a colorbar for arbitrary RGBA colors.
- Letting a scale bar drift from the panel domain after zoom or unit changes.
- Using a long showcase as copied starter code instead of the minimal feature example.

## See Also

- [Map scalar values with colormaps](use-colormaps.md)
- [Add axes](axes.md)
- [Add text, labels, and annotations](add-annotations.md)
