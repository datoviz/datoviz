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
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color attribute format |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` or `streaming` |

### Color Attribute Formats

**`rgba_u8` format** (default): the user supplies 4-byte RGBA directly.
Right when color is pre-computed externally or when colors are heterogeneous and do not correspond
to a continuous scale.

**`scalar_f32` format**: the user supplies a single `float32`.
The scene maps it through an associated `Scale` object (see `semantics/SCALES.md`).
Enables dynamic colormap changes without re-uploading item data, and memory-efficient encoding
when color encodes a single continuous quantity.

The color attribute format is set with `dvz_visual_set_attr_format(visual, "color", format)` before
attaching dense data or an external buffer. It cannot change while a color payload is attached.

Scalar color attributes bind their continuous scale with
`dvz_visual_set_scale(visual, "color", scale)`. The scale slot is the semantic attribute name, not
a visual-family implementation name.

### Fallback

If the runtime cannot support GPU-side palette lookup, the scene falls back to CPU-side colormap
application at upload time.

The active point/pixel slice currently uses this fallback path. Retained point/pixel visuals may
store `scalar_f32` color values, but the point-like shaders still consume an `rgba_u8` vertex color
attribute. During scene lowering, dirty scalar color ranges are mapped through the bound continuous
scale on the CPU and emitted as derived RGBA uploads. Scale domain or colormap changes mark the
retained scalar color attribute dirty so the derived RGBA buffer is refreshed without changing user
data.

This is a compatibility layer for the current point-like pipelines, not the long-term fast path.
The intended optimization path is:

1. keep scalar color as the retained public contract;
2. realize shared scale/colormap transfer resources in the scene layer;
3. add shader variants that consume scalar vertex attributes and sample a 1D colormap resource;
4. reuse the same scale-backed color machinery for mesh, path, sphere, and other scalar-colored
   visual families.


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

**`scalar` mode**: `float32` scalar mapped through a size `Scale` object (see `semantics/SCALES.md`,
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


## `stroke_width_px` Attribute

### Definition

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | determined by `stroke_width_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` (unless family restricts) |
| Typical mutability | `dynamic` |

`stroke_width_px` is the public visual-contract name for screen-space and data-space strokes.
Some first-slice implementation internals still use the historical `line_width` storage/resource
name; specs, public API docs, and examples should use `stroke_width_px`.

### `stroke_width_space` Parameter

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


## Standard Lighting Parameters

Used by families that support Phong shading (`mesh`, `sphere`).
Applies when the family's `lighting` variant axis is set to `phong`.
Ignored when `lighting = flat`.

### `light_pos`

| Property | Value |
|---|---|
| Type | array of up to 4 `vec4` — `(x, y, z, w)`, `w = 0` directional, `w = 1` point light |
| Default | single directional light at `(1, 1, 1, 0)` |
| Mutability | `dynamic` |

### `light_color`

| Property | Value |
|---|---|
| Type | array of up to 4 `rgba_u8` |
| Default | white `(255, 255, 255, 255)` |
| Mutability | `dynamic` |

### `ambient`, `diffuse`, `specular`

| Property | Value |
|---|---|
| Type | `float32` in `[0, 1]` each |
| Default | `0.2`, `0.7`, `0.3` |
| Mutability | `dynamic` |

Material reflection coefficients for the Phong model.

### `shininess`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `32.0` |
| Mutability | `dynamic` |

Phong specular exponent. Higher values produce tighter highlights.

### `emissive`

| Property | Value |
|---|---|
| Type | `float32` in `[0, 1]` |
| Default | `0.0` |
| Mutability | `dynamic` |

Self-emission factor. At `1.0` the surface appears fully lit regardless of light positions.


## Standard Stage Participation

Unless a family spec states otherwise:

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional |
| Offscreen / export | same as render |


## Missing-Value Policy

These rules apply to all visual families unless the family spec overrides them.

**NaN positions**: an item whose `position` contains any NaN component is skipped — it is not
rendered and not pickable. No error is emitted.

**NaN scalar color or size**: when a mapped scalar value is NaN, the scale's missing-value color
(for color scales) or fallback size (for size scales) is used. The default missing-value color
is a configurable field on the scale:

```c
dvz_scale_set_missing_color(scale, rgba_u8_value);  // default: transparent black (0,0,0,0)
```

**Inf coordinates**: treated as NaN for the purposes of rendering (item skipped). A validation
diagnostic is emitted at `DVZ_DIAG_WARN` severity if strict validation is enabled.

**NaN scalar in texture**: for scalar textures with a color scale, NaN maps to the scale's
missing-value color, same as per-item scalar attributes.

**Default fallback**: when an optional attribute is entirely absent and the family spec defines
a default for it, the visual's default parameter value is used. Users may override the default
with `dvz_visual_set_param(visual, attr_name, value)`.


## Visual Defaults Contract

Every visual family has defined default values for all optional attributes and visual-wide
parameters. These defaults are documented in each family's spec.

Status on 2026-05-17: `DvzStyle` remains a future convenience layer, not an installed public API.
The active implementation uses direct per-visual data, parameter, material, and technique setters.
Keep style semantics here as the retained design target for shared defaults.

When the user does not set an optional attribute or parameter:

1. the visual uses the documented default value,
2. the default is applied silently — no warning is emitted,
3. the default may be overridden per visual at any time.

A `DvzStyle` object may be attached to a visual to override a group of defaults in one call:

```c
DvzStyle* style = dvz_style(scene);
dvz_style_set_param(style, "stroke_width_px", &lw);
dvz_style_set_param(style, "color",     &col);
dvz_visual_set_style(visual, style);
```

Multiple visuals may share the same `DvzStyle*`. When a style parameter changes, all attached
visuals are marked dirty for that parameter. The style object is scene-owned and released with
`dvz_style_destroy(style)`.

`DvzStyle` is a convenience layer over `dvz_visual_set_param`. It does not change which
parameters exist — only how they are grouped and shared.
