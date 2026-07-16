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

## Item And Data Model

Create with `dvz_splat(scene, flags)`. One item is one screen-facing anisotropic Gaussian
footprint. Upload `N` rows for every required attribute. The constructor enables alpha blending
and depth testing; the first implementation uses center depth, no sorting, and no projected 3D
covariance.

## Attribute Contract

| Attribute | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[N]` | `float32`, `(N, 3)` | Center in authored visual coordinates. | Dense or range upload. |
| `color` | Required; no documented default | `DvzColor[N]` | `uint8`, `(N, 4)` | RGBA8. Scalar color is not supported by this family. | Dense or range upload. |
| `sigma` | Required; no documented default | `vec2[N]` | `float32`, `(N, 2)` | Gaussian x/y standard deviations in logical pixels; both components must be finite and strictly positive. | Dense or range upload. |
| `angle` | Required; no documented default | `float[N]` | `float32`, `(N,)` | Finite rotation angle in radians. | Dense or range upload. |

Color and sigma accept explicitly configured constant/per-group sources. Dense arrays do not
broadcast.

## Constructor And Options

- Constructor: `dvz_splat(scene, flags)`; the canonical C example passes `flags = 0`.
- The constructor selects alpha blending and enables depth testing. Change those only with the
  common visual-wide alpha/depth setters after considering translucent ordering limitations.
- No canonical Python example is currently published; the top-level NumPy upload contract follows
  the shapes above.

## Verified Usage Pattern

Upload all four arrays with `dvz_visual_set_data_many()`, then attach the visual. The
[canonical C example](../../examples/gallery/visuals/visuals_splat.md) is the executable source.

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
