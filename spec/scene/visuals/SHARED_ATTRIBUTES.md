# Shared Visual Attributes

This document defines attributes, parameters, and behavioral sections that recur across multiple
visual family specs.

Family specs reference this document instead of repeating these definitions.
When a family spec says "standard" for an attribute, the full definition is here.
Family specs only document deviations, restrictions, or extensions specific to that family.


## `color` Attribute

### Definition

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see `color_mode` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` or `streaming` |

### Color Modes

**`rgba` mode** (default): the user supplies 4-byte RGBA directly.
Right when color is pre-computed externally or when colors are heterogeneous and do not correspond
to a continuous scale.

**`scalar` mode**: the user supplies a single `float32`.
The scene maps it through an associated `Scale` object (see `SCALES.md`).
Enables dynamic colormap changes without re-uploading item data, and memory-efficient encoding
when color encodes a single continuous quantity.

`color_mode` is a variant axis set at visual creation time. It cannot change without recreating
the visual.

### Fallback

If the runtime cannot support GPU-side palette lookup, the scene falls back to CPU-side colormap
application at upload time and emits a capability adaptation diagnostic.


## `size` Attribute

### Definition

| Property | Value |
|---|---|
| Type | `float32` (direct) or `scalar_f32` (mapped) — see `size_mode` |
| Unit | determined by `size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` |

### Size Modes

**`direct` mode** (default): `float32` value in the unit defined by `size_space`.

**`scalar` mode**: `float32` scalar mapped through a size `Scale` object (see `SCALES.md`,
`kind = size`). The size scale supports `sqrt` interpolation so that perceived area scales
linearly with data value — useful for bubble charts.

`size_mode` is a variant axis set at visual creation time.

### `size_space` Parameter

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

**`screen`**: size in screen pixels, invariant under zoom.
Right for most scatter plots and annotations.

**`data`**: size in visual-space units, scales with zoom.
Right when marks represent physical objects with real spatial extent (cells, atoms, electrode
contacts).

Applies uniformly to all items in the visual.


## `linewidth` Attribute

### Definition

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | determined by `linewidth_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` |

### `linewidth_space` Parameter

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

Same semantics as `size_space`. Use `data` when lines represent physical structures with real
spatial width (anatomical fibers, vessel walls).


## `shift` Attribute

### Definition (single-anchor visuals)

For visuals with one anchor point per item (`pixel`, `point`, `marker`, `glyph`):

| Property | Value |
|---|---|
| Type | `vec2` — `(dx, dy)` in screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — defaults to `(0, 0)` |

### Definition (dual-endpoint visuals)

For visuals with two endpoints per item (`segment`):

| Property | Value |
|---|---|
| Type | `vec4` — `(dx0, dy0, dx1, dy1)` in screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Optional | yes — defaults to `(0, 0, 0, 0)` |

### Semantics

`shift` is applied in screen space after projection.

It is the right tool when an item must be placed at a precise data-space position AND offset by a
fixed pixel distance from it, regardless of zoom level.
Data-space nudging cannot achieve this because the pixel distance would change with zoom.

Typical uses: jitter plots, label nudges, aligning segment endpoints to marker centers,
zoom-invariant annotation offsets.

`PER_GROUP` is not supported because `shift` is inherently a per-item screen-space adjustment.


## `angle` Attribute

### Definition

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | radians, counter-clockwise from the right (+x) axis |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` |

### Convention

`angle = 0` means the mark points right.
`angle = π/2` means it points up.

This matches `atan2(dy, dx)` directly — a vector `(dx, dy)` in data space has angle
`atan2(dy, dx)` with no additional transformation.

The scene layer handles any coordinate-system adjustment for screen-Y-down rendering.
Users always work in data-space convention.


## Standard Transform Model

Unless a family spec states otherwise:

1. **Normalization** — data-space positions are normalized to visual space before upload.
   This stage runs on the CPU and does not repeat per-frame unless source data changes.
2. **Panel transform** — panel-local camera or panzoom is applied per-frame on the GPU.
   This does not require re-uploading position data.
3. **Screen-space adjustments** (`shift`) are applied after the panel transform.

Visual families do not support a per-visual transform matrix unless explicitly stated.


## Standard Picking Model

Unless a family spec states otherwise:

1. Picking is optional and enabled per visual.
2. A pick result returns `(panel_id, visual_id, item_index)`.
3. `item_index` identifies the logical item within the visual.
4. No sub-item identity is defined unless the family spec adds it.
5. Hover picking follows latest-request-wins semantics.


## Standard Stage Participation

Unless a family spec states otherwise:

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional |
| Offscreen / export | same as render |
