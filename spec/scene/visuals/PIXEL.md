# Visual Family: `pixel`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`pixel` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`pixel` renders filled square pixel-like marks at given positions.

It is intentionally the simplest mark family:

1. no shape selection,
2. no per-item rotation,
3. no edge or stroke treatment,
4. one size shared by all items.

`pixel` is the right choice when the user needs to render many items efficiently and uniform mark
shape is acceptable.
For per-item size, shape, or edge treatment, use `point` or `marker`.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

`z` is available for depth ordering but is typically zero for 2D scenes.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.


## Visual-Wide Parameters

### `size`

| Property | Value |
|---|---|
| Type | `float32`, unit determined by `size_space` |
| Default | implementation-defined, suggested 1.0 screen pixels |
| Mutability | `dynamic` |

Size of every pixel mark. All items share the same size.
For per-item size, use `point`.

### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.


## Fallback

`pixel` is already the simplest mark family. Capability fallback pressure is low.
`color_mode = scalar` fallback: see `SHARED_ATTRIBUTES.md`.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Need per-item size | `point` |
| Need shape, rotation, or edge | `marker` |
| Need connected lines or paths | `path` or `segment` |
| Need low-level topology control | `primitive` |


## Minimum Cases This Spec Must Support

1. 1M uniform-color pixels — `color` `CONSTANT`, `position` `PER_ITEM`,
2. 1M individually colored pixels — `color` `PER_ITEM` rgba,
3. 3 neuron populations of 1M spikes each — `color` `PER_GROUP`,
4. 1M pixels with scalar colormap — `color` `PER_ITEM` scalar,
5. streaming electrode positions — `position` mutability `streaming`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_pixel_position` | `position`, `PER_ITEM` |
| `dvz_pixel_color` | `color`, now also `CONSTANT`/`PER_GROUP` and `scalar` mode |
| `dvz_pixel_size` | `size` parameter |

v0.4 adds: `shift`, `size_space`, `color_mode = scalar`.


## Deferred Questions

1. the exact public API spelling for `color_mode` at creation.

`PER_GROUP` color uses item range partitions — no explicit per-item group-index attribute is
needed. Groups are contiguous ranges; the user declares group sizes and the scene maps each
item to its group by range.

Minimum supported `size`: 1 physical pixel. Maximum: unspecified, backend-dependent.

`z` participates in depth sorting by default when fragment alpha < 1.0. No explicit flag needed;
use `alpha_mode` on the visual to control the transparency path (see `TRANSPARENCY.md`).
