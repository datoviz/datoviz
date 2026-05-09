# Visual Family: `point`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`point` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`point` renders circular point-like marks with per-item size control.

It is richer than `pixel` (per-item size) and simpler than `marker` (no shape, rotation, or edge
treatment).

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


### `size`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
Per-item size is the defining attribute of `point` relative to `pixel`.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.
Useful for jitter plots and zoom-invariant nudging.


## Visual-Wide Parameters

### `size_default`

| Property | Value |
|---|---|
| Type | `float32`, unit determined by `size_space` |
| Default | implementation-defined, suggested 5.0 screen pixels |
| Mutability | `dynamic` |

Fallback used when `size` source is `CONSTANT` and no value has been set.
Ignored when `size` source is `PER_ITEM` or `PER_GROUP`.


### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf item skipped and not pickable | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `size` | `size_default` | scalar NaN uses size-scale fallback | yes |
| `size_default` | family-defined screen size | n/a | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

`size_space` is a visual-wide parameter, not a variant axis — it does not affect data layout and
can change at runtime via `dvz_visual_set_param(visual, "size_space", &space)`.

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
| No per-item size needed | `pixel` |
| Need shape, rotation, or edge | `marker` |
| Need connected geometry | `segment` or `path` |
| Need raw topology | `primitive` |


## Minimum Cases This Spec Must Support

1. uniform scatter plot — `color` and `size` both `CONSTANT`,
2. per-point color — `color` `PER_ITEM` rgba, `size` `CONSTANT`,
3. per-point size — `color` `CONSTANT`, `size` `PER_ITEM` direct,
4. fully independent points — `color` and `size` both `PER_ITEM`,
5. bubble chart — `color` `PER_ITEM` scalar, `size` `PER_ITEM` scalar with sqrt scale,
6. three cell populations with per-type color and size — both `PER_GROUP`,
7. live neural spike positions, colors fixed by cell type — `position` streaming, `color`
   `PER_GROUP` static.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_point_position` | `position`, `PER_ITEM` |
| `dvz_point_size` | `size`, `PER_ITEM`, `direct` mode |
| `dvz_point_color` | `color`, `PER_ITEM`, `rgba` mode |

v0.4 adds: `CONSTANT`/`PER_GROUP` sources for `color` and `size`, `scalar` modes, `shift`,
`size_space`.


## Rendering Model

`point` always renders as a smooth disc using an SDF impostor (circle SDF in the fragment
shader), not as a hardware point sprite. Hardware point sprites are square on many drivers
and not supported in WebGPU; they are never exposed.

Each point occupies a quad (two triangles); the fragment shader discards corners outside the
circle. This is the same approach used by `marker` without the shape or rotation machinery.

Depth sorting of semi-transparent points uses the scene's transparency path
(see `semantics/TRANSPARENCY.md`). `point` declares `alpha_mode` like any other visual.
