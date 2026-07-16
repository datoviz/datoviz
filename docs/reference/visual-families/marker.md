# Marker

Screen-space code-SDF symbols with per-item shape, fill, stroke, size, and rotation.

Status: supported.
Backends: native; WebGPU live (`marker`, `panzoom`).
Primitive: instanced screen-space quads.

## Preview And Links

[![Marker](../../assets/gallery/v0.4/visuals/visuals_marker.webp)](../../examples/gallery/visuals/visuals_marker.md)

- Example: [Marker](../../examples/gallery/visuals/visuals_marker.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Point](point.md), [Pixel](pixel.md), [Glyph](glyph.md), [Text](text.md)

## Use When

Use marker visuals for scatter marks that need categorical symbol shapes, per-item rotation, or
stroke/fill styling while keeping each mark screen-sized.

## Avoid When

Use [Point](point.md) for simple circular marks, [Pixel](pixel.md) for square cells, or
[Glyph](glyph.md) for low-level font-atlas glyph quads.

## Item And Data Model

Create with `dvz_marker(scene, flags)`. One item is one screen-space symbol. `position` is the
center of its bounding box and `diameter_px` is the box width and height. Built-in code-SDF symbols
and homogeneous bitmap, SDF, or MSDF symbol sets use the same dense item model.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Authored visual coordinates; marker center. | Dense or range upload. Upload before using `dvz_marker_set_symbol()`. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8 fill color. Scalar color format is not documented for marker. | Dense or range upload. |
| `diameter_px` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Bounding-box diameter in logical pixels; screen-stable under pan/zoom. | Dense or range upload. |
| `angle` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Radians, positive counter-clockwise in rendered y-up coordinates. | Dense or range upload. |
| `shape` or `symbol` | Required; aliases for the same retained data | `uint32_t[N]` | `uint32`, `(N,)` | Built-in `DvzSymbolBuiltin`/`DvzMarkerShape` value or an id from the bound symbol set. Mixed source encodings in one array are rejected. | Dense or range upload; `dvz_marker_set_symbol()` fills all existing items with one built-in symbol. |
| `item_state` | Optional; absent means no per-item state payload | `uint32_t[N]` | `uint32`, `(N,)` | `DvzItemStateKind` bitfield for retained hover/selection styling. | Usually interaction-managed; direct dense/range upload is supported. |

Marker atlas `tex_rect` data is generated when symbol data changes; user code should upload
`shape`/`symbol`, not construct atlas rectangles.

## Constructor And Options

- Constructor: `dvz_marker(scene, flags)`; current examples pass `flags = 0`.
- `dvz_marker_style()` returns the explicit default filled/no-stroke style.
- `dvz_marker_set_style()` configures visual-wide fill/stroke/outline aspect, edge RGBA8, and
  logical-pixel stroke width.
- Bind a scene-owned `DvzSymbolSet` with `dvz_marker_set_symbols()`. Texture-backed arrays must be
  homogeneous bitmap, SDF, or MSDF in the current slice.
- Common options include alpha mode, depth test, and transform.

`angle` rotates counter-clockwise in rendered y-up coordinates around the bounding-box center.
`DVZ_MARKER_SHAPE_TRIANGLE` at `angle = 0` has marker-local vertices `(-1, -1)`, `(+1, -1)`,
and `(0, +1)`, matching Matplotlib's `^` marker up to scale.

## Picking And Probing

Marker picking reports source marker items. Stroke and shape styling do not create additional user
items.

## Verified Usage Pattern

Upload all five required arrays together using either `shape` or `symbol`, then attach the visual.
See the complete [C and Python example](../../examples/gallery/visuals/visuals_marker.md).

## Backend Notes

Native and WebGPU paths are active. Marker size is in pixels, so zoom changes item positions but not
screen diameter_px unless the data is updated.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/marker.c` |
| Gallery | [Marker](../../examples/gallery/visuals/visuals_marker.md) |
| Build | `just example-c visuals/marker` |
| Smoke | `./build/examples/c/visuals/marker --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use panzoom](../../how-to/use-panzoom.md), [Point](point.md), [Glyph](glyph.md).
