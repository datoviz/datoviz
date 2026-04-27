# Visual Family: `marker`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`marker` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`marker` renders shaped point-like marks with full visual styling: shape, rotation, fill, stroke,
and edge treatment.

Richer than `point` (adds shape, angle, edge color, edge width).
Right for categorical scatter plots needing symbolic differentiation, directional indicators,
flow-field glyphs, and styled annotations.


## Render Mode

Primary variant axis. Determines how the mark shape is produced and which resources are required.

| Mode | Description | Texture required |
|---|---|---|
| `code` | shape from built-in shader SDF | no |
| `bitmap` | shape from RGBA raster texture | yes (RGBA u8) |
| `sdf` | shape from single-channel SDF texture | yes (float) |
| `msdf` | shape from multi-channel SDF texture | yes (RGBA float) |

`code` is the default. `msdf` is preferred over `sdf` and `bitmap` for custom shapes.
Set at visual creation time.


## Built-In Shapes (`code` mode)

| Shape | | Shape | | Shape | |
|---|---|---|---|---|---|
| `disc` | filled circle | `square` | axis-aligned square | `diamond` | rotated square |
| `circle` | ring | `triangle` | equilateral triangle | `cross` | plus sign |
| `asterisk` | six-pointed | `chevron` | V-shape | `clover` | four-lobed |
| `club` | club suit | `spade` | spade suit | `heart` | heart suit |
| `arrow` | directional | `ellipse` | ellipse | `hbar` | horizontal bar |
| `vbar` | vertical bar | `ring` | thick ring | `pin` | map pin |
| `tag` | label shape | `rounded_rect` | rounded rectangle | | |

Shape is visual-wide in `code` mode. All items share the same shape.
For per-item shape variation, use multiple marker visuals or the `glyph` family.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Fill color of the marker body.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.


### `size`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.


### `angle`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
Applied in screen space after the panel transform — markers always face the viewer.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.


### `magnitude`

| Property | Value |
|---|---|
| Type | `float32` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |
| Optional | yes — defaults to `1.0` (no scaling) |

Multiplicative scale applied to `size` per item, after all other size transformations.
Primary use: quiver plots, where `size` sets the base arrow length and `magnitude` encodes
the field strength at each position.
When combined with `shape = arrow` and `angle` `PER_ITEM`, produces a standard 2D vector
field visualization.


## Visual-Wide Parameters

### `shape`

| Property | Value |
|---|---|
| Type | enum — see built-in shape list |
| Default | `disc` |
| Mutability | `dynamic` |
| Applies to | `code` mode only |


### `aspect`

| Property | Value |
|---|---|
| Type | enum: `filled`, `stroke`, `outline` |
| Default | `filled` |
| Mutability | `dynamic` |

- `filled`: solid fill, no visible edge.
- `stroke`: edge only, no fill. Width controlled by `linewidth`.
- `outline`: filled body with edge on top. Uses both `color` (fill) and `edgecolor` (edge).


### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | white `(255, 255, 255, 255)` |
| Mutability | `dynamic` |

Edge color for `aspect = stroke` or `aspect = outline`. Visual-wide.


### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.0` |
| Mutability | `dynamic` |

Edge width for `aspect = stroke` or `aspect = outline`.


### `arrow_style`

| Property | Value |
|---|---|
| Type | `DvzArrowStyle` enum — see `DvzArrowStyle` below |
| Default | `arrow_filled` |
| Mutability | `dynamic` |
| Applies to | `shape = arrow` only |

Controls the arrowhead style when `shape = arrow`. Uses the shared `DvzArrowStyle` enum,
the same set used by `segment` cap types.

| Value | Description |
|---|---|
| `DVZ_ARROW_FILLED` | solid filled arrowhead |
| `DVZ_ARROW_OPEN` | open arrowhead (two lines) |
| `DVZ_ARROW_STEALTH` | swept-back / chevron arrowhead |
| `DVZ_ARROW_CIRCLE` | circular arrowhead |

Ignored when `shape` is not `arrow`.


### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.


### `tex_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |
| Applies to | `msdf`, `sdf`, `bitmap` modes |

Reference size at which the texture was generated (typically the texture width in pixels).
Tells the shader how to scale SDF distances correctly at different item sizes.


### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `bitmap`, `sdf`, `msdf` modes |

Must match the declared render mode format.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `render_mode` | `code`, `bitmap`, `sdf`, `msdf` | `code` |
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

All set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.


## Fallback

If `render_mode = msdf` or `sdf` is unsupported, the scene may fall back to `code` or `bitmap`
and emits a diagnostic. `color_mode = scalar` and `size_mode = scalar` follow standard fallbacks.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| No shape or edge needed | `point` |
| Fixed square pixel mark | `pixel` |
| Connected lines | `segment` or `path` |
| Text or atlas-backed custom symbols | `glyph` |


## Minimum Cases This Spec Must Support

1. uniform disc markers,
2. per-point colored discs,
3. categorical scatter with 5 shapes — 5 marker visuals,
4. directional arrows — `angle` `PER_ITEM`,
5. outlined markers — `aspect = outline`,
6. custom MSDF symbol — `render_mode = msdf` with texture,
7. bubble chart with shaped markers — `size_mode = scalar` with sqrt scale,
8. quiver plot — `shape = arrow`, `angle` `PER_ITEM`, `magnitude` `PER_ITEM`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_marker_mode` | `render_mode` axis |
| `dvz_marker_aspect` | `aspect` parameter |
| `dvz_marker_shape` | `shape` parameter |
| `dvz_marker_position/size/color/angle` | same attributes, extended sources and modes |
| `dvz_marker_edgecolor/linewidth` | unchanged |
| `dvz_marker_texture/tex_scale` | unchanged |

v0.4 adds: `size_space`, `shift`, `color_mode = scalar`, `size_mode = scalar`.
`mtsdf` merged into `msdf` unless implementation evidence separates them.


## Deferred Questions

1. whether per-item `shape` should be supported via a shape index attribute in a future version,
2. whether per-item `edgecolor` is worth the memory cost,
3. the exact public API spelling for the three variant axes,
4. whether `ellipse` needs a per-item aspect-ratio attribute beyond `angle`.
