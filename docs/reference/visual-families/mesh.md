# Mesh

Retained triangle mesh visual for indexed or non-indexed 3D geometry.

Status: supported.
Backends: native; WebGPU live (`mesh`, `indexed-geometry`, `arcball`).
Primitive: triangle list.

## Preview And Links

[![Mesh](../../assets/gallery/v0.4/visuals/visuals_mesh.webp)](../../examples/gallery/visuals/visuals_mesh.md)

- Example: [Mesh](../../examples/gallery/visuals/visuals_mesh.md)
- How-to: [Use lighting and materials](../../how-to/lighting-and-materials.md), [use 3D controllers](../../how-to/3d-navigation.md)
- Related: [Primitive](primitive.md), [Sphere](sphere.md), [Volume](volume.md)

## Use When

Use mesh visuals for surfaces, solids, loaded geometry, and instanced triangle data that should
participate in 3D camera navigation and material lighting.

For many repeated copies of the same object, use one mesh visual with `"instance_transform"` rather
than creating one mesh visual per copy. See [Add visuals to a panel](../../how-to/add-a-visual.md).

## Avoid When

Use [Primitive](primitive.md) for raw topology experiments, [Sphere](sphere.md) for many analytic
spheres, or [Volume](volume.md) for sampled 3D scalar fields.

## Item And Data Model

Create with `dvz_mesh(scene, flags)`. `N` is the vertex count and the topology is triangle list.
Upload dense vertex arrays directly or copy a `DvzGeometry` with `dvz_mesh_set_geometry()`.
Optional `I` indices select vertices. Optional `M` instance transforms create `M` draws of the
same geometry; upload them separately because their count differs from `N`.

## Attribute And Resource Contract

| Attribute/resource | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Vertex positions in authored visual/object coordinates. | Dense or range upload; changing count regenerates omitted default color. |
| `color` | Optional; omitted data becomes opaque white per vertex | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8 vertex color. Scalar color is not supported. | Dense or range upload. |
| `normal` | Optional; absence selects the non-normal material route | `vec3[N]` | `float32`, `(N, 3)` | Vertex normals in object/model basis for lighting. | Dense or range upload. |
| `texcoords` | Optional; required for textured mesh | `vec2[N]` | `float32`, `(N, 2)` | Normalized texture UV coordinates. | Dense or range upload. |
| `instance_transform` | Optional; absence means one instance | `mat4[M]` | `float32`, `(M, 4, 4)` contiguous | One model transform per instance. This is an instance-rate attribute and need not have vertex count `N`. | Upload separately with `dvz_visual_set_data()` or range update. |
| `item_state` | Optional; instance-rate retained state | `uint32_t[M]` | `uint32`, `(M,)` | `DvzItemStateKind` bitfield; when transforms exist, count should match instance count. Textured-mesh item-state styling is not supported. | Usually interaction-managed; direct dense/range upload is supported. |
| index data | Optional | `uint32_t[I]` | `uint32`, `(I,)` | Values index `[0,N)`; triangle-list grouping normally uses multiples of three. | `dvz_visual_set_index_data()` or external `"index"` buffer. |
| texture slot `"texture"` | Optional; enables textured mesh | `DvzSampledField*` | sampled-field handle | RGBA8 2D sampled field in the active first slice; requires `texcoords`. | `dvz_visual_set_field()`; configure filtering with `dvz_visual_set_field_sampling()`. |

## Constructor And Options

- Constructor: `dvz_mesh(scene, flags)`; examples pass `flags = 0`.
- `dvz_mesh_set_geometry()` copies geometry positions, colors, normals, texcoords, and optional
  triangle indices.
- Common options include material, alpha mode, depth test, depth cue, and transform.
- Texture sampling defaults to linear filtering, clamp-to-edge addressing, and no mipmaps. Use
  `dvz_visual_set_field_sampling(mesh, "texture", &sampling)` for nearest filtering when texels
  must remain discrete.
- Upload vertex attributes together; upload instance-rate arrays separately when `M != N`.

## Verified Usage Pattern

The [C example](../../examples/gallery/visuals/visuals_mesh.md) uses
`dvz_mesh_set_geometry()`. Its Python counterpart uploads `position`, `color`, and indices directly.

## Picking And Probing

Mesh bounds are available from retained CPU-side attributes. Picking can identify mesh geometry;
use explicit instance attributes when instance-level identity matters.

## Backend Notes

Native and WebGPU paths are active for indexed and RGBA8 textured geometry. The
[textured-mesh example](../../examples/gallery/features/features_mesh_texture.md) demonstrates an
explicit field-slot sampling descriptor.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/mesh.c` |
| Gallery | [Mesh](../../examples/gallery/visuals/visuals_mesh.md) |
| Build | `just example-c visuals/mesh` |
| Smoke | `./build/examples/c/visuals/mesh --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[3D navigation](../../how-to/3d-navigation.md),
[lighting and materials](../../how-to/lighting-and-materials.md), [Primitive](primitive.md),
[Sphere](sphere.md).
