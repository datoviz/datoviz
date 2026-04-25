# Visual Family: `marker`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`marker` visual family.

It refines:

1. `VISUAL_FAMILIES.md` — family taxonomy and rationale
2. `VISUAL_MINI_CONTRACTS.md` — family-level mini-contract
3. `ATTRIBUTE_SOURCES.md` — attribute granularity and mutability vocabulary
4. `VISUAL_CONTRACT.md` — shared visual responsibilities


## Semantic Purpose

`marker` renders shaped point-like marks with full visual styling: shape, rotation, fill, stroke,
and edge treatment.

It is richer than `point` (adds shape, angle, edge color, edge width) and is the natural family
for scientific scatter plots that need symbolic differentiation between categories, directional
indicators, or styled annotations.

Typical uses: categorical scatter plots with shape-encoded groups, wind or flow field arrows,
directional neuron markers, map-pin-style overlays.


## Render Mode

Render mode is the primary variant axis for `marker`.
It determines how the mark shape is produced and which resources are required.

| Mode | Description | Texture required |
|---|---|---|
| `code` | shape encoded in the shader as a signed-distance function | no |
| `bitmap` | shape from an RGBA raster texture | yes (RGBA) |
| `sdf` | shape from a single-channel SDF texture | yes (single-channel float) |
| `msdf` | shape from a multi-channel SDF texture | yes (RGBA float) |

`code` is the default and requires no texture.
It supports the built-in shape set listed below and renders at any size without quality loss.

`msdf` is preferred over `sdf` and `bitmap` for custom shapes: it is resolution-independent and
supports sharp edges at large sizes.

`bitmap` is the simplest texture mode but pixelates at large sizes.

Render mode is set at visual creation time and cannot change without recreating the visual.


## Built-In Shapes (`code` mode)

When `render_mode = code`, shape is selected from the built-in set:

| Shape | Description |
|---|---|
| `disc` | filled circle (default) |
| `circle` | ring (unfilled circle, use with `aspect = stroke`) |
| `square` | axis-aligned square |
| `diamond` | rotated square |
| `triangle` | equilateral triangle |
| `cross` | plus sign |
| `asterisk` | six-pointed star/asterisk |
| `chevron` | V-shape / chevron |
| `clover` | four-lobed clover |
| `club` | club suit |
| `spade` | spade suit |
| `heart` | heart suit |
| `arrow` | directional arrow |
| `ellipse` | ellipse (uses `angle` for orientation) |
| `hbar` | horizontal bar |
| `vbar` | vertical bar |
| `ring` | thick ring |
| `pin` | map pin / teardrop |
| `tag` | tag / label shape |
| `rounded_rect` | rectangle with rounded corners |

Shape is a visual-wide parameter in `code` mode.
All items in the visual share the same shape.
For per-item shape variation, use multiple marker visuals or the `glyph` family (for atlas-backed
custom shapes).


## Per-Item Attribute Schema

### `position`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color mode |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

Two color modes: `rgba` (default) and `scalar` (mapped through a `Scale` — see `SCALES.md`).
Applies to the fill color of the marker body.


### `size`

| Property | Value |
|---|---|
| Type | `float32` (direct) or `scalar_f32` (mapped) — see size mode |
| Unit | determined by `size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

Two size modes: `direct` (default, float in pixels or data units) and `scalar` (mapped through a
size `Scale`).


### `angle`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | radians, clockwise from up |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |

Rotation of the marker around its center.
`CONSTANT` source with `angle = 0` is the default (no rotation).
Per-item angle is used for directional markers such as arrows or chevrons.
Per-group angle is useful when groups represent orientations (e.g., cardinal directions).


## Visual-Wide Parameters

### `shape`

| Property | Value |
|---|---|
| Type | enum (see built-in shape list) |
| Default | `disc` |
| Mutability | `dynamic` |
| Applies to | `code` mode only |

The marker shape for all items.
Ignored in `bitmap`, `sdf`, and `msdf` modes (shape comes from the texture instead).


### `aspect`

| Property | Value |
|---|---|
| Type | enum: `filled`, `stroke`, `outline` |
| Default | `filled` |
| Mutability | `dynamic` |

Rendering style of the marker body:

- `filled`: solid fill, no visible edge.
- `stroke`: edge only, no fill (hollow marker). The edge width is controlled by `linewidth`.
- `outline`: filled body with a visible edge drawn on top. Requires both `color` (fill) and
  `edgecolor` (edge).


### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | white `(255, 255, 255, 255)` |
| Mutability | `dynamic` |

Color of the marker edge, used when `aspect = stroke` or `aspect = outline`.
Visual-wide — all items share the same edge color.


### `linewidth`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | screen pixels |
| Default | `1.0` |
| Mutability | `dynamic` |

Width of the marker edge in screen pixels, used when `aspect = stroke` or `aspect = outline`.


### `size_space`

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

Same semantics as in `point` and `pixel`. See `visuals/POINT.md` for full discussion.


### `tex_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |
| Applies to | `msdf`, `sdf`, `bitmap` modes only |

Scale factor between the texture coordinate space and the marker size.
Tells the shader the reference size at which the texture was generated, so that SDF distances
scale correctly at different item sizes.
Typically set to the texture width in pixels (e.g., `100.0` for a 100×100 MSDF texture).
Ignored in `code` mode.


### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` (scene resource) |
| Mutability | `dynamic` |
| Applies to | `bitmap`, `sdf`, `msdf` modes only |

The texture used as the marker shape source.
Must match the declared render mode:
- `bitmap`: RGBA u8 texture
- `sdf`: single-channel float texture
- `msdf`: RGBA float multi-channel SDF texture


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `render_mode` | `code`, `bitmap`, `sdf`, `msdf` | `code` |
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |

All three axes are set at visual creation time.
`aspect`, `shape`, and `size_space` are visual-wide parameters that can change at runtime.


## Transform Model

Standard two-stage transform:

1. **Normalization** — data-space positions normalized to visual space before upload.
2. **Panel transform** — panel-local camera or panzoom applied per-frame without re-upload.

`marker` does not support a visual-local transform matrix.
`angle` is applied in screen space after the panel transform, so markers always face the viewer
regardless of 3D camera orientation.


## Stage Participation

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional, natural |
| Offscreen / export | same as render |


## Picking Model

When picking is enabled:

1. a pick result returns `(panel_id, visual_id, item_index)`,
2. no sub-item identity is defined.

Picking uses item index, not visual shape or edge sub-region.


## Fallback Notes

If the runtime cannot support `render_mode = msdf` or `render_mode = sdf`, the scene may fall
back to `render_mode = bitmap` or `render_mode = code` depending on shape availability, and emits
a capability adaptation diagnostic.

`color_mode = scalar` and `size_mode = scalar` fallbacks follow the same CPU-side mapping path as
`point`.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| No shape or edge needed | `point` |
| Fixed square pixel mark | `pixel` |
| Connected lines or paths | `segment` or `path` |
| Text or atlas-backed custom symbols | `glyph` |
| Raw topology | `primitive` |


## Minimum Cases This Spec Must Support

1. uniform disc markers — `shape = disc`, `color` and `size` `CONSTANT`,
2. per-point colored discs — `color` `PER_ITEM` rgba,
3. categorical scatter — 5 groups, per-group shape (5 marker visuals) and per-group color,
4. directional arrows — `shape = arrow`, `angle` `PER_ITEM`,
5. outlined markers — `aspect = outline`, `edgecolor` set,
6. custom MSDF symbol — `render_mode = msdf`, texture supplied,
7. bubble chart with shaped markers — `size_mode = scalar` with sqrt scale, `shape = disc`.


## v0.3 Correspondence

```c
dvz_marker(batch, flags)
dvz_marker_mode(visual, mode)      // render_mode
dvz_marker_aspect(visual, aspect)
dvz_marker_shape(visual, shape)
dvz_marker_position(visual, ...)
dvz_marker_size(visual, ...)
dvz_marker_color(visual, ...)
dvz_marker_angle(visual, ...)
dvz_marker_edgecolor(visual, color)
dvz_marker_linewidth(visual, width)
dvz_marker_texture(visual, texture)
dvz_marker_tex_scale(visual, scale)
```

| v0.3 | v0.4 |
|---|---|
| `dvz_marker_mode` | `render_mode` variant axis |
| `dvz_marker_aspect` | `aspect` parameter |
| `dvz_marker_shape` | `shape` parameter |
| `dvz_marker_position` | `position` attribute, `PER_ITEM` |
| `dvz_marker_size` | `size` attribute, now also `CONSTANT`/`PER_GROUP` and `scalar` mode |
| `dvz_marker_color` | `color` attribute, now also `CONSTANT`/`PER_GROUP` and `scalar` mode |
| `dvz_marker_angle` | `angle` attribute, now also `CONSTANT`/`PER_GROUP` |
| `dvz_marker_edgecolor` | `edgecolor` parameter, unchanged |
| `dvz_marker_linewidth` | `linewidth` parameter, unchanged |
| `dvz_marker_texture` | `texture` parameter (scene resource handle) |
| `dvz_marker_tex_scale` | `tex_scale` parameter, unchanged |

v0.4 adds `size_space`, `color_mode = scalar`, and `size_mode = scalar`.
The `mtsdf` mode from v0.3 is merged into `msdf` unless implementation evidence later justifies
keeping them separate.


## Deferred Questions

1. whether per-item `shape` should be supported in a future version via a shape index attribute
   and a shader lookup table,
2. whether per-item `edgecolor` is worth the memory cost for a future variant,
3. whether `angle` should be in clockwise or counter-clockwise radians — convention should be
   fixed before the first implementation,
4. the exact public API spelling for the three variant axes at creation time,
5. whether `ellipse` shape needs additional per-item aspect-ratio control beyond `angle`.
