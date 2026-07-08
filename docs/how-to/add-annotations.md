# Add Text, Labels, and Annotations

Place readable text in or near a data view.

![Annotation Readout](../assets/gallery/v0.4/features/features_annotation_readout.webp)

## Task Workflow

Choose the text path from the job it needs to do. Use retained text for fixed panel text, labels for
categorical label fields, and annotation labels for data-anchored readouts. Add the object after
deciding whether its placement is screen-space, data-space, or tied to a query result.

| Need | Use | Placement model |
| --- | --- | --- |
| Fixed title, status line, or short overlay block | `dvz_text()` | Screen or panel anchored placement. |
| Integer label image, segmentation mask, or categorical field overlay | `dvz_labels()` | Same image placement attributes as image visuals. |
| One retained label at a data point or query location | `dvz_annotation_label()` | Data placement plus pixel offset. |

## Fixed Text Block

Create a retained `DvzText` when the text is a small number of strings owned by the scene. This is
the right path for corner labels, captions, and status text that should stay readable while the data
view changes.

```c
DvzText* text = dvz_text(panel, 0);

DvzTextStyle style = dvz_text_style();
style.size_px = 18.0f;
style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
style.color[0] = 230;
style.color[1] = 235;
style.color[2] = 242;
style.color[3] = 255;
dvz_text_set_style(text, &style);

DvzTextPlacement placement = dvz_text_placement();
placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
placement.position[0] = 18.0;
placement.position[1] = 22.0;
placement.text_anchor[0] = 0.0f;
placement.text_anchor[1] = 0.5f;
placement.has_text_anchor = true;
dvz_text_set_placement(text, &placement);

dvz_text_set_string(text, "Frame 42\nprobe: 0.731");
```

Use `dvz_text_set_string()` to update a changing readout. Do not rebuild the panel or recreate the
scene for every value change.

## Categorical Label Fields

Use `dvz_labels()` for label images or segmentation masks. A labels visual renders an integer
sampled field through a categorical scale; it is not a per-item string list.

```c
DvzVisual* labels = dvz_labels(scene, 0);

vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
vec2 extent[1] = {{1.0f, 1.0f}};
DvzVisualDataUpdate attrs[] = {
    {.attr_name = "position", .data = position, .item_count = 1},
    {.attr_name = "extent", .data = extent, .item_count = 1},
};
dvz_visual_set_data_many(labels, attrs, 2);

dvz_visual_set_field(labels, "field", label_field);
dvz_visual_set_scale(labels, "labels", categorical_scale);
dvz_labels_set_background(labels, 0);
dvz_labels_set_opacity(labels, 0.85f);
dvz_panel_add_visual(panel, labels, NULL);
```

Keep the label IDs, hidden/selected IDs, and categorical scale metadata synchronized. If the
underlying field changes, update the sampled field or field region rather than replacing the whole
visual.

## Data-Anchored Readout

Use `dvz_annotation_label()` for one or a few labels tied to data coordinates: highlighted peaks,
selected points, probe markers, and callouts. The placement is data-space with optional pixel
offsets so the text can sit near, not on top of, the point.

```c
DvzTextStyle style = dvz_text_style();
style.size_px = 20.0f;
style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;

DvzTextPlacement placement = dvz_text_placement();
placement.mode = DVZ_TEXT_PLACEMENT_DATA;
placement.position[0] = x;
placement.position[1] = y;
placement.position[2] = z;
placement.offset[0] = 24.0f;
placement.offset[1] = -18.0f;
placement.text_anchor[0] = 0.0f;
placement.text_anchor[1] = 0.5f;
placement.has_text_anchor = true;
placement.depth_test = false;

DvzAnnotation* readout = dvz_annotation_label(
    panel, &(DvzLabelDesc){DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
               .text = "peak"});
dvz_annotation_set_style(readout, &style);
dvz_annotation_set_placement(readout, &placement);
```

The annotation object is retained by the panel. Destroy it with `dvz_annotation_destroy()` when the
callout is no longer needed.


## Important Details

Text has layout and readability constraints that geometry does not. Keep labels sparse enough to
remain legible and prefer probe readouts for dense data.

Screen-space text is stable for overlays and status blocks. Data-space text moves with the data and
must be checked after pan, zoom, and 3D navigation. If the annotation should describe a query result,
store the application value separately and update only the text or annotation state when the query
changes.

Text rendering uses glyph atlas resources. Very large font sizes, many unique glyphs, or frequent
string churn can become resource-heavy. Reuse retained text or annotations when the placement and
style are stable.

## Common Mistakes

- Treating labels as a substitute for picking or probing dense fields.
- Treating `dvz_labels()` as a per-string label visual; it is for categorical sampled fields.
- Forgetting that font/glyph resources may have backend-specific limits.
- Baking changing readout text into a full scene rebuild.
- Mixing screen-space and data-space coordinates in one readout path.

## See Also

- [Pick items](pick-items.md)
- [Probe image or field values](probe-fields.md)
- [Add colorbars, scale bars, and legends](adornments.md)

??? example "Related examples"

    - [Text Block](../examples/gallery/features/features_text_block.md) - Source: `examples/c/features/text_block.c`
    - [Labels](../examples/gallery/visuals/visuals_labels.md) - Source: `examples/c/visuals/labels.c`
    - [Annotation Readout](../examples/gallery/features/features_annotation_readout.md) - Source: `examples/c/features/annotation_readout.c`
