# Splat

Experimental screen-facing Gaussian footprint visual.

Status: experimental.
Backends: native; WebGPU deferred (`splat`, `alpha-blending`).
Primitive: screen-space Gaussian quads.

## Preview And Links

[![Splat](../../assets/gallery/v0.4/visuals/visuals_splat.webp)](../../examples/gallery/visuals/visuals_splat.md)

- Example: [Splat](../../examples/gallery/visuals/visuals_splat.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [control depth, blending, and transparency](../../how-to/depth-blending.md)
- Related: [Point](point.md), [Sphere](sphere.md), [Mesh](mesh.md)

## Use When

Use splat visuals for experimental translucent Gaussian marks where each item has an anisotropic
screen-space footprint.

## Avoid When

Use [Point](point.md) for stable circular sprites, [Sphere](sphere.md) for true 3D balls, or
[Mesh](mesh.md) for surface geometry. Avoid splat as copy-paste starter code while it remains
experimental.

## Data Model

Create with `dvz_splat(scene, flags)`. Upload one item per Gaussian footprint. The first
implementation uses center depth, depth test on, depth writes off through alpha blending, and no
sorting or projected 3D covariance.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | `position` (`vec3` center), `color` (RGBA8), `sigma` (`vec2`, screen pixels), `angle` (`float`, radians) |
| Optional | alpha mode; depth test; transform; visual-wide scale bindings |

## Picking And Probing

Treat splat picking as experimental. The retained item identity is available, but translucent
overlap, center-depth ordering, and lack of sorting limit interpretation.

## Backend Notes

Native support exists. WebGPU is deferred in the manifest because splat rendering is outside the RC
browser subset. The canonical example is intended for advanced visual-family inspection rather than
as a first example.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/splat.c` |
| Gallery | [Splat](../../examples/gallery/visuals/visuals_splat.md) |
| Build | `just example-c visuals/splat` |
| Smoke | `./build/examples/c/visuals/splat --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[rendering techniques](../../how-to/depth-blending.md), [Point](point.md), [Sphere](sphere.md).
