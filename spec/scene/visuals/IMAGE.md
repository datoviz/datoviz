# Visual Family: `image`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`image` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Implementation Status

Status on 2026-05-17: this file is the target contract for the v0.4 `image` visual family, not a
description of everything currently implemented.

The active native implementation is a first slice with two geometry paths:

- legacy single-quad geometry: `position` is provided as four `vec3` corner vertices in
  triangle-strip order and `texcoords` as four matching `vec2` UV vertices,
- per-item rectangle geometry: `position` is one item anchor, `extent` is one `vec2`
  display rectangle size, and optional `tex_rect` supplies one atlas UV rectangle per item,
- texture data comes from a 2D `SampledField` bound through `dvz_visual_set_field()`, or from the
  transitional `dvz_visual_set_texture()` / `dvz_visual_set_texture_f32()` helpers,
- RGBA8 image fields upload directly, while scalar fields are currently mapped through the bound
  `Scale`/`Colormap` into an RGBA staging texture before emission,
- image probing uses the same quad vertex/UV contract and must be updated alongside any geometry
  contract change.

The following parts of this target contract are intentionally deferred: `color`, `color_tint`,
`angle`, `shift`, `extent_space`, `transpose`, border/radius parameters, `texture_mode = none`,
native shader-side scalar colormap sampling, heatmap isolines, and label-contour rendering.

Before expanding this visual family, keep the image probe recovery slice covered for GPU-only
readback, non-fullscreen quads, panzoom/keep-aspect transforms, and the live napari label-hover
path. Segment IDs are owned by `dvz_labels()` probes rather than hidden image masks.


## Semantic Purpose

`image` renders textured rectangles anchored at data-space positions.

Each item is one image: a rectangular quad sampling a 2D texture, or a solid-filled rectangle.
Multiple items may sample different regions of the same texture (atlas use), or all items may
share the same full-texture mapping.

Typical uses: raster heatmaps, brain activity maps, image overlays, icon sprites, texture-mapped
annotations.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Anchor position of the image in visual space. Which part of the image aligns to this point is
controlled by `anchor`.


### `extent`

| Property | Value |
|---|---|
| Type | `vec2`, `(width, height)`, unit determined by `extent_space` |
| Accepted sources | `PER_ITEM` |
| Typical mutability | `dynamic` |

Display size of the image rectangle.


### `tex_rect`

| Property | Value |
|---|---|
| Type | `vec4`, `(u0, v0, u1, v1)` — UV rectangle, top-left to bottom-right |
| Accepted sources | `PER_ITEM` |
| Typical mutability | `dynamic` |
| Applies to | `texture_mode = rgba` or `scalar` only |

Portion of the texture to display. `(0, 0, 1, 1)` shows the full texture.
Use per-item values to implement texture atlases where each item samples a different region.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Fill color of the rectangle.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applies to `texture_mode = none` only. Ignored when a texture is active.


### `anchor`

| Property | Value |
|---|---|
| Type | `vec2` — `(ax, ay)`, each in `[-1, 1]` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | inherits visual-wide `anchor` value |
| Typical mutability | `dynamic` |
| Optional | yes |

Per-item anchor override. When set, overrides the visual-wide `anchor` for that item.
Allows mixed-alignment image sets (e.g., some images left-anchored, others centered).


### `color_tint`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | white `(255, 255, 255, 255)` — no tint |
| Typical mutability | `dynamic` |
| Applies to | `texture_mode = rgba` or `scalar` |

Multiplicative color tint applied on top of the texture sample.
White (default) has no effect. Use to colorize or fade individual images.
Ignored when `texture_mode = none` (use `color` instead).


### `angle`

Standard — see `SHARED_ATTRIBUTES.md`. Rotation around the anchor point.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applied in screen space after the panel transform.


### `shift`

Standard `vec2` — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`.


## Visual-Wide Parameters

### `anchor`

| Property | Value |
|---|---|
| Type | `vec2` — `(ax, ay)`, each in `[-1, 1]` |
| Default | `(0, 0)` — centered |
| Mutability | `dynamic` |

Same convention as `glyph`: `(-1, -1)` = top-left, `(1, 1)` = bottom-right.
All items share the same anchor convention.


### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `texture_mode = rgba` or `scalar` |

The 2D texture resource. Format must match the declared `texture_mode`.
All items in the visual sample from the same texture.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `semantics/SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar` only |

Maps single-channel float texture values to display colors.
Domain and palette can be updated without re-uploading texture data.


### `transpose`

| Property | Value |
|---|---|
| Type | `bool` |
| Default | `false` |
| Mutability | `dynamic` |

When `false` (default): texture U axis maps to screen X, V axis maps to screen Y.
When `true`: axes are swapped — U maps to Y, V maps to X.
Useful when scientific array data has shape `(height, width)` but should display as
`(width, height)` without re-allocating the array.


### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | transparent |
| Mutability | `dynamic` |

Border color drawn around each image rectangle. Set to transparent to disable.


### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `0.0` |
| Mutability | `dynamic` |

Border width. `0` means no border.


### `radius`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Default | `0.0` — sharp corners |
| Mutability | `dynamic` |

Rounded corner radius. `0` means rectangular.
`PER_ITEM` allows independently rounded images in the same visual.


### `extent_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.
Use `data` when the image should cover a fixed data-space region and scale with zoom
(e.g., a heatmap tile aligned to data axes).


## Isoline Parameters

Applies when `texture_mode = heatmap` and `isoline_count > 0`.
Isolines are computed on the GPU via a marching-squares `ComputeNode` in the `FramePlan` and
rendered as an overlay on top of the colormap-filled image.

### `isoline_count`

| Property | Value |
|---|---|
| Type | `uint32` |
| Default | `0` — isolines disabled |
| Mutability | `dynamic` |

Number of evenly-spaced isoline levels within `isoline_range`.

### `isoline_range`

| Property | Value |
|---|---|
| Type | `vec2` — `(min, max)` |
| Default | derived from the `colormap` Scale domain; otherwise auto-computed from data |
| Mutability | `dynamic` |

Value range over which isoline levels are distributed.

### `isoline_color`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Color of all isoline contours. Visual-wide.

### `isoline_linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.0` |
| Mutability | `dynamic` |

Width of drawn isolines.


## Contour And Isoline Recommendation

Status on 2026-05-16: the active native image visual is still a sampled-texture quad, so the
following is the target contract rather than a completed implementation.

Use two related but distinct paths:

1. **Label contours**: categorical boundary display for integer label images. The first
   implementation should keep the source label field raw, sample with nearest filtering, and use a
   fragment-shader neighbor check to show boundaries. This is the cheapest useful contour mode for
   napari-style labels and avoids generating geometry for every label edge.
2. **Scalar isolines**: continuous contour lines over a scalar field. If the line is only a visual
   overlay on the same image, a fragment-shader isoline test is acceptable for the first slice. If
   the line must be independently styled, picked, exported as vector geometry, or drawn with
   high-quality joins and antialiasing, generate a derived path/segment overlay with marching
   squares in a compute node or CPU fallback.

Do not bake contours into the uploaded image texture. Contour color, width, selected-label state,
and level selection belong in parameter resources so interactive updates do not force texture
reuploads.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf image instance skipped and not pickable | no |
| `extent`, `anchor`, `tex_rect` | defaults described above | invalid geometry is validation error | yes |
| `tint`, `alpha` | white tint, alpha `1` | NaN alpha falls back to default | yes |
| scalar texture samples | n/a | NaN maps through scale missing color | scale-owned |
| `texture` / sampled field | required for `rgba` and `scalar` modes | missing texture is validation error | no |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `rgba`, `scalar`, `heatmap`, `none` | `rgba` |
| `color_mode` | `rgba`, `scalar` | `rgba` |

Both set at visual creation time.

`texture_mode` controls how the texture is sampled:

| Mode | Texture format | Color source | Isolines |
|---|---|---|---|
| `rgba` | RGBA `u8` texture | texture sample | no |
| `scalar` | single-channel `f32` texture | texture sample mapped via `colormap` | no |
| `heatmap` | single-channel `f32` texture | texture sample mapped via `colormap` | optional, GPU marching squares |
| `none` | no texture | `color` per-item fill | no |

`heatmap` is a scalar colormap display with optional isoline overlay.
When `isoline_count > 0` the scene adds a marching-squares `ComputeNode` before the render node.

`color_mode` applies only when `texture_mode = none` and determines how `color` data is encoded.
Standard — see `SHARED_ATTRIBUTES.md`.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
`angle` and `shift` are applied in screen space after the panel transform.
Picking returns the image index as item identity.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Full-panel heatmap covering axes | `image` with `extent_space = data`, `texture_mode = heatmap` |
| Scalar field, no isolines needed | `image` with `texture_mode = scalar` |
| Sprite icons | `image` with `texture_mode = rgba`, per-item `tex_rect` |
| Colored rectangles without texture | `image` with `texture_mode = none` |
| 3D volume rendering | `volume` |


## Minimum Cases This Spec Must Support

1. single RGBA image overlay — `texture_mode = rgba`, one `position` and `extent`,
2. brain activity colormap — `texture_mode = scalar` with colormap Scale,
3. texture atlas sprite sheet — `texture_mode = rgba`, `tex_rect` `PER_ITEM`,
4. colored rectangle annotations — `texture_mode = none`, `color` `PER_ITEM`,
5. data-aligned heatmap tile — `extent_space = data`,
6. rotated image annotations — `angle` `PER_ITEM`,
7. transposed scientific array — `transpose = true`,
8. heatmap with isoline contours — `texture_mode = heatmap`, `isoline_count > 0`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_image_position` | `position` `PER_ITEM` |
| `dvz_image_size` | `extent` `PER_ITEM` |
| `dvz_image_anchor` | `anchor` `PER_ITEM` or omitted for centered |
| `dvz_image_texcoords` | `tex_rect` `PER_ITEM`; legacy corner UVs remain `texcoords` |
| `dvz_image_facecolor` | `color` `PER_ITEM` when `texture_mode = none` |
| `dvz_image_texture` | `texture` resource reference |
| `dvz_image_colormap` | `colormap` Scale reference |
| `dvz_image_permutation` | `transpose` (bool) |
| `dvz_image_edgecolor` | `edgecolor` |
| `dvz_image_linewidth` | `linewidth` |
| `dvz_image_radius` | `radius` |

v0.4 adds: `angle`, `shift`, `extent_space`, `texture_mode` and `color_mode` variant axes,
`colormap` as Scale reference instead of enum,
`transpose` replacing `permutation`.
v0.4 adds per-item `anchor`, `color_tint`, and per-item `radius`.

Per-item `texture` (each item from a different texture resource) is not supported — use
multiple visual instances instead.


## Follow-Up Pressure

1. Native shader-side scalar lookup should use raw scalar textures, contrast/gamma/opacity
   parameters, a colormap palette or transfer texture, and raw-value query semantics.
2. Image probes should return semantic raw values, data coordinates, visual identity, and
   latest-request-wins hover behavior.
3. Napari-style N-D slicing and thick-slice projection remain adapter-owned first; Datoviz should
   receive display-ready 2D fields and apply validated full or region updates.
