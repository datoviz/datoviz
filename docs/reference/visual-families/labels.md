# Labels

Integer sampled-field visual rendered through categorical label scale metadata.

Status: supported.
Backends: native; WebGPU live (`labels`, `categorical-scale`, `panzoom`).
Primitive: textured quad or retained label item quads.

## Preview And Links

[![Labels](../../assets/gallery/v0.4/visuals/visuals_labels.webp)](../../examples/gallery/visuals/visuals_labels.md)

- Example: [Labels](../../examples/gallery/visuals/visuals_labels.md)
- How-to: [Add text, labels, and annotations](../../how-to/add-annotations.md), [use sampled fields and textures](../../how-to/use-sampled-fields.md)
- Related: [Image](image.md), [Text](text.md), [Pixel](pixel.md)

## Use When

Use labels visuals for segmentation masks, categorical rasters, and integer ID fields where values
map to category colors rather than continuous colormaps.

## Avoid When

Use [Image](image.md) for continuous scalar or color sampled fields, [Text](text.md) for semantic
text annotations, or [Pixel](pixel.md) for sparse categorical marks.

## Item And Data Model

Create with `dvz_labels(scene, flags)`. Bind an integer label-semantic sampled field to `"field"`
and a categorical scale to `"labels"`. The documented placement route uses `N` matching
`position`/`extent` rectangles. Each rectangle displays the bound field or its `tex_rect` subset;
the common case has `N = 1`.

## Attribute And Resource Contract

| Attribute/resource | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| field slot `"field"` | Required | `DvzSampledField*` | Handle from `dvz_sampled_field_from_array()` | Integer label-semantic field. Python commonly uses `int32` with explicit `R32_SINT` or `uint32` inferred/explicit as `R32_UINT`; 2D arrays are `(H, W)`, 3D arrays `(D, H, W)`. | Bind with `dvz_visual_set_field()`; update samples through sampled-field region APIs. |
| scale slot `"labels"` | Required by documented categorical route | `DvzScale*` | ctypes scale handle | Must be a categorical scale; continuous scales and slot `"color"` are rejected. | `dvz_visual_set_scale()`. |
| `position` | Required | `vec3[N]` | `float32`, `(N, 3)` | Rectangle reference position in authored visual coordinates. | Dense/range upload; count must match `extent`. |
| `extent` | Required | `vec2[N]` | `float32`, `(N, 2)` | Rectangle width/height in the same authored coordinate basis as `position`. | Dense or range upload. |
| `anchor` | Optional; default `(0,0)` centers the rectangle | `vec2[N]` | `float32`, `(N, 2)` | Normalized anchor, using `-1` and `+1` for opposing edges. | Dense or range upload. |
| `tex_rect` | Optional; default `(0,0,1,1)` | `vec4[N]` | `float32`, `(N, 4)` | Normalized atlas bounds `(u0,v0,u1,v1)`. | Dense or range upload. |

## Constructor And Options

- Constructor: `dvz_labels(scene, flags)`; examples pass `flags = 0`. It defaults to alpha
  blending with depth testing disabled.
- State defaults are opacity `1`, transparent background id `0`, no selection/hidden ids, boundary
  disabled with width `1` and white color, fallback seed `0`, Z slice axis, and slice position
  `0.5`.
- Opacity and slice position must be finite in `[0,1]`. Boundary width must be finite and
  nonnegative. Up to `DVZ_LABELS_MAX_HIDDEN` category IDs may be hidden.
- The 3D field route is slice-based and uses `dvz_labels_set_slice_axis()` plus
  `dvz_labels_set_slice_position()`.

## Verified Usage Pattern

Create an integer sampled field and categorical scale, upload one `position`/`extent` rectangle,
bind both resources, then configure presentation state. See the complete
[C and Python example](../../examples/gallery/visuals/visuals_labels.md).

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
| Gallery | [Labels](../../examples/gallery/visuals/visuals_labels.md) |
| Build | `just example-c visuals/labels` |
| Smoke | `./build/examples/c/visuals/labels --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Image](image.md).
