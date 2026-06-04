# Visual Family: `marker`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`marker` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.
Reusable symbol-set semantics are in [../semantics/SYMBOLS.md](../semantics/SYMBOLS.md).
Landed naming and marker implementation decisions are tracked in `IMPLEMENTATION_DECISIONS.md`.


## Current Implementation Status

Status on 2026-06-04: the active v0.4 target is full v0.3 built-in code-SDF marker
parity, followed by the `DvzSymbolSet` resource layer for texture-backed and imported symbols.

The implemented path supports:

1. retained `marker` visual construction via `dvz_marker()`;
2. dense `position`, `color`, public `diameter`, `angle`, and `shape` attributes, where
   `diameter` aliases the current internal `size` slot;
3. `shape` values stored as `uint32_t` `DvzMarkerShape` values;
4. built-in code-SDF shapes for the v0.3 marker vocabulary plus `target`;
5. `dvz_marker_style()` and `dvz_marker_set_style()` with `edge_color`, `stroke_width`, and
   exclusive `filled`/`stroke`/`outline` aspect semantics;
6. GLSL/Vulkan native point-list lowering with marker SDF coverage;
7. WGSL/WebGPU instanced-quad lowering through the point-like lowering policy;
8. GPU-backed marker picking using the marker sprite bounding box.

The following sections describe the target marker contract. Reusable symbol sets, bitmap/SDF/MSDF
symbol sources, SVG-path import, and exact SDF-mask picking are v0.4 parity work. Scalar
color/diameter modes, `shift`, aspect-ratio/magnitude helpers, and data-space sizing remain planned
capabilities unless explicitly marked as implemented above.


## Semantic Purpose

`marker` renders shaped point-like marks with full visual styling: shape, rotation, fill, stroke,
and edge treatment.

Richer than `point` (adds shape, angle, edge color, edge width).
Right for categorical scatter plots needing symbolic differentiation, directional indicators,
flow-field glyphs, and styled annotations.


## Symbol And Encoding Model

Long-term marker APIs should select symbols, not rendering modes. A marker item names a symbol from a
bound symbol set; the runtime chooses or is configured with an encoding for that symbol.

Target marker data:

| Attribute | Meaning |
|---|---|
| `position` | Anchor point in visual space. |
| `diameter` | Screen-space symbol extent by default. |
| `angle` | Screen-space rotation around the anchor. |
| `color` | Fill/tint color. |
| `symbol` | `uint32_t` `DvzSymbolId` selecting an entry in a bound `DvzSymbolSet`. |

The current first slice uses `shape` instead of `symbol`; this should become either an alias for
built-in symbol ids or a compatibility spelling for code-SDF built-ins.

Encodings remain implementation capabilities:

| Encoding | Description | Public source that may use it |
|---|---|---|
| `code` | Built-in shader SDF. | Built-in symbols. |
| `bitmap` | RGBA or alpha raster texture. | Bitmap symbols. |
| `sdf` | Single-channel distance texture. | Imported/generated SDF symbols. |
| `msdf` | Multi-channel distance texture. | SVG path and imported MSDF symbols. |

`code`, `bitmap`, `sdf`, and `msdf` should not be the primary marker API. They are symbol backing
encodings, diagnostics, and optional advanced preferences. See
[../semantics/SYMBOLS.md](../semantics/SYMBOLS.md).


## Built-In Shapes (`code` mode)

The shape vocabulary below preserves the useful enum sketch from the retired broad scene API draft.
Final installed enum names may differ, but the semantic set should remain stable unless the marker
family spec is revised.

| Shape | | Shape | | Shape | |
|---|---|---|---|---|---|
| `disc` | filled circle | `square` | axis-aligned square | `diamond` | rotated square |
| `circle` | ring | `triangle` | equilateral triangle | `cross` | plus sign |
| `asterisk` | six-pointed | `chevron` | V-shape | `clover` | four-lobed |
| `club` | club suit | `spade` | spade suit | `heart` | heart suit |
| `arrow` | directional | `ellipse` | ellipse | `hbar` | horizontal bar |
| `vbar` | vertical bar | `ring` | thick ring | `pin` | map pin |
| `tag` | label shape | `rounded_rect` | rounded rectangle | `target` | ring plus crosshair |

Shape is visual-wide by default in the target `code` mode.
Per-item shape is supported via the `shape` attribute (see Per-Item Attributes).

Status on 2026-06-04: v0.4 parity requires this full built-in vocabulary. The installed path uses a
dense per-item `shape` attribute instead of a visual-wide shape parameter; `shape` becomes the
compatibility spelling for built-in symbol ids when `DvzSymbolSet` lands.


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


### `diameter`

Standard — see `SHARED_ATTRIBUTES.md`.
Storage name: `size`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.


### `angle`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `PER_ITEM` in the active descriptor. `CONSTANT` and `PER_GROUP` are target
capabilities.
Applied in screen space after the panel transform — markers always face the viewer.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.


### `shape`

| Property | Value |
|---|---|
| Type | `uint32` — `DvzMarkerShape` |
| Accepted sources | `PER_ITEM` in the active first slice |
| Typical mutability | `dynamic` |
| Optional | no in the active first slice |
| Applies to | `code` mode only |

Per-item shape selector. Each item renders with the requested built-in code-SDF shape.

The built-in symbol-set slice complements this with a `symbol` attribute selecting `DvzSymbolId`
values from a bound `DvzSymbolSet`. In the current code-SDF path, `shape` and `symbol` share the
same retained storage slot for built-in ids.


### `symbol`

| Property | Value |
|---|---|
| Type | `uint32` — `DvzSymbolId` scoped to the bound `DvzSymbolSet` |
| Accepted sources | `PER_ITEM`, target `CONSTANT` and `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | target yes; defaults to the marker visual's default symbol |
| Applies to | built-in, bitmap, SDF, and MSDF symbol sources |

Per-item symbol selector. This is the long-term generalized form of `shape`.

Status on 2026-06-04: built-in symbol-set binding is active for code-SDF markers. `symbol` lowers
through the same retained slot as `shape`. Homogeneous bitmap symbol arrays render through generated
`tex_rect` data and a per-visual RGBA atlas texture. SDF/MSDF symbol source APIs copy payloads into
per-encoding `DvzSymbolSet` atlas pages, but marker validation rejects those ids until the
distance-field marker shader variants land. Mixed built-in/texture-backed arrays are also rejected
for now.


### `edge_color` (per-item)

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Accepted sources | `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes — when absent, the visual-wide `edge_color` applies |
| Applies to | `aspect = outline` or `aspect = stroke` |

Per-item edge color override. Only allocated when explicitly set — no memory cost when not used.
Overrides the visual-wide `edge_color` per item.

Status on 2026-05-17: per-item `edge_color` is not implemented. The active style API uses a
visual-wide `edge_color` field in `DvzMarkerStyle`.


### `aspect_ratio`

| Property | Value |
|---|---|
| Type | `float32` — height / width ratio |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Default | `1.0` (circle) |
| Typical mutability | `dynamic` |
| Optional | yes — ignored when `shape ≠ ellipse` |
| Applies to | `shape = ellipse` (visual-wide or via per-item `shape`) |

Per-item ellipse aspect ratio. Values > 1 produce tall ellipses; values < 1 produce wide ones.
Combined with `angle` `PER_ITEM` for oriented ellipses.


### `magnitude`

| Property | Value |
|---|---|
| Type | `float32` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |
| Optional | yes — defaults to `1.0` (no scaling) |

Multiplicative scale applied to `diameter` per item, after all other size transformations.
Primary use: quiver plots, where `diameter` sets the base arrow length and `magnitude` encodes
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

Status on 2026-05-17: not implemented as a visual-wide parameter. Use the dense per-item `shape`
attribute in the active first slice.


### `aspect`

| Property | Value |
|---|---|
| Type | enum: `filled`, `stroke`, `outline` |
| Default | `filled` |
| Mutability | `dynamic` |

- `filled`: solid fill, no visible edge.
- `stroke`: edge only, no fill. Width controlled by `stroke_width`.
- `outline`: filled body with edge on top. Uses both `color` (fill) and `edge_color` (edge).


### `edge_color`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Edge color for `aspect = stroke` or `aspect = outline`. Visual-wide.


### `stroke_width`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `0.0` |
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

Implementation status on 2026-05-16: `size_space = data` remains required for markers whose
shape represents physical extent. The marker still faces the camera and keeps its SDF/bitmap/MSDF
shape evaluation in screen space, but the final mark extent is derived by projecting a data-space
size through the active panel transform. Backends that only support fixed screen-space markers must
diagnose the unsupported size-space mode.


### `tex_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |
| Applies to | `msdf`, `sdf`, `bitmap` symbol encodings |

Reference size at which the texture was generated (typically the texture width in pixels).
Tells the shader how to scale SDF distances correctly at different item sizes.


### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `bitmap`, `sdf`, `msdf` symbol encodings |

Must match the declared render mode format.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf item skipped and not pickable | no |
| `color`, `edge_color` | fill white, edge transparent | scalar NaN uses scale missing color | yes |
| `diameter`, `magnitude` | family-defined screen size, scale `1` | scalar NaN uses fallback size | yes |
| `angle`, `aspect`, `shape`/`symbol` | defaults described above | invalid value is validation error | yes |
| `shift` | `(0, 0)` | NaN component treated as zero shift | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `symbol_source` | built-in, bitmap, sdf, msdf, svg path | built-in |
| `encoding_preference` | auto, code, bitmap, sdf, msdf | auto |
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

The installed v0.4 marker constructor keeps source selection on `DvzSymbolSet`; marker visual flags
should remain focused on marker attribute modes such as scalar color and scalar size. Built-in and
homogeneous bitmap symbols render today. SDF/MSDF atlas pages wait for distance-field marker shader
variants and scale-correct decode metadata.


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
| Text | `glyph` |
| Centered atlas-backed icons/symbols | `marker` with `DvzSymbolSet` |


## Minimum Cases This Spec Must Support

1. uniform disc markers,
2. per-point colored discs,
3. categorical scatter with 5 shapes — 5 marker visuals,
4. directional arrows — `angle` `PER_ITEM`,
5. outlined markers — `aspect = outline`,
6. custom MSDF symbol — SVG path or MSDF source in `DvzSymbolSet`,
7. bubble chart with shaped markers — `size_mode = scalar` with sqrt scale,
8. quiver plot — `shape = arrow`, `angle` `PER_ITEM`, `magnitude` `PER_ITEM`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_marker_mode` | symbol encoding preference or compatibility backend knob |
| `dvz_marker_aspect` | `aspect` parameter |
| `dvz_marker_shape` | built-in `symbol` id or compatibility `shape` parameter |
| `dvz_marker_position/size/color/angle` | `position`/`diameter`/`color`/`angle`, extended sources and modes |
| `dvz_marker_edgecolor/linewidth` | `edge_color` and `stroke_width` style fields |
| `dvz_marker_texture/tex_scale` | symbol source texture plus distance-field metadata |

v0.4 adds: `size_space`, `shift`, `color_mode = scalar`, `size_mode = scalar`.
`mtsdf` merged into `msdf` unless implementation evidence separates them.

v0.3 marker prior art to preserve:

1. render modes were `code`, `bitmap`, `sdf`, `msdf`, and `mtsdf`;
2. code shapes included `disc`, `asterisk`, `chevron`, `clover`, `club`, `cross`, `diamond`,
   `arrow`, `ellipse`, `hbar`, `heart`, `infinity`, `pin`, `ring`, `spade`, `square`, `tag`,
   `triangle`, `vbar`, and `rounded_rect`;
3. bitmap markers sampled an RGBA texture while combining the per-item marker alpha;
4. SDF markers sampled a single-channel distance texture and used `tex_scale` for distance scaling;
5. MSDF markers accepted SVG path strings through `dvz_msdf_from_svg()` before upload to the marker
   texture;
6. marker SDF/MSDF custom-symbol import should return in v0.4 through reusable symbol sets, not as
   a separate visual family or as an ad hoc example-only bitmap path.


## Follow-Up Pressure

1. Exact marker picking should test the active SDF or bitmap-alpha mask, not the sprite rectangle.
   Transparent corners and holes in ring/cross shapes must miss.
2. Bitmap marker mode still needs item-state styling composition and exact alpha-aware picking.
3. Shared SDF/MSDF decode helpers with text should wait until atlas entries carry distance-field
   metadata needed for scale-correct antialiasing.
4. Restore SVG path import for custom SDF/MSDF marker symbols, equivalent in capability to the v0.3
   `dvz_msdf_from_svg()` marker test path.
5. Add SDF/MSDF marker shader variants so marker can use all installed symbol sources without
   exposing `code`/`bitmap`/`sdf`/`msdf` as the primary marker API.
