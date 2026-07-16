# Segment

Independent endpoint-pair strokes rendered as analytic screen-space line segments.

Status: supported.
Backends: native; WebGPU live (`segment`, `panzoom`).
Primitive: expanded screen-space stroked segments.

## Preview And Links

[![Segment](../../assets/gallery/v0.4/visuals/visuals_segment.webp)](../../examples/gallery/visuals/visuals_segment.md)

- Example: [Segment](../../examples/gallery/visuals/visuals_segment.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Path](path.md), [Vector](vector.md), [Primitive](primitive.md)

## Use When

Use segment visuals for disconnected line pairs such as edge lists, rulers, tick marks, and short
annotations where each item has its own start and end point.

## Avoid When

Use [Path](path.md) for connected polylines with joins and subpaths, [Vector](vector.md) for
direction fields or arrows, or [Primitive](primitive.md) for raw GPU line topology.

## Item And Data Model

Create with `dvz_segment(scene, flags)`. One item is one independent start/end pair. All four dense
arrays have `N` rows and describe the same `N` segments. Caps are visual-wide, not per item.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position_start` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Authored visual coordinates; start endpoint. | Dense or range upload. |
| `position_end` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Same space as `position_start`; end endpoint. | Dense or range upload. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8, one color per segment in the dense route. Scalar color is deferred. | Dense or range upload. |
| `stroke_width_px` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Stroke width in logical pixels; use finite positive values for visible strokes. | Dense or range upload. |

`color` and `stroke_width_px` may use an explicitly configured constant source. A one-row dense
array is still one segment, not a broadcast value.

## Constructor And Options

- Constructor: `dvz_segment(scene, flags)`; current examples pass `flags = 0`.
- Caps default to `DVZ_SEGMENT_CAP_BUTT` at both ends. Change them with
  `dvz_segment_set_caps()`; supported values are none, round, triangle-in, triangle-out, square,
  and butt.
- Common options include alpha mode, depth test, and transform. Dashes, scalar color, arrow caps,
  and per-item caps are deferred.

## Verified Usage Pattern

Upload the two endpoint arrays, color, and width together, optionally set caps, then attach the
visual. See the complete [C and Python example](../../examples/gallery/visuals/visuals_segment.md).

## Picking And Probing

Picking and retained data are item-based: one segment item corresponds to one start/end pair.

## Backend Notes

Native and WebGPU paths are active. Each source item lowers to expanded screen-space stroke
geometry while retaining the source segment identity.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/segment.c` |
| Gallery | [Segment](../../examples/gallery/visuals/visuals_segment.md) |
| Build | `just example-c visuals/segment` |
| Smoke | `./build/examples/c/visuals/segment --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Path](path.md), [Vector](vector.md), [Primitive](primitive.md).
