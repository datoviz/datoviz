# Image, Colorbar, and Probe Workflow

This composed workflow now lives in the Examples layer. Use the task guides below for reusable
pieces.

## Task Workflow

Render the field as an image, map values through a scalar range, add a matching colorbar, then use a
probe readout for cursor values.

## Minimal Call Sequence

```text
sampled field -> colormap -> image visual -> colorbar -> probe readout
```

## Canonical Examples

- Gallery: [Linked Probe With Colorbar](../examples/gallery/showcases/linked_panels_probe_colorbar.md)
- Source: `examples/c/showcases/linked_probe_colorbar.c`
- Gallery: [2D Sampled Field](../examples/gallery/features/feature_sampled_field_2d.md)
- Source: `examples/c/features/sampled_field_2d.c`
- Gallery: [Image Probe](../examples/gallery/features/image_probe.md)
- Source: `examples/c/features/image_probe.c`
- Gallery: [Colorbar](../examples/gallery/features/colorbar.md)
- Source: `examples/c/features/colorbar.c`

## Important Details

The showcase is useful for composition, but the minimal examples are better starting points for new
code.

## Common Mistakes

- Hand-copying a full showcase when only image probing is needed.
- Letting probe coordinates ignore the image transform.
- Showing a colorbar with a range that differs from the field normalization.

## See Also

- [Use sampled fields and textures](use-sampled-fields.md)
- [Map scalar values with colormaps](use-colormaps.md)
- [Probe image or field values](probe-fields.md)
