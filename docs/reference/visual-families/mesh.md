# Mesh

Retained triangle mesh visual for indexed or non-indexed 3D geometry.

Status: supported.
Backends: native; WebGPU live (`mesh`, `indexed-geometry`, `arcball`).
Primitive: triangle list.

## Preview And Links

[![Mesh](../../assets/gallery/v0.4/visuals/visual_mesh.webp)](../../examples/gallery/visuals/visual_mesh.md)

- Example: [Mesh](../../examples/gallery/visuals/visual_mesh.md)
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

## Data Model

Create with `dvz_mesh(scene, flags)`. Upload dense vertex attributes directly or copy a
`DvzGeometry` with `dvz_mesh_set_geometry()`. Optional indices bind through the `"index"` slot.

## Attributes

Required: `position` (`vec3`).

Optional: `color` (RGBA8, defaults to opaque white when omitted), `normal` (`vec3`), `texcoords`
(`vec2`), `instance_transform` (`mat4` per instance), `"index"` buffer, RGBA8 sampled field bound
to `"texture"`, material, depth cue, alpha mode, depth test, transform.

## Picking And Probing

Mesh bounds are available from retained CPU-side attributes. Picking can identify mesh geometry;
use explicit instance attributes when instance-level identity matters.

## Backend Notes

Native and WebGPU paths are active for indexed geometry. The canonical example uses an arcball-style
3D camera and a retained cube mesh helper.

## Canonical Example

- Source: `examples/c/visuals/mesh.c`
- Gallery: [Mesh](../../examples/gallery/visuals/visual_mesh.md)
- Build: `just example-c visuals/mesh`
- Smoke: `./build/examples/c/visuals/mesh --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[3D navigation](../../how-to/3d-navigation.md),
[lighting and materials](../../how-to/lighting-and-materials.md), [Primitive](primitive.md),
[Sphere](sphere.md).
