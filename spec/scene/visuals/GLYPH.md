# Visual Family: `glyph`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`glyph` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`glyph` renders atlas-backed text geometry. In the target semantic model, each logical glyph item
belongs to a higher-level text string anchored in screen, data, or world space.

Each logical item is one string in the target model. The scene layer handles font layout, atlas
management, and per-character geometry internally. Ordinary users should not interact with
glyph-level atlas coordinates.

Typical uses: axis tick labels, data point annotations, legend entries, categorical labels,
per-character color coding.

Higher-level retained text objects are defined in [../semantics/TEXT.md](../semantics/TEXT.md).
Those objects may lower to this `glyph` visual contract, but the public text API should remain
semantic and should not expose glyph atlas internals.


## Current Implementation Status

The current v0.4 implementation has visible glyph rendering, but its public text entry point is
still being migrated:

1. `dvz_glyph()` is installed as a low-level `DvzVisual*` constructor,
2. `dvz_text()` is installed as a semantic `DvzText*` API that lowers internally to glyph visuals,
3. text uses bitmap/SDF/MSDF-capable atlas resources and renders through the scene -> FramePlan ->
   DRP2 -> vklite/canvas path,
4. focused tests cover text realization, UTF-8 atlas growth, missing-glyph fallback, many-label
   batching, runtime readback, and app/offscreen visible pixels.

The installed low-level glyph visual descriptor is quad/atlas oriented, with `position`, `bounds`,
`texcoords`, `color`, and `angle` attributes plus a bound 2D atlas field. It does not expose
`string`, `font_size`, or `char_color` as installed `dvz_visual_set_data()` attributes; those belong
to semantic text ownership and text lowering.

The v0.4 target is semantic text ownership. `DvzText*` objects own content, style, placement, and
identity; glyph visuals are derived
implementation output or an explicit low-level escape hatch for advanced callers.


## Item and Group Model

The target semantic glyph model is span-structured: each span is one string. Characters within each
string are sub-items managed internally by the scene layer.

The user provides data at the string level. The scene layer expands each string into per-character
geometry using the active font.

`PER_ITEM` attribute sources are indexed by string.
`PER_SPAN` is equivalent to `PER_ITEM` for `glyph` — string = span = item at the user-facing
level. `PER_GROUP` applies when strings belong to semantic groups (e.g., neuron populations)
declared via a per-span `"group_id"` attribute.


## Per-String Attributes

### `string`

| Property | Value |
|---|---|
| Type | UTF-8 text |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Text content of the string. When updated, the scene re-runs font layout for affected strings.


### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Anchor point of the string in visual space. Which part of the string aligns to this point is
controlled by `anchor`.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Color of the string text.
Accepted sources: `CONSTANT`, `PER_ITEM`.

`PER_ITEM` here means one color per string (all characters in a string share the same color).
For per-character coloring, use `char_color`.


### `char_color`

| Property | Value |
|---|---|
| Type | same as `color` |
| Accepted sources | `PER_CHAR` — one value per character across all strings |
| Typical mutability | `dynamic` |
| Optional | yes — when not set, `color` applies uniformly |

Per-character color. Indexed over the flat array of all characters in render order (strings
concatenated in declaration order). When set, overrides `color` for each character.

Must use the same `color_mode` as `color`.


### `font_size`

| Property | Value |
|---|---|
| Type | `float32`, unit determined by `font_size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Size of the rendered text. `CONSTANT` means all strings share the same size.


### `angle`

Standard — see `SHARED_ATTRIBUTES.md`. Rotation applied to the whole string around its anchor.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applied in screen space after the panel transform — text always faces the viewer.


### `anchor`

| Property | Value |
|---|---|
| Type | `vec2` — `(ax, ay)`, each in `[-1, 1]` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | inherits visual-wide `anchor` value |
| Typical mutability | `dynamic` |
| Optional | yes |

Per-string anchor override. When set, overrides the visual-wide `anchor` for that string.
Useful for mixed-alignment label sets (e.g., left-aligned axis labels + centered title).


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`. Screen-space pixel offset of the string anchor.
Accepted sources: `CONSTANT`, `PER_ITEM`.


## Visual-Wide Parameters

### `anchor`

| Property | Value |
|---|---|
| Type | `vec2` — `(ax, ay)`, each in `[-1, 1]` |
| Default | `(0, 0)` — centered |
| Mutability | `dynamic` |

Normalized alignment point within the string bounding box.

| `(ax, ay)` | Meaning |
|---|---|
| `(-1, -1)` | top-left |
| `(0, 0)` | center |
| `(1, 1)` | bottom-right |
| `(-1, 0)` | left-centered vertically |
| `(0, -1)` | top-centered horizontally |

`ax = -1`: left edge of bounding box aligns to `position`.
`ax = 1`: right edge aligns to `position`.
`ay = -1`: top edge aligns to `position`.
`ay = 1`: bottom edge aligns to `position`.

This is the visual-wide default. Individual strings may override it via the per-string
`anchor` attribute (see Per-String Attributes).


### `line_height`

| Property | Value |
|---|---|
| Type | `float32`, multiplier of `font_size` |
| Default | `1.2` |
| Mutability | `dynamic` |

Line spacing multiplier for multi-line strings (strings containing `\n`).
`1.0` means lines are packed at exactly `font_size` height; `1.2` adds 20% spacing.
Applies uniformly to all strings in the visual.


### `bgcolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | transparent `(0, 0, 0, 0)` |
| Mutability | `dynamic` |

Background fill color drawn behind the string bounding box.
Useful for readability against varying backgrounds.
`PER_ITEM` allows individual strings to have different background colors.
`CONSTANT` applies the same background to all strings (visual-wide default: transparent).


### `up_axis`

| Property | Value |
|---|---|
| Type | `vec3` — unit vector in visual space |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | `(0, 1, 0)` — screen-up (Y-up) |
| Mutability | `dynamic` |

Text orientation in 3D space. Defines which direction is "up" for the string layout.
Combined with `position` and the panel transform to orient text anchored to 3D surfaces,
volume slice labels, or axis labels in a 3D scene.
In 2D panels the default `(0, 1, 0)` matches screen-space Y-up and `angle` handles rotation.
`angle` is applied on top of `up_axis` as an additional screen-space rotation.


### `font`

| Property | Value |
|---|---|
| Type | font resource reference |
| Default | scene-provided default font |
| Mutability | `static` — set at visual creation time |

Typeface used for layout and rendering. The scene provides at least one built-in font.
Custom fonts are loaded as scene resources (see `../pipeline/RESOURCE_MODEL.md`).
All strings in a visual share the same font.


### `font_size_space`

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

Same semantics as `size_space`. Use `data` when text size should scale with zoom
(e.g., labels embedded in a physical diagram).


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `string` / glyph text | required | empty string renders nothing and is not pickable | no |
| `position` | required | NaN/Inf string skipped and not pickable | no |
| `color`, `char_color`, `background` | defaults described above | scalar NaN uses scale missing color | yes |
| `font`, `size`, `anchor`, `line_height` | defaults described above | invalid font/size is validation error or fallback by policy | yes |
| `angle`, `axis` | defaults described above | NaN angle falls back to zero | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time. Applies to both `color` and `char_color`.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
`angle` and `shift` are applied in screen space after the panel transform.
The target semantic text path returns the string index as item identity. Sub-character picking is
not supported.

Status on 2026-05-27: low-level `glyph` and internal `text` query families are registered by name,
but they do not yet provide active build/decode operations. Glyph/text picking remains a target
capability.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Ordinary labels, axes, colorbars, annotations, readouts | semantic `DvzText` or annotation APIs |
| Low-level generated glyph batches | `glyph` with `color` `CONSTANT` |
| Per-character colored text | semantic text when available; otherwise `glyph` with `char_color` |
| Custom atlas symbols (icons) | future extension or `marker` with `msdf` |


## Minimum Cases This Spec Must Support

1. axis tick labels — `PER_ITEM` position and string, `CONSTANT` color and font_size,
2. per-point annotations — `PER_ITEM` position, color, and string,
3. per-character colored sequence logo — `char_color` `PER_CHAR`,
4. large zoomed-in labels — `font_size_space = data`,
5. rotated labels — `angle` `PER_ITEM`,
6. centered vs. left-aligned labels — `anchor` variants.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_glyph_position` (per-char) | `position` per-string; scene handles char layout |
| `dvz_glyph_color` (per-char) | `char_color` `PER_CHAR`; `color` for per-string |
| `dvz_glyph_size` (vec2 per-char) | `font_size` per-string; scene computes char sizes |
| `dvz_glyph_texcoords` (per-char) | hidden — scene layer concern, not user-facing |
| `dvz_glyph_shift` (per-char) | `shift` per-string; char-level shifts are internal layout |
| `dvz_glyph_anchor` (per-char) | `anchor` visual-wide |
| `dvz_glyph_group_size` (per-group) | implicit in font layout — not user-facing |
| `dvz_glyph_axis` (vec3 per-char) | replaced by text orientation modes and `angle`; no public per-character axis |
| `dvz_glyph_scale` (per-char) | subsumed by `font_size` per-string |
| `dvz_glyph_bgcolor` | `bgcolor` visual-wide |
| `dvz_glyph_texture` | hidden — scene layer concern |
| `dvz_glyph_atlas_font` | `font` resource reference |
| `dvz_glyph_unicode` / `dvz_glyph_ascii` | `string` `PER_ITEM` |
| `dvz_glyph_strings` | primary user-facing API pattern |
| `dvz_monoglyph` | dropped |

v0.4 hides all per-character GPU details behind the scene layer. The user works at string
granularity only.


`char_color` `PER_SPAN` is redundant with `color` `PER_ITEM` (both give one color per string)
— not supported; use `color` `PER_ITEM` for per-string uniform color.

`letter_spacing` is not supported in v0.4 — standard font kerning is applied by the atlas layout.

All other deferred questions resolved — see Per-String Attributes and Visual-Wide Parameters.
