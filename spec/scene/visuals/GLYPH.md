# Visual Family: `glyph`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`glyph` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`glyph` renders text strings anchored at data-space positions.

Each logical item is one string. The scene layer handles font layout, atlas management, and
per-character geometry internally. Users do not interact with glyph-level atlas coordinates.

Typical uses: axis tick labels, data point annotations, legend entries, categorical labels,
per-character color coding.


## Item and Group Model

`glyph` is a **grouped visual**: each logical item is one string (group). Characters within each
string are sub-items managed internally by the scene layer.

The user provides data at the string level. The scene layer expands each string into per-character
geometry using the active font.

`PER_ITEM` attribute sources are indexed by string.
`PER_GROUP` is not a distinct concept here — string = group = item at the user-facing level.


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

This is a visual-wide parameter. All strings share the same anchor convention.
Per-string anchor is a deferred question.


### `bgcolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | transparent `(0, 0, 0, 0)` |
| Mutability | `dynamic` |

Background fill color drawn behind each character's bounding box.
Useful for readability against varying backgrounds.
Visual-wide — all characters share the same background color.


### `font`

| Property | Value |
|---|---|
| Type | font resource reference |
| Default | scene-provided default font |
| Mutability | `static` — set at visual creation time |

Typeface used for layout and rendering. The scene provides at least one built-in font.
Custom fonts are loaded as scene resources (see `RESOURCE_MODEL.md`).
All strings in a visual share the same font.


### `font_size_space`

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

Same semantics as `size_space`. Use `data` when text size should scale with zoom
(e.g., labels embedded in a physical diagram).


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time. Applies to both `color` and `char_color`.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
`angle` and `shift` are applied in screen space after the panel transform.
Picking returns the string index as item identity. Sub-character picking is not supported.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Single uniform labels | `glyph` with `color` `CONSTANT` |
| Per-character colored text | `glyph` with `char_color` |
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
| `dvz_glyph_axis` (vec3 per-char) | replaced by `angle` per-string; 3D text deferred |
| `dvz_glyph_scale` (per-char) | subsumed by `font_size` per-string |
| `dvz_glyph_bgcolor` | `bgcolor` visual-wide |
| `dvz_glyph_texture` | hidden — scene layer concern |
| `dvz_glyph_atlas_font` | `font` resource reference |
| `dvz_glyph_unicode` / `dvz_glyph_ascii` | `string` `PER_ITEM` |
| `dvz_glyph_strings` | primary user-facing API pattern |
| `dvz_monoglyph` | dropped |

v0.4 hides all per-character GPU details behind the scene layer. The user works at string
granularity only.


## Deferred Questions

1. whether per-string `anchor` should be supported (vs. visual-wide only),
2. whether multi-line strings (newline handling, line spacing) are in scope,
3. whether per-string `bgcolor` is worth supporting,
4. whether `char_color` should support `PER_GROUP` (one color per string, same as `color`
   `PER_ITEM`) or whether `color` `PER_ITEM` already covers that,
5. whether a `letter_spacing` or `line_height` parameter is needed,
6. whether 3D text orientation (`axis` as vec3) is needed for volume or 3D scene use cases.
