# Path

Connected polylines with optional screen-space stroke width, caps, joins, and subpaths.

Status: supported.
Backends: native; WebGPU live (`path`, `panzoom`).
Primitive: line strip or expanded screen-space stroke geometry.

## Preview And Links

[![Path](../../assets/gallery/v0.4/visuals/visual_path.webp)](../../examples/gallery/visuals/visual_path.md)

- Example: [Path](../../examples/gallery/visuals/visual_path.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Segment](segment.md), [Vector](vector.md), [Primitive](primitive.md)

## Use When

Use path visuals for ordered samples that should read as connected curves, traces, boundaries, or
multi-subpath lines.

## Avoid When

Use [Segment](segment.md) for independent endpoint pairs, [Vector](vector.md) for arrows or vector
fields, or [Primitive](primitive.md) for raw line topology without path semantics.

## Data Model

Create with `dvz_path(scene, flags)`. Upload ordered path points. If `stroke_width` is absent, the
visual uses the primitive line-strip pipeline; with `stroke_width`, it uses the scene path stroke
pipeline. Use `dvz_path_set_subpaths()` to split one visual into several open paths.

## Attributes

Required: `position` (`vec3`), `color` (RGBA8).

Optional: `stroke_width` (`float`, pixels), subpath lengths, caps through `dvz_path_set_caps()`,
joins through `dvz_path_set_join()`, alpha mode, depth test, transform, and visual-wide scale
bindings.

## Picking And Probing

Retained attributes are point-based. Subpaths preserve grouping for rendering; use segment visuals
when picking should map directly to independent edges.

## Backend Notes

Native and WebGPU paths are active. Closed subpaths, dashes, arrow caps, and per-item cap
attributes are deferred.

## Canonical Example

- Source: `examples/c/visuals/path.c`
- Gallery: [Path](../../examples/gallery/visuals/visual_path.md)
- Build: `just example-c visuals/path`
- Smoke: `./build/examples/c/visuals/path --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[axes](../../how-to/axes.md), [Segment](segment.md), [Vector](vector.md).
