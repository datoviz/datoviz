# Visual Family: `path`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`path` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.
Durable family-level decisions should live here. `IMPLEMENTATION_DECISIONS.md` records the landed
first-slice history when useful, but this document is authoritative for `path` semantics.


## Current Implementation Status

Status on 2026-05-21: the active v0.4 runtime implements both the earlier line-strip path and the
path-native stroked path slice.

The implemented path supports:

1. retained `path` visual construction via `dvz_path()`;
2. dense `position` and `color` attributes;
3. primitive line-strip rendering when `stroke_width` is absent;
4. optional dense per-point `stroke_width` in screen pixels;
5. `dvz_path_set_subpaths()` for explicit open subpath lengths in the stroked path lane;
6. `dvz_path_set_caps()` and `dvz_path_set_join()` with round caps/joins by default and a default
   miter limit of `4.0`;
7. repeated first/last points in a stroked subpath are treated as a closed-ring sentinel for
   adjacency and cap suppression, which is the current polygon-stroke path;
8. stroked lowering through path-native adjacency resources carrying previous/current/next
   positions, color, internal `line_width`, per-vertex role/subpath flags, cumulative distance, and
   indices when `stroke_width` is present;
9. GLSL/Vulkan frame-plan and DRP2 emission through the `scene.path` stroke pipeline for stroked
   paths, while `segment` remains the independent endpoint-pair stroke pipeline;
10. GPU-backed item picking for stroked paths.

`stroke_width` is the public attribute name. The current retained storage and shader width input
still use the historical internal name `line_width`.

Current limitations:

1. thin line-strip paths do not yet consume explicit subpath lengths;
2. a first-class closed-path API, dashes, path/span identity picking, SVG parsing, filled
   paths/polygons, data-space stroke width, analytic curve tessellation helpers, and WGSL lowering
   are deferred.

The following sections describe the target path contract. Closed subpaths, dashes, filled
paths/polygons, SVG parsing, path/span picking, data-space stroke width, and fuller backend parity are
planned capabilities unless explicitly marked as implemented above.


## Semantic Purpose

`path` renders connected polylines, each defined as an ordered sequence of vertices.

The primary semantic unit is one path (one connected sequence), not a single segment.
For independent unconnected segments with per-segment styling, use `segment`.

Typical uses: signal traces, time series, contour lines, trajectories, graph edges with bends,
scientific line plots.


## Analytic Curve Lowering

`path` remains a polyline/span visual. Analytic curves such as Bezier, Catmull-Rom, or B-spline
inputs are prepared by CPU-side geometry utilities, which tessellate control data into ordinary
path vertices before upload.

The intended user workflow is:

1. call a `dvz_tessellate_*` geometry helper to produce F64 polyline vertices;
2. downcast or upload through the normal scene path as the `"position"` attribute;
3. set `"color"`, `"stroke_width"`, caps, joins, and subpath lengths on `dvz_path()`.

This keeps curve rendering on the same `path` frame-plan, DRP2, picking, styling, and backend
adaptation path as authored polylines. Datoviz should not add a separate `curve` visual for the
v0.4 baseline.

Curves that require a physical radius, surface normals, corrected tube depth, material/SSAO
participation, ribbons, or tube-specific rendering modes should use the future `tube` family rather
than extending `path` beyond screen-space strokes.


## Item and Span Model

`path` is a **span-structured visual**: each logical span is one path (a connected sequence of
vertices). The vertices within a path form the rendering primitive, not individual points.

This maps to a `GroupedItemTable`: spans are paths, items are vertices within each path.
The user provides:
- a flat array of vertex positions (across all paths, concatenated),
- a path count and per-path vertex count via `dvz_path_set_subpaths()`.

`PER_ITEM` attribute sources are indexed by vertex.
`PER_SPAN` attribute sources are indexed by path.

The target picking model returns the path (span) index as item identity, not the vertex index.


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
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_SPAN`, `PER_GROUP`.

`PER_SPAN` means one color per path (all vertices of a path share the same color).
`PER_ITEM` means one color per vertex, enabling along-path gradients.
`CONSTANT` means one color for all vertices of all paths.


### `stroke_width`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM` in the active descriptor. `PER_SPAN` is a target
capability.

Per-point stroke width enables tapered paths. The active first slice averages neighbouring endpoint
widths when lowering each path edge to the segment stroke pipeline.


## Visual-Wide Parameters

### `cap_start` and `cap_end`

| Property | Value |
|---|---|
| Type | enum — same cap type list as `segment` |
| Default | `round` for both in the target contract |
| Mutability | `dynamic` |

Cap styles applied to the start and end of each open path independently.
Ignored for closed paths.

Status on 2026-05-21: path-specific cap parameters are implemented for open stroked paths in the
GLSL/Vulkan path pipeline.

The cap and join vocabulary below is the focused home for the useful path enum sketch from the
retired broad scene API draft. Arrow-style caps may be added here when the `path` or `segment`
families grow rendered caps.

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

Status on 2026-05-21: path-specific join parameters are implemented for open stroked paths in the
GLSL/Vulkan path pipeline.

`miter` may produce very long spikes at near-180° angles.
The miter limit is controlled by `miter_limit` (see below).


### `miter_limit`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `4.0` |
| Mutability | `dynamic` |

Maximum miter length as a multiple of `stroke_width`. When a miter join would exceed this limit,
the join falls back to `bevel` automatically. `4.0` is a standard SVG/PostScript default.
Set to a large value to disable the limit (allows arbitrarily long miter spikes).

When target `stroke_width` `PER_SPAN` support is installed, the miter limit is evaluated per-path
using that path's own stroke width — a path with a wider stroke uses its width as the reference
multiple.

Status on 2026-05-21: path-specific miter-limit handling is implemented for GLSL/Vulkan stroked
paths.


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

Status on 2026-05-25: a repeated first/last point in the stroked path lane is treated as a closed
ring for adjacency and cap suppression. A first-class closed-path API is not implemented yet.
`dvz_path_set_subpaths()` still records point counts only.


### `stroke_width_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf vertex breaks or skips the affected path segment | no |
| subpath lengths | optional | invalid lengths are validation errors | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `stroke_width` | family-defined screen width | NaN falls back to default | yes |
| `cap_start`, `cap_end`, `join` | defaults described above | n/a | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `closed` | `true`, `false` | `false` |

`closed` is listed as a variant axis because it affects internal geometry generation, not just
a parameter value. Set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
The target contract returns the path span index as item identity. Sub-item vertex identity is not
returned.

Status on 2026-05-27: GPU-backed stroked-path item picking is installed, but richer path/span
identity is still deferred.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Independent segments with per-segment styling | `segment` |
| Raw lines, no caps or joins | `primitive` with `line_strip` |
| Single closed polygon | `path` with `closed = true` |


## Streaming And Partial Updates

Partial path updates (e.g., streaming new vertices into an existing path without re-uploading
the full buffer) use the standard byte-range upload mechanism from `integration/THREAD_SAFETY.md`. No
dedicated streaming API is needed.


## Minimum Cases This Spec Must Support

1. single signal trace — one path, `color` `CONSTANT`, `stroke_width` `CONSTANT`,
2. 20 overlaid signal traces — `color` `PER_SPAN`, `stroke_width` `CONSTANT`,
3. per-vertex colored trajectory — `color` `PER_ITEM` rgba (gradient along path),
4. scalar-colored fiber bundle — `color` `PER_ITEM` scalar,
5. per-path-width contour lines — target `stroke_width` `PER_SPAN`,
6. closed polygon outlines — `closed = true`, `join = miter`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_path_position` with `path_lengths` | `position` `PER_ITEM` + group structure |
| `dvz_path_color` | `color`, extended sources and scalar mode |
| `dvz_path_linewidth` | `stroke_width`, active `PER_ITEM`, target `PER_SPAN` |
| `dvz_path_cap` | `cap_start` + `cap_end` (split; both default `round`) |
| `dvz_path_join` | `join`, extended to `miter`/`round`/`bevel` |

v0.4 adds: `PER_SPAN` color sources, target `PER_SPAN` stroke widths, scalar color mode,
`stroke_width_space`, and closed subpath support.
v0.3 `join` had only `square`/`round`; v0.4 renames `square` to `bevel` and adds `miter`.
