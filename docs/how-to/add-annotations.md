# Add Text, Labels, and Annotations

Place readable text in or near a data view.

## Task Workflow

Use text for fixed blocks, labels for many data-attached strings, and annotation readouts for
interactive values. Add the text visual or composite to the panel after deciding whether the text is
data-space, screen-space, or overlay content.

## Minimal Call Sequence

```c
DvzVisual* labels = dvz_labels(scene, 0);
dvz_visual_set_data(labels, "position", pos, n);
dvz_visual_set_data(labels, "text", text, n);
dvz_panel_add_visual(panel, labels, NULL);
```

Check the canonical source for the current string storage and attribute names.

## Canonical Examples

- Gallery: [Text Block](../examples/gallery/features/feature_text_block.md)
- Source: `examples/c/features/text_block.c`
- Gallery: [Labels](../examples/gallery/visuals/visual_labels.md)
- Source: `examples/c/visuals/labels.c`
- Gallery: [Annotation Readout](../examples/gallery/features/annotation_readout.md)
- Source: `examples/c/features/annotation_readout.c`

## Important Details

Text has layout and readability constraints that geometry does not. Keep labels sparse enough to
remain legible and prefer probe readouts for dense data.

## Common Mistakes

- Treating labels as a substitute for picking or probing dense fields.
- Forgetting that font/glyph resources may have backend-specific limits.
- Baking changing readout text into a full scene rebuild.

## See Also

- [Pick items](pick-and-probe.md)
- [Probe image or field values](probe-fields.md)
- [Add colorbars, scale bars, and legends](adornments.md)
