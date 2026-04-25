# Visual Family: `point`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`point` visual family.

It refines:

1. `VISUAL_FAMILIES.md` — family taxonomy and rationale
2. `VISUAL_MINI_CONTRACTS.md` — family-level mini-contract
3. `ATTRIBUTE_SOURCES.md` — attribute granularity and mutability vocabulary
4. `VISUAL_CONTRACT.md` — shared visual responsibilities


## Semantic Purpose

`point` renders circular point-like marks with per-item size control.

It is richer than `pixel` (per-item size) and simpler than `marker` (no shape, rotation, or edge
treatment).

`point` is the right choice for scatter plots, particle systems, and similar data where items need
independent sizes but do not require shaped or styled marks.


## Per-Item Attribute Schema

### `position`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

Identical to `pixel`. `z` participates in depth ordering.


### `color`

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color mode |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |

Identical to `pixel`. Two color modes: `rgba` (default) and `scalar` (mapped through a
`Scale` object — see `SCALES.md`).


### `size`

| Property | Value |
|---|---|
| Type | `float32` (direct) or `scalar_f32` (mapped) — see size mode |
| Unit | screen pixels (direct) or domain units mapped to pixels (scalar) |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

Per-item size is the defining attribute of `point` relative to `pixel`.

Two size modes are defined as a variant axis:

**`direct` mode** (default): the user supplies a `float32` size in screen pixels directly.
`CONSTANT` source gives all points the same size.
`PER_ITEM` source gives each point its own size.
`PER_GROUP` source gives each group a shared size (e.g., three cell types with different typical
radii).

**`scalar` mode**: the user supplies a `float32` scalar value per item (or constant, or per-group),
mapped through an associated size `Scale` object to a pixel size.
See `SCALES.md` for the scale contract (`kind = size`).
This is the natural encoding for bubble charts where mark area encodes a data quantity.
The size scale supports `sqrt` interpolation so that perceived area scales linearly with the
underlying data value.


## Visual-Wide Parameters

### `size_default`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | screen pixels |
| Default | implementation-defined, suggested 5.0 |
| Mutability | `dynamic` |

Fallback size used when `size` source is `CONSTANT` and no explicit constant is set, or as a
default before the first `size` write.

This parameter is distinct from the `size` attribute.
When `size` source is `PER_ITEM` or `PER_GROUP`, `size_default` is ignored.


## Variant Axes

`point` has two independent variant axes:

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

Both axes are selected at visual creation time and cannot change without recreating the visual.

The four combinations are all valid.
The most common are `(rgba, direct)` for simple scatter plots and `(scalar, scalar)` for bubble
charts where both color and size encode data quantities.


## Transform Model

Standard two-stage transform:

1. **Normalization** — data-space positions normalized to visual space before upload.
2. **Panel transform** — panel-local camera or panzoom applied per-frame without re-upload.

`point` does not support a visual-local transform matrix.


## Stage Participation

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional, natural |
| Offscreen / export | same as render |

Picking is natural for this family.
Each point maps directly to one item index.


## Picking Model

When picking is enabled:

1. a pick result returns `(panel_id, visual_id, item_index)`,
2. no sub-item identity is defined.

Hover picking follows latest-request-wins semantics.


## Fallback Notes

If the runtime cannot support `size_mode = scalar` GPU-side, the scene falls back to computing
sizes on the CPU at upload time, same as the `color_mode = scalar` fallback.

If per-item size is unsupported (unlikely but possible on constrained backends), the scene may fall
back to `size_default` for all points and emit a capability adaptation diagnostic.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| No per-item size needed | `pixel` |
| Need shape, rotation, or edge | `marker` |
| Need connected geometry | `segment` or `path` |
| Need raw topology control | `primitive` |

`point` should not grow shape or edge controls.
Any such request is pressure toward `marker`.


## Minimum Cases This Spec Must Support

1. uniform scatter plot — `color` and `size` both `CONSTANT`,
2. scatter plot with per-point color — `color` `PER_ITEM` rgba, `size` `CONSTANT`,
3. scatter plot with per-point size — `color` `CONSTANT`, `size` `PER_ITEM` direct,
4. fully independent points — `color` and `size` both `PER_ITEM`,
5. bubble chart — `color` `PER_ITEM` scalar, `size` `PER_ITEM` scalar with sqrt scale,
6. three cell populations with per-type color and per-type size — both `PER_GROUP`,
7. live neural spike positions, colors fixed by cell type — `position` streaming, `color`
   `PER_GROUP` static.


## v0.3 Correspondence

In v0.3:

```c
dvz_point(batch, flags)
dvz_point_position(visual, first, count, positions, flags)
dvz_point_size(visual, first, count, sizes, flags)
dvz_point_color(visual, first, count, colors, flags)
dvz_point_alloc(visual, item_count)
```

| v0.3 | v0.4 |
|---|---|
| `dvz_point_position` | `position`, `PER_ITEM` |
| `dvz_point_size` | `size`, `PER_ITEM`, `direct` mode |
| `dvz_point_color` | `color`, `PER_ITEM`, `rgba` mode |

v0.4 adds: `CONSTANT` and `PER_GROUP` sources for both `color` and `size`, and `scalar` mode for
both.
The `PER_ITEM` direct path is a strict superset of the v0.3 behavior.


## Deferred Questions

1. whether `point` should render as a filled circle (smooth disc) or as a hardware point sprite,
   and whether this is a variant axis or a capability-gated fallback,
2. the exact public API spelling for declaring `color_mode` and `size_mode` at creation,
3. whether a separate `alpha` attribute (distinct from the alpha channel of `color`) is useful,
4. whether depth sorting of semi-transparent points is in scope for this family or deferred.
