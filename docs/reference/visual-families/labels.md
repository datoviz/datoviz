# Labels

Integer sampled-field visual rendered through categorical label scale metadata.

Status: supported.
Backends: native; WebGPU live (`labels`, `categorical-scale`, `panzoom`).
Primitive: textured quad or retained label item quads.

## Preview And Links

[![Labels](../../assets/gallery/v0.4/visuals/visual_labels.webp)](../../examples/gallery/visuals/visual_labels.md)

- Example: [Labels](../../examples/gallery/visuals/visual_labels.md)
- How-to: [Add text, labels, and annotations](../../how-to/add-annotations.md), [use sampled fields and textures](../../how-to/use-sampled-fields.md)
- Related: [Image](image.md), [Text](text.md), [Pixel](pixel.md)

## Use When

Use labels visuals for segmentation masks, categorical rasters, and integer ID fields where values
map to category colors rather than continuous colormaps.

## Avoid When

Use [Image](image.md) for continuous scalar or color sampled fields, [Text](text.md) for semantic
text annotations, or [Pixel](pixel.md) for sparse categorical marks.

## Data Model

Create with `dvz_labels(scene, flags)`. Bind an integer sampled field with
`dvz_visual_set_field(labels, "field", field)` and a categorical scale with
`dvz_visual_set_scale(labels, "labels", scale)`.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | sampled field bound to `"field"`, categorical scale bound to `"labels"`, and placement attributes |
| Placement forms | `position` plus `extent` for retained items; `anchor` and `tex_rect` are optional |
| Optional | opacity; transparent background label ID; selected label ID; hidden label IDs; boundary rendering; fallback-color seed; first-slice 3D slice axis/position; alpha mode; depth test; transform |

## Picking And Probing

Labels visuals support field-style probing. Use probe results as category IDs, then resolve labels
through the categorical scale used by the visual.

## Backend Notes

Native and WebGPU paths are active for the 2D categorical sampled-field route. The example disables
depth testing and enables alpha blending.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/labels.c` |
| Gallery | [Labels](../../examples/gallery/visuals/visual_labels.md) |
| Build | `just example-c visuals/labels` |
| Smoke | `./build/examples/c/visuals/labels --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Image](image.md).
