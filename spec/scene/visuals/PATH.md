# Visual Family: `path`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`path` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`path` renders connected polylines, each defined as an ordered sequence of vertices.

The primary semantic unit is one path (one connected sequence), not a single segment.
For independent unconnected segments with per-segment styling, use `segment`.

Typical uses: signal traces, time series, contour lines, trajectories, graph edges with bends,
scientific line plots.


## Item and Group Model

`path` is a **grouped visual**: each logical item is one path (a connected sequence of vertices).
The vertices within a path form the rendering primitive, not individual points.

This maps to a `GroupedItemTable`: items are paths, vertices are sub-items within each path.
The user provides:
- a flat array of vertex positions (across all paths, concatenated),
- a path count and per-path vertex count.

`PER_ITEM` attribute sources are indexed by vertex.
`PER_GROUP` attribute sources are indexed by path.

Picking returns the path index as item identity, not the vertex index.


## Per-Item (Per-Vertex) Attributes

Each entry in the flat vertex arrays corresponds to one vertex.

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Per-vertex color produces a gradient along the path.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.

`PER_GROUP` means one color per path (all vertices of a path share the same color).
`PER_ITEM` means one color per vertex, enabling along-path gradients.
`CONSTANT` means one color for all vertices of all paths.


### `linewidth`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_GROUP`.

Per-vertex linewidth is not supported — join geometry requires uniform width per path.
Use `segment` if per-segment width variation is needed.


## Visual-Wide Parameters

### `cap_start` and `cap_end`

| Property | Value |
|---|---|
| Type | enum — same cap type list as `segment` |
| Default | `round` for both |
| Mutability | `dynamic` |

Cap styles applied to the start and end of each open path independently.
Ignored for closed paths.

| Cap | Description |
|---|---|
| `none` | no cap |
| `round` | semicircular |
| `square` | rectangular extension |
| `butt` | flat exactly at endpoint |
| `triangle_out` | triangular pointing outward |
| `triangle_in` | triangular pointing inward (notch) |

Setting both to the same value is equivalent to a single cap parameter.
Use `cap_end = triangle_out` with `cap_start = butt` for directed paths (arrow at end only).


### `join`

| Property | Value |
|---|---|
| Type | enum: `miter`, `round`, `bevel` |
| Default | `round` |
| Mutability | `dynamic` |

Corner join style between consecutive segments of a path.

| Join | Description |
|---|---|
| `miter` | sharp corner, extends to intersection point |
| `round` | circular arc at the join |
| `bevel` | flat diagonal cut |

`miter` may produce very long spikes at near-180° angles.
The miter limit is controlled by `miter_limit` (see below).


### `miter_limit`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `4.0` |
| Mutability | `dynamic` |

Maximum miter length as a multiple of `linewidth`. When a miter join would exceed this limit,
the join falls back to `bevel` automatically. `4.0` is a standard SVG/PostScript default.
Set to a large value to disable the limit (allows arbitrarily long miter spikes).


### `closed`

| Property | Value |
|---|---|
| Type | `bool` |
| Default | `false` |
| Mutability | `static` — set at visual creation time |

When `true`, each path is closed: the last vertex connects back to the first.
Cap style is ignored for closed paths.

Cannot change at runtime. Paths in a `closed` visual are all closed; paths in an `open` visual
are all open.


### `linewidth_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `closed` | `true`, `false` | `false` |

`closed` is listed as a variant axis because it affects internal geometry generation, not just
a parameter value. Set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the path (group) index as item identity.
Sub-item (vertex) identity is not returned.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Independent segments with per-segment styling | `segment` |
| Raw lines, no caps or joins | `primitive` with `line_strip` |
| Single closed polygon | `path` with `closed = true` |


## Minimum Cases This Spec Must Support

1. single signal trace — one path, `color` `CONSTANT`, `linewidth` `CONSTANT`,
2. 20 overlaid signal traces — `color` `PER_GROUP`, `linewidth` `CONSTANT`,
3. per-vertex colored trajectory — `color` `PER_ITEM` rgba (gradient along path),
4. scalar-colored fiber bundle — `color` `PER_ITEM` scalar,
5. per-group-width contour lines — `linewidth` `PER_GROUP`,
6. closed polygon outlines — `closed = true`, `join = miter`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_path_position` with `path_lengths` | `position` `PER_ITEM` + group structure |
| `dvz_path_color` | `color`, extended sources and scalar mode |
| `dvz_path_linewidth` | `linewidth`, now also `PER_GROUP` |
| `dvz_path_cap` | `cap_start` + `cap_end` (split; both default `round`) |
| `dvz_path_join` | `join`, extended to `miter`/`round`/`bevel` |

v0.4 adds: `PER_GROUP` sources, `scalar` color mode, `linewidth_space`, `closed` variant axis.
v0.3 `join` had only `square`/`round`; v0.4 renames `square` to `bevel` and adds `miter`.


## Resolved Questions

- **Per-vertex `linewidth` (tapered lines)**: not supported in v0.4. Tapered lines require
  complex per-segment geometry generation and are reserved as a v0.4+ feature.

Partial path updates are served by the standard byte-range upload mechanism from
`THREAD_SAFETY.md` — no dedicated streaming API needed.
