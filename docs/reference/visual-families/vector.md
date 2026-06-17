# Vector

Arrow and vector-field visual lowered through the retained path/segment stroke machinery.

Status: supported.
Backends: native; WebGPU live (`vector`, `path`, `panzoom`).
Primitive: screen-space stroked segments or paths.

## Use When

Use vector visuals for displacement fields, arrows, and curved vector annotations where each item
has a direction or where a path needs arrow-like caps.

## Avoid When

Use [Segment](segment.md) for plain endpoint pairs, [Path](path.md) for non-directional curves, or
[Primitive](primitive.md) for raw line topology.

## Data Model

Create with `dvz_vector(scene, flags)`. Straight vectors use `position` plus `vector`; curved mode
omits `vector` and interprets positions as path points, optionally grouped with
`dvz_vector_set_subpaths()`. Style is visual-wide.

## Attributes

Required for straight vectors: `position` (`vec3` anchor), `vector` (`vec3` displacement), `color`
(RGBA8), `stroke_width` (`float`, pixels).

Required for curved vectors: `position`, `color`, `stroke_width`.

Optional: style through `dvz_vector_set_style()` (`scale`, anchor, caps, joins), subpath lengths,
alpha mode, depth test, transform, and visual-wide scale bindings.

## Picking And Probing

Straight vector lowering preserves source item identity. Curved vector data is path-point based,
with subpaths used for grouping.

## Backend Notes

Native and WebGPU paths are active. The canonical example demonstrates both straight and curved
vectors.

## Canonical Example

- Source: `examples/c/visuals/vector.c`
- Gallery: [Vector](../../examples/gallery/visuals/visual_vector.md)
- Build: `just example-c visuals/vector`
- Smoke: `./build/examples/c/visuals/vector --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Path](path.md), [Segment](segment.md).
