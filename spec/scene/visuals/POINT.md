# Visual Family: `point`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`point` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.
Landed naming and first-slice decisions are tracked in `IMPLEMENTATION_DECISIONS.md`.


## Current Implementation Status

Status on 2026-05-17: the active v0.4 runtime implements the first styled point slice.

The implemented path supports:

1. retained `point` visual construction via `dvz_point()`;
2. dense `position`, `color`, and public `diameter_px` attributes, where `diameter_px` aliases the
   current internal `size` slot and is measured in screen pixels;
3. antialiased circular rendering;
4. `dvz_point_style_desc()` and `dvz_point_set_style()` with `edge_color`, `stroke_width_px`,
   and exclusive `filled`/`stroke`/`outline` aspect semantics;
5. GLSL/Vulkan native point-list lowering using point-coordinate coverage;
6. WGSL/WebGPU instanced-quad lowering;
7. GPU-backed circular picking that rejects points outside the disc;
8. existing depth cueing, EDL, alpha-mode, WBOIT/depth-peel, and app/offscreen coverage for point
   visuals.

The following sections describe the target point contract. Scalar color/diameter_px modes, `shift`, and
data-space diameter_px are planned capabilities unless explicitly marked as implemented above.


## Semantic Purpose

`point` renders circular point-like marks with per-item diameter_px control.

It is richer than `pixel` (circular coverage and optional edge/stroke styling) and simpler than
`marker` (no shape selection or rotation).

Right for scatter plots, particle systems, and similar data where items need independent sizes but
do not require shaped or styled marks.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.


### `diameter_px`

Standard — see `SHARED_ATTRIBUTES.md`.
Storage name: `size`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
Per-item diameter_px is the defining attribute of `point` relative to `pixel`.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.
Useful for jitter plots and zoom-invariant nudging.

Status on 2026-05-17: not implemented in the active point slice.


## Visual-Wide Parameters

### `diameter_default`

| Property | Value |
|---|---|
| Type | `float32`, unit determined by `size_space` |
| Default | implementation-defined, suggested 5.0 screen pixels |
| Mutability | `dynamic` |

Fallback used when `diameter_px` source is `CONSTANT` and no value has been set.
Ignored when `diameter_px` source is `PER_ITEM` or `PER_GROUP`.


### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.

Implementation status on 2026-05-16: `size_space = data` is part of the required point
contract even when an early backend only supports screen-pixel point sizes. A backend that cannot
yet project data-space radii must emit a capability diagnostic or fall back explicitly; it should
not reinterpret data-space sizes as screen pixels silently.


### `aspect`

| Property | Value |
|---|---|
| Type | enum: `filled`, `stroke`, `outline` |
| Default | `filled` |
| Mutability | `dynamic` |

- `filled`: solid fill, no visible edge.
- `stroke`: edge only, no fill. Width controlled by `stroke_width_px`.
- `outline`: filled body with edge on top. Uses both `color` (fill) and `edge_color` (edge).


### `edge_color`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Edge color for `aspect = stroke` or `aspect = outline`. Visual-wide.


### `stroke_width_px`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `0.0` |
| Mutability | `dynamic` |

Edge width for `aspect = stroke` or `aspect = outline`.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf item skipped and not pickable | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `diameter_px` | `diameter_default` | scalar NaN uses size-scale fallback | yes |
| `diameter_default` | family-defined screen size | n/a | yes |
| `aspect` | `filled` | n/a | yes |
| `edge_color` | black RGBA | n/a | yes |
| `stroke_width_px` | 0 screen pixels | n/a | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

`size_space` is a visual-wide parameter for the public `diameter_px` attribute, not a variant axis. It
does not affect data layout and can change at runtime via
`dvz_visual_set_param(visual, "size_space", &space)`.

Both mode axes are set at visual creation time by combining flag constants and passing to
`dvz_point(scene, flags)`:

| Group | Flag constants |
|---|---|
| color mode | `DVZ_COLOR_RGBA` (default), `DVZ_COLOR_SCALAR` |
| size mode | `DVZ_SIZE_DIRECT` (default), `DVZ_SIZE_SCALAR` |

All four combinations are valid. Most common: `(rgba, direct)` for scatter plots, `(scalar,
scalar)` for bubble charts.

There is no separate `alpha` attribute. Use the alpha channel of `color`.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| No per-item diameter_px needed | `pixel` |
| Need shape, rotation, or edge | `marker` |
| Need connected geometry | `segment` or `path` |
| Need raw topology | `primitive` |


## Minimum Cases This Spec Must Support

1. uniform scatter plot — `color` and `diameter_px` both `CONSTANT`,
2. per-point color — `color` `PER_ITEM` rgba, `diameter_px` `CONSTANT`,
3. per-point diameter_px — `color` `CONSTANT`, `diameter_px` `PER_ITEM` direct,
4. fully independent points — `color` and `diameter_px` both `PER_ITEM`,
5. bubble chart — `color` `PER_ITEM` scalar, `diameter_px` `PER_ITEM` scalar with sqrt scale,
6. three cell populations with per-type color and diameter_px — both `PER_GROUP`,
7. live neural spike positions, colors fixed by cell type — `position` streaming, `color`
   `PER_GROUP` static.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_point_position` | `position`, `PER_ITEM` |
| `dvz_point_size` | `diameter_px`, `PER_ITEM`, `direct` mode |
| `dvz_point_color` | `color`, `PER_ITEM`, `rgba` mode |

v0.4 adds: `CONSTANT`/`PER_GROUP` sources for `color` and `diameter_px`, `scalar` modes, `shift`,
`size_space`.


## Rendering Model

`point` exposes smooth circular marks regardless of backend. The active GLSL/Vulkan path keeps
native point-list lowering and evaluates circular coverage from point coordinates. The active
WGSL/WebGPU lowering uses instanced quads because WebGPU has no native point-size equivalent.

A later slice may move GLSL to instanced quads too if exact GLSL/WGSL parity becomes more important
than native point-list throughput.

Depth sorting of semi-transparent points uses the scene's transparency path
(see `../semantics/TRANSPARENCY.md`). `point` declares `alpha_mode` like any other visual.
