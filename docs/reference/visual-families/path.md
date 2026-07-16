# Path

Connected polylines with optional screen-space stroke width, caps, joins, and subpaths.

Status: supported.
Backends: native; WebGPU live (`path`, `panzoom`).
Primitive: line strip or expanded screen-space stroke geometry.

## Preview And Links

[![Path](../../assets/gallery/v0.4/visuals/visuals_path.webp)](../../examples/gallery/visuals/visuals_path.md)

- Example: [Path](../../examples/gallery/visuals/visuals_path.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Segment](segment.md), [Vector](vector.md), [Primitive](primitive.md)

## Use When

Use path visuals for ordered samples that should read as connected curves, traces, boundaries, or
multi-subpath lines.

## Avoid When

Use [Segment](segment.md) for independent endpoint pairs, [Vector](vector.md) for arrows or vector
fields, or [Primitive](primitive.md) for raw line topology without path semantics.

## Item And Data Model

Create with `dvz_path(scene, flags)`. Here `N` counts ordered path points, not independent line
items. Without subpaths, all points form one open path. `dvz_path_set_subpaths()` supplies ordered
point counts whose sum must equal `N` when the frame is emitted.

If `stroke_width_px` is absent, the visual uses primitive line-strip rendering. If it is present,
the visual uses the expanded screen-space stroke path with configured caps and joins.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Ordered points in authored visual coordinates. | Dense or range upload. Changing count requires reconciling subpath lengths. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8 per point in the dense route. | Dense or range upload. |
| `stroke_width_px` | Optional; absence selects primitive line strip | `float[N]` | `float32`, `(N,)` | Logical-pixel width per point; presence selects stroked-path lowering. Use finite positive values. | Dense or range upload. |

Color supports configured per-item, constant, per-span, or per-group sources; stroke width supports
per-item or constant. These source modes require explicit configuration and are not dense-array
broadcasting.

## Constructor And Options

- Constructor: `dvz_path(scene, flags)`; current examples pass `flags = 0`.
- `dvz_path_set_subpaths()` copies `uint32_t[subpath_count]` point counts. Unset means one open
  subpath.
- `dvz_path_set_caps()` applies visual-wide caps to each open subpath.
- `dvz_path_set_join()` accepts round, bevel, or miter joins and a positive finite miter limit.
  For miter joins the limit is measured in local stroke widths; round and bevel ignore it.
- Closed subpaths, dashes, arrow caps, and per-item caps are deferred.

## Verified Usage Pattern

For the stroked route, upload `position`, `color`, and `stroke_width_px`, configure caps/joins and
optional subpaths, then attach the visual. See the complete
[C and Python example](../../examples/gallery/visuals/visuals_path.md).

## Picking And Probing

Retained attributes are point-based. Subpaths preserve grouping for rendering; use segment visuals
when picking should map directly to independent edges.

## Backend Notes

Native and WebGPU paths are active. Closed subpaths, dashes, arrow caps, and per-item cap
attributes are deferred.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/path.c` |
| Gallery | [Path](../../examples/gallery/visuals/visuals_path.md) |
| Build | `just example-c visuals/path` |
| Smoke | `./build/examples/c/visuals/path --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[axes](../../how-to/axes.md), [Segment](segment.md), [Vector](vector.md).
