# Visual Family: `segment`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`segment` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.
Durable family-level decisions should live here. `IMPLEMENTATION_DECISIONS.md` records the landed
first-slice history when useful, but this document is authoritative for `segment` semantics.


## Current Implementation Status

Status on 2026-05-17: the active v0.4 runtime implements the first retained segment slice.

The implemented segment supports:

1. retained `segment` visual construction via `dvz_segment()`;
2. dense `position_start`, `position_end`, `color`, and public `stroke_width` attributes;
3. screen-space analytic stroked segments derived from the v0.3 four-vertex/six-index technique;
4. caps `none`, `round`, `triangle_in`, `triangle_out`, `square`, and `butt`;
5. `dvz_segment_set_caps()`, with `butt` as the default at both ends;
6. GLSL/Vulkan frame-plan and DRP2 emission through the segment pipeline;
7. GPU-backed item picking against the rendered stroke.

`stroke_width` is the public attribute name. The current retained storage, shader input, and DRP2
resource metadata still use the historical internal name `line_width`.

The following sections describe the target segment contract. Dashes, arrow caps, endpoint shifts,
`color_end` gradients, scalar/grouped color, data-space stroke width, exact cap-aware picking, and
WGSL segment lowering are planned capabilities unless explicitly marked as implemented above.


## Semantic Purpose

`segment` renders independent line segments, each defined by two endpoints.

The primary semantic unit is one segment, not a connected sequence.
For connected sequences, use `path`.

Typical uses: error bars, rulers, graph edges, connection lines, axis ticks, crosshair guides,
vector field glyphs, anatomical fiber bundles.


## Per-Item Attributes

Each item is one segment with two endpoints, P0 (start) and P1 (end).

### `P0`

| Property | Value |
|---|---|
| Type | `vec3`, start endpoint `(x, y, z)` in visual space |
| Attribute name | `"position_start"` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `P1`

| Property | Value |
|---|---|
| Type | `vec3`, end endpoint `(x, y, z)` in visual space |
| Attribute name | `"position_end"` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

The two endpoints are written as separate `dvz_visual_set_data` calls:

```c
dvz_visual_set_data(seg, "position_start", p0_array, n);
dvz_visual_set_data(seg, "position_end",   p1_array, n);
```


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Color at P0, or uniform color when `color_end` is not set.
Accepted sources: `CONSTANT`, `PER_ITEM` in the active descriptor. `PER_GROUP` is a target
capability.


### `color_end`

| Property | Value |
|---|---|
| Type | same as `color` |
| Accepted sources | `CONSTANT`, `PER_ITEM`; target `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes — gradient disabled when not set |

Color at P1. When set, a linear gradient is drawn from `color` (P0) to `color_end` (P1).
Must use the same `color_mode` as `color`.

Useful for time-colored trajectories, FA-colored fibers, signal-strength connection lines.


### `stroke_width`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM` in the active descriptor. `PER_GROUP` is a target
capability.
Per-item stroke width is the defining capability of `segment` over `primitive` line topologies.

Status on 2026-05-27: the active slice supports constant and dense per-item `stroke_width`.


### `shift`

Standard `vec4` (dual-endpoint form) — see `SHARED_ATTRIBUTES.md`.
`(dx0, dy0, dx1, dy1)` in screen pixels.
Useful for aligning segment endpoints precisely to marker centers.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP` in the target contract.

Status on 2026-05-27: `shift` is not installed in the active descriptor.


## Visual-Wide Parameters

### `cap_start` and `cap_end`

| Property | Value |
|---|---|
| Type | enum — see cap type list below |
| Default | `butt` |
| Mutability | `dynamic` |

Cap style at P0 and P1 respectively. May differ.
Arrow cap types use the shared `DvzArrowStyle` enum, which is also used by the `marker` family
for quiver-plot arrow markers.

| Cap | Description |
|---|---|
| `none` | no cap, ends at the endpoint coordinate |
| `round` | semicircular, extends by half stroke width |
| `square` | rectangular, extends by half stroke width |
| `butt` | flat exactly at endpoint, no extension |
| `triangle_out` | triangular cap pointing outward |
| `triangle_in` | triangular cap pointing inward (notch) |
| `arrow_filled` | filled (solid) arrowhead — `DVZ_ARROW_FILLED` |
| `arrow_open` | open arrowhead (two lines forming a V) — `DVZ_ARROW_OPEN` |
| `arrow_stealth` | swept-back stealth/chevron arrowhead — `DVZ_ARROW_STEALTH` |
| `arrow_circle` | circular cap with arrowhead semantics — `DVZ_ARROW_CIRCLE` |

`arrow_*` caps extend beyond the endpoint by a size proportional to `stroke_width`.

Status on 2026-05-17: arrow caps are not implemented. The active defaults are `cap_start = butt`
and `cap_end = butt`.


### `stroke_width_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `P0`, `P1` | required | NaN/Inf endpoint skips segment and picking | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `color_end` | inherits `color` | scalar NaN uses scale missing color | yes |
| `stroke_width` | family-defined screen width | NaN falls back to default | yes |
| `shift` | `(0, 0, 0, 0)` | NaN component treated as zero shift | yes |
| `cap_start`, `cap_end` | directional default described above | n/a | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time.
`color_end` can be added or removed at runtime without recreating the visual.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
`shift` is applied after the panel transform in screen space.
Picking returns the segment index as item identity.

Status on 2026-05-27: segment item picking is installed for the GPU query path. Richer precision
against every cap style remains follow-up work.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Connected sequence | `path` |
| Raw lines, no caps or per-item width | `primitive` with `line_list` |
| Directional arrows | `marker` with `shape = arrow` |


## Minimum Cases This Spec Must Support

1. uniform error bars — `P0`/`P1` `PER_ITEM`, `color` `CONSTANT`, `stroke_width` `CONSTANT`,
2. per-segment colored graph edges — `color` `PER_ITEM` rgba,
3. multi-group error bars with group-encoded widths — target `stroke_width` `PER_GROUP`,
4. time-colored trajectories — gradient, `color` and `color_end` both `PER_ITEM` scalar,
5. error bar caps aligned to markers — `shift` `PER_ITEM`,
6. fiber bundle with scalar FA coloring — `color` `PER_ITEM` scalar.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `initial`/`terminal` in `dvz_segment_position` | `P0`, `P1` |
| `dvz_segment_color` | `color`, extended sources and scalar mode |
| `dvz_segment_linewidth` | `stroke_width`, extended sources |
| `dvz_segment_shift` | `shift` (`vec4`) |
| `dvz_segment_cap` | `cap_start`, `cap_end` |

v0.4 target adds: `color_end` (gradient), `PER_GROUP` sources, `scalar` color mode,
`stroke_width_space`.
