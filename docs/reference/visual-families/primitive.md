# Primitive

Raw point, line, and triangle topology visual with built-in shaders.

Status: supported.
Backends: native; WebGPU live (`primitive`).
Primitive: selected by `DvzPrimitiveTopology` at construction.

## Preview And Links

[![Primitive](../../assets/gallery/v0.4/visuals/visual_primitive.webp)](../../examples/gallery/visuals/visual_primitive.md)

- Example: [Primitive](../../examples/gallery/visuals/visual_primitive.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Mesh](mesh.md), [Path](path.md), [Segment](segment.md)

## Use When

Use primitive visuals when your data is already GPU-style geometry and you want direct control over
topology without mesh helpers or screen-space stroke expansion.

## Avoid When

Use [Mesh](mesh.md) for retained triangle meshes with geometry helpers, [Path](path.md) for
stroked polylines, or [Segment](segment.md) for independent analytic line segments.

## Data Model

Create with `dvz_primitive(scene, topology, flags)`. Topology is fixed at construction. Upload
vertex attributes and, when needed, bind or upload an index buffer.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | `position` (`vec3`), `color` (RGBA8) |
| Optional | `normal` (`vec3`); `"index"` buffer through `dvz_visual_set_index_data()` or `dvz_visual_set_buffer()`; material; alpha mode; depth test; transform; visual-wide scale bindings |

## Picking And Probing

Primitive visuals retain source attributes and can expose bounds. Picking follows the submitted
primitive topology and index data; use higher-level visuals when item identity should mean
"segment", "path", or "mesh instance".

## Backend Notes

Native and WebGPU paths are active for the canonical triangle examples. The example demonstrates
triangle list, strip, and fan setup.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/primitive.c` |
| Gallery | [Primitive](../../examples/gallery/visuals/visual_primitive.md) |
| Build | `just example-c visuals/primitive` |
| Smoke | `./build/examples/c/visuals/primitive --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Mesh](mesh.md), [Path](path.md), [Segment](segment.md).
