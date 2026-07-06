# Segment

Independent endpoint-pair strokes rendered as analytic screen-space line segments.

Status: supported.
Backends: native; WebGPU live (`segment`, `panzoom`).
Primitive: expanded screen-space stroked segments.

## Preview And Links

[![Segment](../../assets/gallery/v0.4/visuals/visual_segment.webp)](../../examples/gallery/visuals/visual_segment.md)

- Example: [Segment](../../examples/gallery/visuals/visual_segment.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Path](path.md), [Vector](vector.md), [Primitive](primitive.md)

## Use When

Use segment visuals for disconnected line pairs such as edge lists, rulers, tick marks, and short
annotations where each item has its own start and end point.

## Avoid When

Use [Path](path.md) for connected polylines with joins and subpaths, [Vector](vector.md) for
direction fields or arrows, or [Primitive](primitive.md) for raw GPU line topology.

## Data Model

Create with `dvz_segment(scene, flags)`. Upload one item per endpoint pair. Caps are visual-wide in
v0.4.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | `position_start` (`vec3`), `position_end` (`vec3`), `color` (RGBA8), `stroke_width_px` (`float`, pixels) |
| Optional | endpoint caps through `dvz_segment_set_caps()`; alpha mode; depth test; transform; visual-wide scale bindings |

## Picking And Probing

Picking and retained data are item-based: one segment item corresponds to one start/end pair.

## Backend Notes

Native and WebGPU paths are active. v0.4 caps include none, round, triangle-in,
triangle-out, square, and butt. Dashes and per-item cap attributes are deferred.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/segment.c` |
| Gallery | [Segment](../../examples/gallery/visuals/visual_segment.md) |
| Build | `just example-c visuals/segment` |
| Smoke | `./build/examples/c/visuals/segment --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Path](path.md), [Vector](vector.md), [Primitive](primitive.md).
