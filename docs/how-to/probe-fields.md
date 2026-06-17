# Probe Image or Field Values

Read the data value under a cursor or selected coordinate.

## Task Workflow

Convert the pointer position to panel data coordinates, map that coordinate into the sampled field's
index or texture coordinate space, then display the value with a label, annotation, or overlay.

## Minimal Workflow

1. Convert pointer position to panel data coordinates.
2. Convert data coordinates to the image texel, field index, or texture coordinate used by the
   rendered visual.
3. Read the sampled value through the feature example's probe path.
4. Update a label or readout visual with the sampled value.

Use picking for item identity; use probing for field values.

For an image with a colorbar and cursor readout, keep the sampled field, scalar normalization,
colorbar range, and probe coordinate transform synchronized.


## Important Details

Probing depends on the same coordinate transform used for rendering. If the image is scaled,
translated, or texture-mapped onto a mesh, account for that transform before indexing the field.

## Common Mistakes

- Treating screen pixels as image indices after pan or zoom.
- Ignoring interpolation and sampling mode when reporting values.
- Turning a composed linked-probe showcase into copied starter code.
- Showing a colorbar with a range that differs from the field normalization.

## See Also

- [Use sampled fields and textures](use-sampled-fields.md)
- [Pick items](pick-and-probe.md)
- [Add text, labels, and annotations](add-annotations.md)

??? example "Related examples"

    - Gallery: [Image Probe](../examples/gallery/features/image_probe.md)
    - Source: `examples/c/features/image_probe.c`
    - Gallery: [Label Probe](../examples/gallery/features/feature_probe_labels.md)
    - Source: `examples/c/features/probe_labels.c`
    - Gallery: [Linked Probe With Colorbar](../examples/gallery/showcases/linked_panels_probe_colorbar.md)
    - Source: `examples/c/showcases/linked_probe_colorbar.c`
