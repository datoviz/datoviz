# Visual Family: `splat`

Status: implemented v0.4 experimental visual family with a narrow first slice.

This document defines a deliberately small screen-space Gaussian splat contract. It refines
`../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`,
`../pipeline/ATTRIBUTE_SOURCES.md`, and `../semantics/TRANSPARENCY.md`.

Full Gaussian-splat pipelines, projected 3D covariance, sorting, tile binning, and trained asset
formats remain future work. The broader frame-plan pressure is recorded in
`../proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md`.


## Current Implementation Status

Status on 2026-05-27: `splat` has a retained scene API and render path.

The active runtime implements `DVZ_VISUAL_TYPE_SPLAT`, public `dvz_splat()`, retained
`position`/`color`/`sigma`/`angle` attributes, finite-positive `sigma` validation, finite `angle`
validation, GLSL and WGSL shader variants, and DRP2 lowering through the normal scene visual path.

This first slice intentionally omits separate `opacity`, WBOIT/depth-peel shader variants, and
request/readback behavior. It renders rotated screen-space Gaussian billboards with source-over
alpha blending and center-depth testing.


## Semantic Purpose

`splat` renders one continuous Gaussian footprint per item. The footprint is screen-facing,
elliptical, and measured in screen pixels.

It is separate from nearby point-like families:

| Family | Use when |
|---|---|
| `point` | the mark is a compact antialiased circle with no Gaussian tail |
| `marker` | the mark is a discrete symbol or SDF shape with fill/stroke styling |
| `sphere` | the item has 3D radius, normals, material, and depth-correct impostor behavior |
| `splat` | the item represents a smooth kernel, uncertainty footprint, or density-like blob |

Round Gaussian billboards are expressed by equal `sigma.x` and `sigma.y`; they are not a separate
family.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

Splat center before panel transform and projection.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Base RGBA color. The alpha channel contributes to final
splat opacity.

Accepted sources for the first slice: `PER_ITEM`, with `CONSTANT` and `PER_GROUP` allowed by the
target contract when the generic source machinery supports them for this family.


### `sigma`

| Property | Value |
|---|---|
| Type | `vec2`, `(sigma_x, sigma_y)` |
| Unit | screen pixels |
| Accepted sources | `PER_ITEM` initially; target contract also allows `CONSTANT` and `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |

Standard deviations along the local ellipse axes. Both components must be finite and positive.
The isotropic case uses `sigma_x == sigma_y`.


### `angle`

Standard — see `SHARED_ATTRIBUTES.md`.

Applied in screen space to the local ellipse axes. Values must be finite. The current first slice
accepts only `PER_ITEM` angles.


### `opacity` (deferred)

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | multiplier in `[0, 1]` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes |

Multiplies the color alpha after Gaussian evaluation. The current first slice uses `color.a`
directly and does not expose a separate opacity attribute.


## Visual-Wide Parameters

| Parameter | Type | Default | Meaning |
|---|---|---:|---|
| `kernel` | enum | `gaussian` | Kernel evaluated in local ellipse coordinates; v0.4 only requires Gaussian. |
| `cutoff` | `float32` | `3.0` | Billboard half-size multiplier in standard deviations. |
| `min_alpha` | `float32` | `0.0` | Optional low-alpha discard threshold before blending. |

`cutoff` controls the conservative billboard extent:

```text
half_extent_px = cutoff * max(sigma_x, sigma_y)
```

A larger cutoff preserves more tail energy and increases overdraw. `cutoff <= 0`, non-finite
values, and non-positive `sigma` values are validation errors.


## Kernel Evaluation

The fragment shader evaluates opacity in local ellipse coordinates:

```text
q = rotate(local_pixel_offset, -angle) / sigma
alpha_kernel = exp(-0.5 * dot(q, q))
alpha = color.a * opacity * alpha_kernel
```

Fragments outside the cutoff ellipse may be discarded. The shader must follow the scene's active
straight-alpha or premultiplied-alpha output convention; this contract defines only the scalar
opacity before that convention is applied.


## Variant Axes

Required first slice:

| Axis | Values | Default |
|---|---|---|
| `kernel` | `gaussian` | `gaussian` |
| `sigma_space` | `screen` | `screen` |

Deferred variant axes:

| Axis | Deferred values |
|---|---|
| `sigma_space` | `data`, `world`, projected covariance |
| `color_mode` | scalar color mapped through a `Scale` |
| `quality` | unsorted, WBOIT-preferred, sorted, tile-binned |

`sigma_space = screen` is the only v0.4 conformance target. A runtime must not reinterpret
data-space covariance as screen-pixel `sigma` silently.


## Transform Model

The splat center follows the standard scene transform model:

```text
data/scene position -> panel transform -> camera/projection -> screen center
```

The footprint is screen-facing and measured in screen pixels. It does not rotate with the 3D camera
except through the explicit screen-space `angle` attribute.


## Depth And Transparency

The default splat path is transparent:

| Property | Default |
|---|---|
| Depth test | enabled |
| Depth write | disabled |
| Alpha mode | `DVZ_ALPHA_BLENDED` |
| Fragment depth | center depth |

The visual participates in the same source-over transparent-stage routing as other blended scene
visuals. Weighted blended OIT and exact sorted compositing are deferred.


## Picking And Requests

GPU picking is optional for the first experimental slice. If implemented, the minimum behavior is:

1. hit if the query position lies within the cutoff ellipse,
2. report `(panel_id, visual_id, item_index)`,
3. use center depth for depth ordering.

If no GPU request path exists, splat picking must fail explicitly through the scene request status
mechanism rather than falling back to CPU testing or silently returning a miss.

Deferred request semantics include highest-contribution picking, top-k contributing splats,
integrated-kernel queries, transparent hit reconstruction, and 3D covariance-aware picking.


## Rendering Model

The preferred v0.4 rendering path is an instanced billboard:

1. one logical instance per splat,
2. four generated or static quad corners,
3. per-instance `position`, `color`, `sigma`, and `angle`,
4. vertex expansion in screen space,
5. fragment Gaussian opacity evaluation.

This is ordinary scene -> DRP2 render emission. It must not depend on compute passes, indirect
draws, CPU/GPU sort, tile bins, or a splat-specific runtime escape hatch.


## Minimum Cases This Spec Must Support

1. isotropic soft points: per-item `position`, `color`, and equal `sigma` components,
2. anisotropic uncertainty ellipses: per-item `sigma` and `angle`,
3. translucent density cloud: low-alpha colors with source-over blending,
4. streaming kernels: range updates of `position`, `color`, `sigma`, and `angle`,
5. sparse overlaid kernels with opaque geometry depth-tested by center depth.


## Deferred Tiers

The following are outside the v0.4 splat contract:

1. packed 2D covariance attributes,
2. projected 3D covariance from `scale + rotation` or covariance matrices,
3. compute projection, culling, compaction, or sorting,
4. CPU or GPU depth sorting,
5. indirect draw or generated draw counts,
6. tile-binned splatting,
7. spherical harmonic colors,
8. 3DGS PLY loaders, trained asset formats, LOD, and out-of-core streaming.
