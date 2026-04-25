# Visual Family: `pixel`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`pixel` visual family.

It refines:

1. `VISUAL_FAMILIES.md` — family taxonomy and rationale
2. `VISUAL_MINI_CONTRACTS.md` — family-level mini-contract
3. `ATTRIBUTE_SOURCES.md` — attribute granularity and mutability vocabulary
4. `VISUAL_CONTRACT.md` — shared visual responsibilities


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


## Per-Item Attribute Schema

Each item in a `pixel` visual has the following attributes.

### `position`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

`x` and `y` are the primary placement coordinates.
`z` is available for depth ordering but is typically zero for 2D scenes.

Position is always per-item.
A single position for all items is not meaningful for this family.


### `color`

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color mode below |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |

Color is the primary style attribute and accepts all three granularities.

Two color modes are defined as a variant axis:

**`rgba` mode** (default): the user supplies color as 4-byte RGBA directly.
The scene uploads the value as-is.
This is the right choice when color is determined externally — for example, pre-computed from a
Python colormap call — or when colors are heterogeneous and do not correspond to a continuous scale.

**`scalar` mode**: the user supplies a single `float32` per item (or per group, or one constant).
The scene maps the scalar through an associated `Scale` object to produce the final color.
This enables:

1. dynamic colormap changes without re-uploading item data,
2. memory-efficient encoding when item color encodes a single continuous quantity,
3. shared color scales across multiple visuals and a linked colorbar.

When using `scalar` mode, an associated scale must be declared.
See `SCALES.md` for the scale contract.

The color mode is selected at visual creation time and cannot change without recreating the visual.


## Visual-Wide Parameters

These parameters apply to all items and are not per-item attributes.

### `size`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | determined by `size_space` |
| Default | implementation-defined, suggested 1.0 screen pixels |
| Mutability | `dynamic` |

The size of every pixel mark.
All items share the same size.
For per-item size, use the `point` family.

Size is a visual parameter, not an item attribute.
It is not subject to attribute source declarations.


### `size_space`

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

Controls whether `size` is interpreted in screen pixels or data-space units.

**`screen`** (default): size is invariant under zoom. Right for most use cases.

**`data`**: size scales with zoom. Right when pixels represent physical objects with real spatial
extent.

See `visuals/POINT.md` for a full discussion of this parameter.


### `shift`

| Property | Value |
|---|---|
| Type | `vec2` — two `float32` values `(dx, dy)` |
| Unit | screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — defaults to `(0, 0)` |

Screen-space pixel offset applied to each item's position after projection.

Useful when items must be placed at a precise data-space position AND a fixed pixel distance from
it, independently of zoom level. Data-space nudging cannot achieve this because the pixel distance
would change with zoom.

`shift` is a general concept across visual families. For `segment` it extends to `vec4` to
independently offset each endpoint.


## Color Mode Variant Axis

The `color_mode` variant axis has two values:

1. `rgba` — direct 4-byte RGBA color per item (or constant or per-group).
2. `scalar` — one `float32` scalar per item (or constant or per-group), mapped through a scale.

The default is `rgba`.

This is the only variant axis for `pixel`.
The family does not expose shape, topology, join, or edge variants.


## Transform Model

`pixel` uses the standard two-stage scene transform:

1. **Normalization** — data-space positions are normalized to visual space before upload.
   This stage is a scene concern and does not run per-frame unless source data changes.
2. **Panel transform** — panel-local panzoom or camera transform is applied per-frame.
   This does not require re-uploading position data.

For 2D scenes, `z` is typically zero and the panel transform is a 2D panzoom.
For 3D scenes, `z` participates in depth ordering under the panel camera.

`pixel` does not support a visual-local transform (a per-visual matrix).
If a visual-local offset or scale is needed, prefer applying it in the normalization stage or use
the `primitive` family.


## Stage Participation

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional |
| Offscreen / export | same as render |

Picking is optional.
When enabled, picking returns the item index as the primary identity.
Sub-item identity does not exist for this family.


## Picking Model

When picking is enabled:

1. each pixel item has a unique item index within the visual,
2. a pick result returns `(panel_id, visual_id, item_index)`,
3. no group or sub-item identity is defined.

Hover picking follows latest-request-wins semantics.


## Fallback Notes

`pixel` is already the simplest mark family.
Capability fallback pressure should be low.

If the runtime cannot support the declared `color_mode = scalar`, the scene may fall back to
`color_mode = rgba` by applying the colormap on the CPU at upload time and reporting a capability
adaptation diagnostic.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Need per-item size | `point` |
| Need shape, rotation, or edge treatment | `marker` |
| Need connected lines or paths | `path` or `segment` |
| Need low-level topology control | `primitive` |

`pixel` should not grow per-item size or shape controls.
Any such request is pressure toward `point` or `marker`.


## Minimum Cases This Spec Must Support

This spec is acceptable only if it can describe:

1. 1M uniform-color pixels at varying positions — `color` as `CONSTANT`, `position` as `PER_ITEM`,
2. 1M individually colored pixels — `color` as `PER_ITEM` with `rgba` mode,
3. 3 neuron populations of 1M spikes each with per-population color — `color` as `PER_GROUP`,
4. 1M pixels with scalar values mapped through a colormap — `color` as `PER_ITEM` with `scalar`
   mode,
5. streaming position updates (electrode array live data) — `position` mutability `streaming`.


## v0.3 Correspondence

In `v0.3`, the corresponding API was:

```c
dvz_pixel(batch, flags)
dvz_pixel_position(visual, first, count, positions, flags)
dvz_pixel_color(visual, first, count, colors, flags)
dvz_pixel_size(visual, size)
dvz_pixel_alloc(visual, item_count)
```

The v0.4 spec maps onto this as follows:

1. `position` → `dvz_pixel_position`, always `PER_ITEM`,
2. `color` in `rgba` mode → `dvz_pixel_color`, now also supporting `CONSTANT` and `PER_GROUP`,
3. `color` in `scalar` mode → new in v0.4, no direct v0.3 equivalent,
4. `size` → `dvz_pixel_size`, unchanged.

The v0.4 API will differ in naming and construction model but the data content should remain
recognizable to users familiar with v0.3.


## Deferred Questions

1. the exact public API spelling for declaring `color_mode` at visual creation,
2. whether `PER_GROUP` color with an `ItemTable` requires an explicit per-item group-index attribute
   or can be inferred from item range partitions,
3. the minimum and maximum supported `size` values across backends,
4. whether `z` participates in depth sorting by default or requires an explicit depth-sort flag.
