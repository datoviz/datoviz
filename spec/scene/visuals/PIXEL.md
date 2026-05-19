# Visual Family: `pixel`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`pixel` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.
Landed naming and first-slice decisions are tracked in `IMPLEMENTATION_DECISIONS.md`.


## Current Implementation Status

Status on 2026-05-17: the active v0.4 runtime implements the first retained pixel slice.

The implemented path supports:

1. retained `pixel` visual construction via `dvz_pixel()`;
2. dense `position`, `color`, and `size` attributes, where `size` is per item and measured in
   screen pixels;
3. GLSL/Vulkan lowering as native square point-list sprites;
4. WGSL/WebGPU lowering as instanced quads, preserving the same public visual contract;
5. depth-cue pipeline switching for pixel visuals;
6. GPU-backed square picking through the scene request path;
7. offscreen/app smoke coverage proving square marks render nonblank pixels.

The following sections describe the target pixel contract. Constant/default size storage, `shift`,
scalar color and scale binding, grouped color, and data-space pixel size are planned capabilities
unless explicitly marked as implemented above.


## Semantic Purpose

`pixel` renders filled square pixel-like marks at given positions.

It is intentionally the simplest mark family:

1. no shape selection,
2. no per-item rotation,
3. no edge or stroke treatment,
4. no antialiased or analytic non-square coverage.

`pixel` is the right choice when the user needs to render many items efficiently and uniform mark
shape is acceptable.
For circular, shaped, rotated, or edge-styled marks, use `point` or `marker`.


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

`PER_GROUP` color requires a per-item integer `"group_id"` attribute because `pixel` is a flat
`ItemTable` visual. Contiguous range grouping belongs to span-structured visuals, not to `pixel`.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.

Status on 2026-05-17: not implemented in the active pixel slice.


### `size`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Accepted sources | `PER_ITEM` only in the active first slice |
| Typical mutability | `dynamic` or `streaming` |

Side length of each square mark.


## Visual-Wide Parameters

### `size`

| Property | Value |
|---|---|
| Type | `float32`, unit determined by `size_space` |
| Default | implementation-defined, suggested 1.0 screen pixels |
| Mutability | `dynamic` |

Size of every pixel mark. All items share the same size.
Minimum supported size: 1 physical pixel. Maximum: unspecified, backend-dependent.

Status on 2026-05-17: this visual-wide/default size convenience is not implemented yet. The active
slice uses dense per-item `size` data.

### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.

Implementation status on 2026-05-16: `size_space = data` is a valid pixel-family contract for
zoom-scaled cell-like squares or dense regular marks. It may be implemented by expanding each item
to a quad after projection. Backends that only support screen-pixel pixels must report that
limitation instead of silently treating data-space sizes as screen-space sizes.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf item skipped and not pickable | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `shift` | `(0, 0)` | NaN component treated as zero shift | yes |
| `size` | `1 px` | invalid or NaN size falls back to default | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time by passing flags to `dvz_pixel(scene, flags)`:
- `DVZ_COLOR_RGBA` (default) or `DVZ_COLOR_SCALAR` (scalar input mapped via a `DvzScale`).


## Depth And Transparency

`z` participates in depth sorting when fragment alpha < 1.0. No explicit flag is needed;
use `alpha_mode` on the visual to control the transparency path (see `../semantics/TRANSPARENCY.md`).


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
