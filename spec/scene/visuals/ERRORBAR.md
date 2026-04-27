# Visual Family: `errorbar`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`errorbar` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`errorbar` renders per-item error indicators: vertical or horizontal bars extending from a center
position to mark uncertainty, spread, or confidence intervals.

Each item is one measurement point with optional lower and upper extents along each axis.
Bars are rendered as line segments with optional end caps.

Typical uses: statistical plots (mean ± SD, median with IQR), measurement uncertainty in
scientific figures, confidence intervals, data quality indicators.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` center position in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Center anchor of the error bar. The bar extends from `position + low` to `position + high`
along the configured axis.


### `x_low`, `x_high`

| Property | Value |
|---|---|
| Type | `float32`, offset from `position.x` (negative for low, positive for high) |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — no horizontal bar if both are absent |

Horizontal error extents. `x_low` is typically negative (leftward), `x_high` positive.
When only one is provided, the bar is one-sided.


### `y_low`, `y_high`

| Property | Value |
|---|---|
| Type | `float32`, offset from `position.y` (negative for low, positive for high) |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — no vertical bar if both are absent |

Vertical error extents. `y_low` is typically negative (downward), `y_high` positive.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
Applies to all line segments of the error bar for a given item.


## Visual-Wide Parameters

### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.5` |
| Mutability | `dynamic` |

Width of the error bar lines. Visual-wide.


### `cap_size`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `6.0` |
| Mutability | `dynamic` |

Half-width of the flat end caps drawn at the tips of each bar.
Set to `0` to disable caps.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the item index (the measurement point identity).


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Raw line segments with per-item styling | `segment` |
| Box-and-whisker plots | `boxplot` |
| Error bars as part of a scatter plot | `errorbar` overlaid on `point` or `marker` |


## Minimum Cases This Spec Must Support

1. vertical-only error bars — `y_low` and `y_high` `PER_ITEM`, no x extents,
2. horizontal-only error bars — `x_low` and `x_high` `PER_ITEM`,
3. asymmetric error bars — `y_low ≠ y_high` per item,
4. group-colored error bars — `color` `PER_GROUP`,
5. no-cap error bars — `cap_size = 0`.


## v0.3 Correspondence

`errorbar` is a new family in v0.4. There is no direct v0.3 equivalent.
In v0.3, error bars were typically assembled from `segment` primitives by the user.


## Deferred Questions

1. whether `cap_size` should be per-item,
2. whether `linewidth` should be per-item or per-group,
3. whether `z_low`/`z_high` for 3D error bars should be supported.
