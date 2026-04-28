# Visual Family: `boxplot`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`boxplot` visual family.

It refines `VISUAL_FAMILIES.md`, `../VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`boxplot` renders per-item statistical summary shapes consisting of a box body and whiskers.

Each item is one distribution summary defined by five scalar values (whisker low, box low,
median, box high, whisker high) at a given position.

The `DVZ_BOXPLOT_UNIFORM` variant renders classic box-and-whisker plots as used in statistical
visualization. The `DVZ_BOXPLOT_DIRECTIONAL` variant renders OHLC / candlestick shapes as used in
financial time-series visualization, where the box body encodes open and close prices and
whisker tips encode high and low prices.

Typical uses: statistical distribution summaries, inter-quartile range plots, financial OHLC
charts, candlestick charts.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` center position in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

The x position of the box center. The y axis carries the five statistical values.


### `whisker_low`

| Property | Value |
|---|---|
| Type | `float32`, y value in data space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Lower whisker tip. Rendered as a horizontal end cap connected to `box_low` by a vertical line.
In `DVZ_BOXPLOT_DIRECTIONAL` (candlestick): low price of the period.


### `box_low`

| Property | Value |
|---|---|
| Type | `float32`, y value in data space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Bottom of the filled box body.
In statistical plots: first quartile (Q1).
In candlestick: open or close price (whichever is lower).


### `box_high`

| Property | Value |
|---|---|
| Type | `float32`, y value in data space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Top of the filled box body.
In statistical plots: third quartile (Q3).
In candlestick: open or close price (whichever is higher).


### `whisker_high`

| Property | Value |
|---|---|
| Type | `float32`, y value in data space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Upper whisker tip. Rendered as a horizontal end cap connected to `box_high` by a vertical line.
In candlestick: high price of the period.


### `median`

| Property | Value |
|---|---|
| Type | `float32`, y value in data space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — omitted in `DVZ_BOXPLOT_DIRECTIONAL` |

Median line drawn across the box body.
Not used in `DVZ_BOXPLOT_DIRECTIONAL` (candlestick).


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Fill color of the box body.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.

In `DVZ_BOXPLOT_DIRECTIONAL`, the box color should encode direction:
- up-candle (close ≥ open): typically green,
- down-candle (close < open): typically red.
The caller sets `color` per item accordingly.


## Visual-Wide Parameters

### `width`

| Property | Value |
|---|---|
| Type | `float32`, in data-space x units or screen pixels depending on `width_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | `0.8` (data-space units) |
| Mutability | `dynamic` |

Horizontal width of the box body. `PER_ITEM` enables variable-width boxes (e.g., proportional
to sample size or bin count).


### `width_space`

| Property | Value |
|---|---|
| Type | enum: `data`, `screen` |
| Default | `data` |
| Mutability | `dynamic` |

Whether `width` is measured in data space (scales with zoom) or screen space (fixed pixel width).


### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.5` |
| Mutability | `dynamic` |

Width of all outlines and whisker lines.


### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Outline color for the box body and whisker lines. Visual-wide.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `style` | `DVZ_BOXPLOT_UNIFORM`, `DVZ_BOXPLOT_DIRECTIONAL`, `DVZ_BOXPLOT_NOTCHED` | `DVZ_BOXPLOT_UNIFORM` |
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time.

`DVZ_BOXPLOT_UNIFORM` renders a classical box plot (median line visible, symmetric whiskers).
`DVZ_BOXPLOT_DIRECTIONAL` renders a candlestick / OHLC bar (no median, directional body
coloring per item, open/close body with high/low wicks).
`DVZ_BOXPLOT_NOTCHED` renders a notched box plot: the box body has a confidence-interval notch
centered on the median. The notch width is derived from the IQR and item count via the standard
formula `1.58 × IQR / √n`. A `notch_width` per-item attribute carries `√n` (or the full notch
half-width directly when `notch_width_mode = absolute`).


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the item index.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Simple error bars (mean ± SD) | `errorbar` |
| Distribution summaries (median, IQR) | `boxplot` with `DVZ_BOXPLOT_UNIFORM` |
| Financial OHLC / candlestick | `boxplot` with `DVZ_BOXPLOT_DIRECTIONAL` |
| Raw segments with caps | `segment` |


## Minimum Cases This Spec Must Support

1. classical box plot — `style = DVZ_BOXPLOT_UNIFORM`, `median` `PER_ITEM`, `color` `CONSTANT`,
2. per-box colored distribution plot — `color` `PER_ITEM`,
3. group-colored box sets — `color` `PER_GROUP`,
4. financial candlestick chart — `style = DVZ_BOXPLOT_DIRECTIONAL`, `color` `PER_ITEM` (up/down),
5. no-median outline-only box — `median` absent, `color` transparent.


## v0.3 Correspondence

`boxplot` is a new family in v0.4. There is no direct v0.3 equivalent.
In v0.3, box plots were assembled from `segment` and `primitive` primitives by the user.


Violin plots require a separate family (different geometry: density outline, not a fixed
five-number summary) and are not part of `boxplot`.
