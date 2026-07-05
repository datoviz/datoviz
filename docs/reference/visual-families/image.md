# Image

2D sampled field visual placed in panel coordinates.

Status: supported.
Backends: native; WebGPU live (`image`, `sampled-field`, `panzoom`).
Primitive: textured quad or retained image item quads.

## Preview And Links

[![Image](../../assets/gallery/v0.4/visuals/visual_image.webp)](../../examples/gallery/visuals/visual_image.md)

- Example: [Image](../../examples/gallery/visuals/visual_image.md)
- How-to: [Use sampled fields and textures](../../how-to/use-sampled-fields.md), [map scalar values with colormaps](../../how-to/use-colormaps.md)
- Related: [Pixel](pixel.md), [Labels](labels.md), [Volume](volume.md)

## Use When

Use image visuals for dense 2D scalar or color sampled fields, texture-backed panels, heatmaps, and
rasters where neighboring samples form one rectangular field.

## Avoid When

Use [Pixel](pixel.md) for sparse independently selectable square marks, [Labels](labels.md) for
integer categorical label fields, or [Volume](volume.md) for 3D sampled fields.

## Data Model

Create with `dvz_image(scene, flags)`. Bind a scene-owned `DvzSampledField` with
`dvz_visual_set_field(image, "field", field)`. The canonical example uploads four corner
positions, four texture coordinates, a scalar 2D field, and a color scale.

Set texture filtering with `dvz_image_set_sampling(image, DVZ_IMAGE_SAMPLING_LINEAR)` or
`dvz_image_set_sampling(image, DVZ_IMAGE_SAMPLING_NEAREST)`. Linear sampling is the default; nearest
sampling is intended for pixel-exact image/checkerboard rendering.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | sampled field bound to `"field"` plus placement attributes |
| Placement forms | four-corner `position` (`vec3[4]`) and `texcoords` (`vec2[4]`), or retained per-item `position` plus `extent` |
| Optional | `tex_rect`; `anchor`; scale bound to `"color"`; alpha mode; depth test; transform |

## Picking And Probing

Image visuals are the main surface for field probing. Use pick/probe helpers to map screen
positions to image coordinates and sampled field values.

## Backend Notes

Native and WebGPU paths are active for 2D sampled fields. Public examples should bind scene-owned
sampled fields explicitly with `dvz_visual_set_field()`.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/image.c` |
| Gallery | [Image](../../examples/gallery/visuals/visual_image.md) |
| Build | `just example-c visuals/image` |
| Smoke | `./build/examples/c/visuals/image --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Labels](labels.md), [Volume](volume.md).
