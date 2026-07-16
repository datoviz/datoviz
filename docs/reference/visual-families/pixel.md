# Pixel

Screen-space square sprites for per-item pixel-like marks.

Status: supported.
Backends: native; WebGPU live (`pixel`, `panzoom`).
Primitive: instanced screen-space quads.

## Preview And Links

[![Pixel](../../assets/gallery/v0.4/visuals/visuals_pixel.webp)](../../examples/gallery/visuals/visuals_pixel.md)

- Example: [Pixel](../../examples/gallery/visuals/visuals_pixel.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Point](point.md), [Marker](marker.md), [Image](image.md), [Labels](labels.md)

## Use When

Use pixel visuals for square marks with explicit pixel sizes, especially sparse raster samples or
selection overlays where each item remains an independently addressable mark.

## Avoid When

Use [Point](point.md) for circular marks, [Marker](marker.md) for symbolic glyphs, or
[Image](image.md) for dense regular sampled fields.

## Item And Data Model

Create with `dvz_pixel(scene, flags)`. One item is one independently addressable square sprite.
Upload `N` rows for each required attribute. This is not a sampled image: items need not lie on a
regular grid.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Authored visual coordinates; each row is the square center. | Dense or range upload. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8. Scalar `float[N]` is supported after selecting `DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32` and binding a color scale. | Set format before data; dense or range upload. |
| `pixel_size_px` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Square side length in logical pixels; use finite nonnegative values. | Dense or range upload. |
| `item_state` | Optional; absent means no per-item state payload | `uint32_t[N]` | `uint32`, `(N,)` | `DvzItemStateKind` bitfield for retained hover/selection styling. | Usually managed by interaction APIs; direct dense/range upload is supported. |

## Constructor And Options

- Constructor: `dvz_pixel(scene, flags)`; current examples pass `flags = 0`.
- Common visual-wide options include alpha mode, depth test, depth cue, transform, and a color
  scale. Pixel has no family-specific style setter.
- `color` and `pixel_size_px` support configured constant or per-group sources; dense arrays do not
  broadcast.

## Verified Usage Pattern

Upload `position`, `color`, and `pixel_size_px` together, then attach the visual to the panel. See
the complete [C and Python example](../../examples/gallery/visuals/visuals_pixel.md).

## Picking And Probing

Pixel visuals preserve item identity through the screen-space quad lowering. Use them when picking
or selection must report the original item index.

## Backend Notes

Native and WebGPU paths are active. WebGPU lowers each item to an instanced quad. The example
disables depth testing for a 2D panzoom panel.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/pixel.c` |
| Gallery | [Pixel](../../examples/gallery/visuals/visuals_pixel.md) |
| Build | `just example-c visuals/pixel` |
| Smoke | `./build/examples/c/visuals/pixel --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[select items](../../how-to/select-items.md), [Point](point.md), [Image](image.md).
