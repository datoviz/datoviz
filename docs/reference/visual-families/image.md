# Image

2D sampled field visual placed in panel coordinates.

Status: supported.
Backends: native; WebGPU live (`image`, `sampled-field`, `panzoom`).
Primitive: textured quad or retained image item quads.

## Preview And Links

[![Image](../../assets/gallery/v0.4/visuals/visuals_image.webp)](../../examples/gallery/visuals/visuals_image.md)

- Example: [Image](../../examples/gallery/visuals/visuals_image.md)
- How-to: [Use sampled fields and textures](../../how-to/use-sampled-fields.md), [map scalar values with colormaps](../../how-to/use-colormaps.md)
- Related: [Pixel](pixel.md), [Labels](labels.md), [Volume](volume.md)

## Use When

Use image visuals for dense 2D scalar or color sampled fields, texture-backed panels, heatmaps, and
rasters where neighboring samples form one rectangular field.

## Avoid When

Use [Pixel](pixel.md) for sparse independently selectable square marks, [Labels](labels.md) for
integer categorical label fields, or [Volume](volume.md) for 3D sampled fields.

## Item And Data Model

Create with `dvz_image(scene, flags)`, bind one scene-owned 2D `DvzSampledField` to `"field"`, and
choose exactly one placement form:

1. Quad-corner form: four `position` and four `texcoords` rows in triangle-strip order.
2. Retained rectangle form: `N` matching `position`/`extent` rows in authored coordinates, or `N`
   matching `position_px`/`extent_px` rows in logical pixels. Do not mix the two rectangle spaces.

## Attribute And Resource Contract

| Attribute/resource | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| field slot `"field"` | Required; one field per visual | `DvzSampledField*` | Handle from `dvz_sampled_field_from_array()` | 2D sampled field. Common inferred arrays are scalar `float32`/`uint8` `(H, W)` or color `uint8` `(H, W, 4)`. | Bind with `dvz_visual_set_field()`; update samples with sampled-field region APIs. |
| `position` | Required in corner or authored-rectangle form | corner: `vec3[4]`; rectangle: `vec3[N]` | `float32`, `(4, 3)` or `(N, 3)` | Corner vertices or rectangle reference position in authored visual coordinates. | Dense/range upload; rectangle count must match `extent`. |
| `texcoords` | Required only in corner form | `vec2[4]` | `float32`, `(4, 2)` | Normalized texture UVs matching the four corner rows. | Dense or range upload. |
| `extent` | Required in authored-rectangle form | `vec2[N]` | `float32`, `(N, 2)` | Rectangle width/height in the same authored coordinate basis as `position`. | Dense/range upload; count must match `position`. |
| `position_px` | Required in pixel-rectangle form | `vec3[N]` | `float32`, `(N, 3)` | Rectangle reference position in logical pixels; z is retained. | Dense/range upload; count must match `extent_px`. |
| `extent_px` | Required in pixel-rectangle form | `vec2[N]` | `float32`, `(N, 2)` | Rectangle width/height in logical pixels. | Dense or range upload. |
| `anchor` | Optional; default `(0, 0)` centers each retained rectangle | `vec2[N]` | `float32`, `(N, 2)` | Normalized anchor; generated corners use `-1` and `+1` as opposing edges. | Dense or range upload; rectangle form only. |
| `tex_rect` | Optional; default `(0, 0, 1, 1)` | `vec4[N]` | `float32`, `(N, 4)` | Per-item normalized `(u0, v0, u1, v1)` atlas rectangle. | Dense or range upload; rectangle form only. |

A continuous scalar field needs a scale bound to `"color"`; a direct color field uses its field
color role without a scale.

## Constructor And Options

- Constructor: `dvz_image(scene, flags)`; examples pass `flags = 0`.
- Sampling defaults to `DVZ_IMAGE_SAMPLING_LINEAR`. Use `dvz_image_set_sampling()` with
  `DVZ_IMAGE_SAMPLING_NEAREST` for pixel-exact filtering.
- Common options include alpha mode, depth test, transform, and color scale.

## Verified Usage Pattern

The [scalar C/Python example](../../examples/gallery/visuals/visuals_image.md) demonstrates a
continuous field and scale. The [RGBA C example](../../examples/gallery/visuals/visuals_image_rgba.md)
demonstrates a direct color field. Both use the four-corner placement form.

## Picking And Probing

Image visuals are the main surface for field probing. Use pick/probe helpers to map screen
positions to image coordinates and sampled field values.

## Backend Notes

Native and WebGPU paths are active for 2D sampled fields. Public examples should bind sampled
fields explicitly with `dvz_visual_set_field()`.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/image.c` |
| Gallery | [Image](../../examples/gallery/visuals/visuals_image.md) |
| Build | `just example-c visuals/image` |
| Smoke | `./build/examples/c/visuals/image --png` |
| Validation | `smoke+screenshot` |

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/image_rgba.c` |
| Gallery | [RGBA Image](../../examples/gallery/visuals/visuals_image_rgba.md) |
| Build | `just example-c visuals/image_rgba` |
| Smoke | `./build/examples/c/visuals/image_rgba --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Labels](labels.md), [Volume](volume.md).
