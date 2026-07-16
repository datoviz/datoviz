# Vector

Arrow and vector-field visual lowered through the retained path/segment stroke machinery.

Status: supported.
Backends: native; WebGPU live (`vector`, `path`, `panzoom`).
Primitive: screen-space stroked segments or paths.

## Preview And Links

[![Vector](../../assets/gallery/v0.4/visuals/visuals_vector.webp)](../../examples/gallery/visuals/visuals_vector.md)

- Example: [Vector](../../examples/gallery/visuals/visuals_vector.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Segment](segment.md), [Path](path.md), [Primitive](primitive.md)

## Use When

Use vector visuals for displacement fields, arrows, and curved vector annotations where each item
has a direction or where a path needs arrow-like caps.

## Avoid When

Use [Segment](segment.md) for plain endpoint pairs, [Path](path.md) for non-directional curves, or
[Primitive](primitive.md) for raw line topology.

## Item And Data Model

Create with `dvz_vector(scene, flags)`. The presence of a dense `vector` attribute selects straight
mode: each of the `N` rows is one vector item. If `vector` is absent, `position`, `color`, and
`stroke_width_px` are interpreted as `N` ordered curved-path points. Optional subpath counts must
sum to `N` at emission.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required in both modes; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Straight: vector anchor in authored visual coordinates. Curved: ordered path point. | Dense or range upload. |
| `vector` | Required for straight mode; must be absent for curved mode | `vec3[N]` | `float32`, `(N, 3)` | Displacement in the same authored coordinate basis as `position`; multiplied by style `scale`. | Dense or range upload. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8. Scalar color is not documented for vector. | Dense or range upload. |
| `stroke_width_px` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Logical-pixel stroke width; use finite positive values. | Dense or range upload. |

Color and stroke width support explicitly configured constant sources. Dense one-row arrays do not
broadcast.

## Constructor And Options

- Constructor: `dvz_vector(scene, flags)`; current examples pass `flags = 0`.
- `dvz_vector_style()` returns the defaults: scale `1`, tail anchor, no start cap,
  triangle-out end cap, round join, and miter limit `4`.
- `dvz_vector_set_style()` sets finite scale, tail/center/head anchor, endpoint caps, join, and a
  positive finite miter limit. Pass `NULL` in C to restore defaults.
- `dvz_vector_set_subpaths()` applies only to curved mode; lengths are `uint32_t` and must sum to
  the point count.

## Verified Usage Pattern

For straight vectors, upload all four attributes and optionally modify the descriptor returned by
`dvz_vector_style()`. See the complete
[C and Python example](../../examples/gallery/visuals/visuals_vector.md); the C source also contains
the verified curved-vector route.

## Picking And Probing

Straight vector lowering preserves source item identity. Curved vector data is path-point based,
with subpaths used for grouping.

## Backend Notes

Native and WebGPU paths are active. The canonical example demonstrates both straight and curved
vectors.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/vector.c` |
| Gallery | [Vector](../../examples/gallery/visuals/visuals_vector.md) |
| Build | `just example-c visuals/vector` |
| Smoke | `./build/examples/c/visuals/vector --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Path](path.md), [Segment](segment.md).
