# Probe Image or Field Values

Read the data value under a cursor or selected coordinate.

## Task Workflow

Convert the pointer position to panel data coordinates, map that coordinate into the sampled field's
index or texture coordinate space, then display the value with a label, annotation, or overlay.

## Minimal Call Sequence

```c
/* Pointer position -> panel data coordinate. */
/* Data coordinate -> image texel or field sample. */
/* Update a label/readout visual with the sampled value. */
```

Use picking for item identity; use probing for field values.

## Canonical Examples

- Gallery: [Image Probe](../examples/gallery/features/image_probe.md)
- Source: `examples/c/features/image_probe.c`
- Gallery: [Label Probe](../examples/gallery/features/feature_probe_labels.md)
- Source: `examples/c/features/probe_labels.c`
- Gallery: [Linked Probe With Colorbar](../examples/gallery/showcases/linked_panels_probe_colorbar.md)
- Source: `examples/c/showcases/linked_probe_colorbar.c`

## Important Details

Probing depends on the same coordinate transform used for rendering. If the image is scaled,
translated, or texture-mapped onto a mesh, account for that transform before indexing the field.

## Common Mistakes

- Treating screen pixels as image indices after pan or zoom.
- Ignoring interpolation and sampling mode when reporting values.
- Turning a composed linked-probe showcase into copied starter code.

## See Also

- [Use sampled fields and textures](use-sampled-fields.md)
- [Pick items](pick-and-probe.md)
- [Add text, labels, and annotations](add-annotations.md)
