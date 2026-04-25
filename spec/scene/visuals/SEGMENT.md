# Visual Family: `segment`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`segment` visual family.

It refines:

1. `VISUAL_FAMILIES.md` — family taxonomy and rationale
2. `VISUAL_MINI_CONTRACTS.md` — family-level mini-contract
3. `ATTRIBUTE_SOURCES.md` — attribute granularity and mutability vocabulary
4. `VISUAL_CONTRACT.md` — shared visual responsibilities


## Semantic Purpose

`segment` renders independent line segments, each defined by two endpoints.

The primary semantic unit is one segment, not a connected sequence.
For connected sequences, use `path`.

Typical uses: error bars, rulers, graph edges, connection lines, axis ticks, crosshair guides,
vector field glyphs, anatomical fiber bundles rendered as individual segments.


## Per-Item Attribute Schema

Each item is one segment with two endpoints, P0 (start) and P1 (end).

### `P0`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | start endpoint `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `P1`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | end endpoint `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `color`

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color mode |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

The color of the segment, or the color at P0 when `color_end` is also set (gradient mode).
Two color modes: `rgba` (default) and `scalar` (mapped through a `Scale` — see `SCALES.md`).


### `color_end`

| Property | Value |
|---|---|
| Type | same as `color` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes — gradient is disabled when not set |

Color at the P1 endpoint.
When set, the segment is rendered with a linear color gradient from `color` (at P0) to `color_end`
(at P1).
When not set, the segment is uniformly colored by `color`.

`color_end` uses the same color mode as `color` — both must be `rgba` or both must be `scalar`.

Gradient segments are useful for encoding a second scalar channel along each segment's length — for
example, time-colored trajectories or signal-strength connection lines.


### `linewidth`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

Width of the segment in screen pixels.
`PER_ITEM` linewidth is the defining capability of `segment` over `primitive` line topologies.

`PER_GROUP` is useful for error-bar bundles where different groups carry different uncertainty
widths, or for multi-channel displays with channel-encoded linewidths.


### `shift`

| Property | Value |
|---|---|
| Type | `vec4` — four `float32` values `(dx0, dy0, dx1, dy1)` |
| Unit | screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — defaults to `(0, 0, 0, 0)` |

Screen-space pixel offsets applied to each endpoint after projection.

`shift` enables precise sub-pixel positioning relative to other visual elements — for example,
placing error bar caps exactly on top of marker centers without adjusting data-space coordinates.

`PER_GROUP` is not supported for `shift` because it is inherently a per-item screen-space
adjustment.


## Visual-Wide Parameters

### `cap_start`

| Property | Value |
|---|---|
| Type | enum — see cap type list |
| Default | `round` |
| Mutability | `dynamic` |

Cap style at the P0 endpoint.


### `cap_end`

| Property | Value |
|---|---|
| Type | enum — see cap type list |
| Default | `round` |
| Mutability | `dynamic` |

Cap style at the P1 endpoint.
May differ from `cap_start`.


### Cap Types

| Value | Description |
|---|---|
| `none` | no cap, segment ends at the endpoint coordinate |
| `round` | semicircular cap extending by half the linewidth |
| `square` | rectangular cap extending by half the linewidth |
| `butt` | flat cap exactly at the endpoint, no extension |
| `triangle_out` | triangular cap pointing outward from the endpoint |
| `triangle_in` | triangular cap pointing inward (arrow-like notch) |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Color mode is set at visual creation time.
`color_end` is an optional attribute that can be added or removed at any time — enabling or
disabling gradient mode does not require recreating the visual, but does require a re-upload of the
color data.


## Transform Model

Standard two-stage transform:

1. **Normalization** — data-space endpoint positions normalized to visual space before upload.
2. **Panel transform** — panel-local camera or panzoom applied per-frame without re-upload.
   `shift` is applied after the panel transform, in screen space.

`segment` does not support a visual-local transform matrix.


## Stage Participation

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional |
| Offscreen / export | same as render |


## Picking Model

When picking is enabled:

1. a pick result returns `(panel_id, visual_id, item_index)`,
2. item index identifies the segment.

No sub-segment identity (endpoint or midpoint) is defined.


## Fallback Notes

`segment` has no meaningful capability fallback beyond the standard `color_mode = scalar` CPU
path.

If per-item linewidth is not supported on a constrained backend, the scene may fall back to a
uniform linewidth using `CONSTANT` source and emit a diagnostic.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Connected sequence of points | `path` |
| Raw line topology, no caps or per-item width | `primitive` with `line_list` |
| Directional arrows with shaped heads | `marker` with `shape = arrow` |
| Many paths batched together | `path` |


## Minimum Cases This Spec Must Support

1. uniform error bars — `P0`, `P1` `PER_ITEM`, `color` `CONSTANT`, `linewidth` `CONSTANT`,
2. per-segment colored connection graph — `color` `PER_ITEM` rgba,
3. multi-group error bars with group-encoded widths — `linewidth` `PER_GROUP`,
4. time-colored trajectories — gradient mode, `color` and `color_end` both `PER_ITEM` scalar,
5. error bar caps precisely aligned to markers — `shift` `PER_ITEM`,
6. fiber bundle with per-fiber scalar FA coloring — `color` `PER_ITEM` scalar mode.


## v0.3 Correspondence

```c
dvz_segment(batch, flags)
dvz_segment_position(visual, first, count, initial, terminal, flags)
dvz_segment_color(visual, first, count, colors, flags)
dvz_segment_linewidth(visual, first, count, widths, flags)
dvz_segment_shift(visual, first, count, shifts, flags)
dvz_segment_cap(visual, cap_initial, cap_terminal)
```

| v0.3 | v0.4 |
|---|---|
| `initial`, `terminal` in `dvz_segment_position` | `P0`, `P1` attributes |
| `dvz_segment_color` | `color` attribute, now also `CONSTANT`/`PER_GROUP` and `scalar` mode |
| `dvz_segment_linewidth` | `linewidth` attribute, now also `CONSTANT`/`PER_GROUP` |
| `dvz_segment_shift` | `shift` attribute, unchanged |
| `dvz_segment_cap` | `cap_start`, `cap_end` parameters |

v0.4 adds: `color_end` for gradient segments (new), `PER_GROUP` source for `color` and
`linewidth`, `scalar` color mode.


## Deferred Questions

1. whether `linewidth` should support a `data` space mode (linewidth scales with zoom) — deferred,
   screen pixels is sufficient for the current use cases,
2. whether `shift` should support `PER_GROUP` source in a future version,
3. the exact public API spelling for `P0`/`P1` — whether they are two separate attribute calls or
   one interleaved call as in v0.3.
