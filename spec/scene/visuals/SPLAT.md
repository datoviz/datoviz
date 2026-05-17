# Splat Visual Contract

Status: proposed v0.4 visual family.

This document defines the v0.4 contract for a simple anisotropic Gaussian splat visual.
It is intentionally limited to screen-space billboard splats so it can land without requiring
GPU sorting, compute prepasses, indirect draws, tile binning, or 3D Gaussian Splatting
compatibility.

## Scope

The v0.4 `splat` family renders one screen-facing Gaussian footprint per item. Each item has a
center position in the normal Datoviz scene coordinate pipeline and an anisotropic footprint in
screen pixels.

The first implementation should support:

1. screen-space anisotropic Gaussian footprints,
2. the isotropic Gaussian case as `sigma.x == sigma.y`,
3. per-item color and optional opacity,
4. transparent rendering through the existing scene alpha/depth machinery,
5. retained scene data updates through the normal visual data path,
6. DRP2 emission as an ordinary render visual.

The first implementation must not depend on:

1. GPU or CPU depth sorting,
2. compute prepasses,
3. indirect draw or dispatch,
4. tile binning,
5. projected 3D covariance,
6. spherical harmonic colors,
7. 3DGS dataset loaders.

## Relationship to points and markers

The `splat` family is not a replacement for `point` or `marker`:

- `point` remains the compact antialiased circular point primitive.
- `marker` remains the discrete shape/SDF marker primitive.
- `splat` represents a continuous kernel footprint with explicit Gaussian falloff and
  anisotropic screen-space support.

Round Gaussian billboards are a degenerate form of the splat visual, not a separate visual family.

## Per-item attributes

| Attribute | Type | Required | Units | Meaning |
|---|---:|---:|---|---|
| `position` | `vec3f` | yes | scene/data coordinates | Splat center before panel transform and camera projection. |
| `color` | `rgba8` or active scene color format | yes | normalized color | Base splat color. Alpha participates in final opacity. |
| `sigma` | `vec2f` | yes | screen pixels | Standard deviations along the local ellipse axes. |
| `angle` | `float` | no | radians | Screen-space rotation of the local ellipse axes. Defaults to `0`. |
| `opacity` | `float` | no | unitless | Multiplicative opacity. Defaults to `1`. |

`position`, `color`, and `sigma` are the minimal required v0.4 data contract. `angle` and `opacity`
should be accepted when the visual data path supports optional attributes; otherwise they may be
implemented as visual-wide defaults in the first implementation and promoted to per-item attributes
before the family is considered complete.

All per-item attributes share the same item count. Range updates follow the same mutability and
validation rules as other retained visual attributes.

## Visual-wide parameters

| Parameter | Default | Meaning |
|---|---:|---|
| `kernel` | `gaussian` | Kernel evaluated inside the billboard footprint. v0.4 only requires Gaussian. |
| `footprint` | `screen_ellipse` | Footprint is a screen-space ellipse. |
| `cutoff` | `3.0` or `4.0` | Billboard half-size multiplier in standard deviations. |
| `min_alpha` | implementation-defined | Optional low-alpha discard/clamp threshold. |

The visual-wide `cutoff` controls the conservative billboard extent:

```text
half_extent_px = cutoff * max(sigma.x, sigma.y)
```

The implementation should choose a default that balances quality and overdraw. A cutoff of `3.0`
is faster; a cutoff of `4.0` preserves more tail energy.

## Kernel evaluation

The fragment shader evaluates a Gaussian in local ellipse coordinates:

```text
q = rotate(local_pixel_offset, -angle) / sigma
alpha_kernel = exp(-0.5 * dot(q, q))
alpha = color.a * opacity * alpha_kernel
```

Fragments outside the cutoff ellipse may be discarded. The shader must use the existing scene
convention for premultiplied or straight-alpha output; this contract only defines the scalar opacity
before that convention is applied.

## Transform model

The splat center follows the normal Datoviz scene transform stack:

```text
data/scene position -> panel transform -> camera/projection -> clip/screen center
```

The footprint is screen-facing and measured in screen pixels. It does not rotate with the 3D camera
except through the explicit screen-space `angle` attribute.

The v0.4 contract does not define data-space or world-space splat radii. Projected 3D covariance is
reserved for a later tier.

## Depth and transparency

The v0.4 splat visual is a transparent render visual by default.

Recommended defaults:

1. depth test: enabled,
2. depth write: disabled,
3. alpha mode: ordinary alpha blending unless the user selects weighted blended OIT,
4. depth value: center depth.

The visual should be eligible for the same transparent-stage routing as other alpha-enabled scene
visuals. If weighted blended OIT is active and the current frame plan supports the target formats,
splats may render through that path. Exact order-dependent compositing is out of scope for v0.4.

## Picking and requests

GPU picking is optional for v0.4. If implemented, the initial behavior should be explicit and simple:

1. hit if the query position lies within the cutoff ellipse,
2. report the item identity,
3. use center depth for depth ordering.

The following picking semantics are deferred:

1. highest Gaussian contribution among overlapping splats,
2. top-k contributing splats,
3. integrated-kernel queries,
4. sorted transparent hit reconstruction,
5. 3D covariance-aware picking.

If no GPU picking path exists, splat picking must fail explicitly through the scene request status
mechanism rather than silently falling back to CPU testing.

## Implementation guidance

The preferred v0.4 implementation is an instanced billboard path:

1. one item instance per splat,
2. four quad corners generated from vertex index or a small static quad buffer,
3. per-instance `position`, `color`, `sigma`, optional `angle`, optional `opacity`,
4. vertex shader expands the quad in screen space,
5. fragment shader evaluates the Gaussian opacity.

A single oversized triangle per splat is acceptable if it better matches the existing renderer, but
an instanced quad is easier to validate and extend.

## Deferred tiers

The `splat` family should be designed so future tiers can add:

1. packed 2D covariance attributes,
2. projected 3D covariance from `scale + rotation` or a covariance matrix,
3. compute preprojection and culling,
4. weighted blended OIT quality improvements,
5. CPU and GPU depth sorting options,
6. indirect draws and generated draw counts,
7. tile binning for large transparent splat sets,
8. spherical harmonic colors for 3DGS-like datasets.

Those tiers are roadmap items, not v0.4 conformance requirements.
