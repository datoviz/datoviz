# Primitive

Raw point, line, and triangle topology visual with built-in shaders.

Status: supported.
Backends: native; WebGPU live (`primitive`).
Primitive: selected by `DvzPrimitiveTopology` at construction.

## Preview And Links

[![Primitive](../../assets/gallery/v0.4/visuals/visuals_primitive.webp)](../../examples/gallery/visuals/visuals_primitive.md)

- Example: [Primitive](../../examples/gallery/visuals/visuals_primitive.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Mesh](mesh.md), [Path](path.md), [Segment](segment.md)

## Use When

Use primitive visuals when your data is already GPU-style geometry and you want direct control over
topology without mesh helpers or screen-space stroke expansion.

## Avoid When

Use [Mesh](mesh.md) for retained triangle meshes with geometry helpers, [Path](path.md) for
stroked polylines, or [Segment](segment.md) for independent analytic line segments.

## Item And Data Model

Create with `dvz_primitive(scene, topology, flags)`. Here `N` is the vertex count. The topology is
fixed at construction and determines how consecutive or indexed vertices form primitives. An
optional index array changes draw ordering without changing the vertex-attribute cardinality.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Vertex position in authored visual coordinates. | Dense or range upload. |
| `color` | Required for the documented primitive route; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8 vertex color. Scalar color is not documented for primitive. | Dense or range upload. |
| `normal` | Optional; absence selects the unlit/non-normal route | `vec3[N]` | `float32`, `(N, 3)` | Vertex normal used by material lighting; supply finite vectors in the same model basis. | Dense or range upload. |
| index data | Optional; not a named visual attribute | `uint32_t[I]` | `uint32`, `(I,)` | Each value indexes `[0, N)`. Cardinality and grouping must match the selected topology. | `dvz_visual_set_index_data()`; external buffers use `dvz_visual_set_buffer()` with role `"index"`. |

`color` supports configured constant or per-group sources. A one-row dense color array does not
broadcast.

## Constructor And Options

- Constructor: `dvz_primitive(scene, topology, flags)`; current examples pass `flags = 0`.
- Choose a `DvzPrimitiveTopology` before uploading data. Public constructors document point lists,
  line lists/strips, and triangle lists/strips; consult the generated
  [C types and primitive API](../c-api/visuals.md) for the current enum and exact signature.
- Common visual-wide options include material, alpha mode, depth test, depth cue, transform, and
  index data.

## Verified Usage Pattern

Create the primitive with an explicit topology, upload matching `position` and `color` arrays,
optionally add normals/indices, then attach it. See the complete
[C and Python example](../../examples/gallery/visuals/visuals_primitive.md).

## Picking And Probing

Primitive visuals retain source attributes and can expose bounds. Picking follows the submitted
primitive topology and index data; use higher-level visuals when item identity should mean
"segment", "path", or "mesh instance".

## Backend Notes

Native and WebGPU paths are active for the canonical triangle-list route. Verify the
[WebGPU subset](../webgpu-subset.md) before relying on another topology in the browser.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/primitive.c` |
| Gallery | [Primitive](../../examples/gallery/visuals/visuals_primitive.md) |
| Build | `just example-c visuals/primitive` |
| Smoke | `./build/examples/c/visuals/primitive --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[Mesh](mesh.md), [Path](path.md), [Segment](segment.md).
