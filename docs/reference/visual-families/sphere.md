# Sphere

Analytic 3D sphere impostors with per-item center, radius, color, and optional lighting.

Status: supported.
Backends: native; WebGPU live (`sphere`, `arcball`).
Primitive: impostor quads with fragment-shader sphere reconstruction.

## Preview And Links

[![Sphere](../../assets/gallery/v0.4/visuals/sphere_impostor.webp)](../../examples/gallery/visuals/sphere_impostor.md)

- Example: [Sphere](../../examples/gallery/visuals/sphere_impostor.md)
- How-to: [Use 3D controllers](../../how-to/3d-navigation.md), [use lighting and materials](../../how-to/lighting-and-materials.md)
- Related: [Point](point.md), [Mesh](mesh.md), [Splat](splat.md)

## Use When

Use sphere visuals for many true 3D balls where each item has a world-space radius and should sort
or shade as a sphere rather than a flat screen-space mark.

## Avoid When

Use [Point](point.md) for simple screen-sized circles, [Mesh](mesh.md) for arbitrary solid
geometry, or [Splat](splat.md) for translucent Gaussian footprints.

## Data Model

Create with `dvz_sphere(scene, flags)`. The canonical example enables lighting, sets
`DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR`, applies a standard material, and uploads one item per sphere.

## Attributes

Required: `position` (`vec3` center), `radius` (`float`, object-space), `color` (RGBA8).

Optional: mode through `dvz_sphere_mode()`, material, depth cue, alpha mode, depth test, transform,
and visual-wide scale bindings.

## Picking And Probing

Picking and bounds are item-based. Raycast impostor mode writes sphere-surface depth for more
accurate 3D interaction than flat point sprites.

## Backend Notes

Native and WebGPU paths are active. The GLSL native backend reconstructs the surface in the fragment
shader and uses analytic antialiasing at silhouettes.

## Canonical Example

- Source: `examples/c/visuals/sphere.c`
- Gallery: [Sphere](../../examples/gallery/visuals/sphere_impostor.md)
- Build: `just example-c visuals/sphere`
- Smoke: `./build/examples/c/visuals/sphere --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[3D navigation](../../how-to/3d-navigation.md),
[lighting and materials](../../how-to/lighting-and-materials.md), [Point](point.md),
[Mesh](mesh.md).
