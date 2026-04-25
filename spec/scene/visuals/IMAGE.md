# Visual Family: `image`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`image` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


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


### `size`

| Property | Value |
|---|---|
| Type | `vec2`, `(width, height)`, unit determined by `size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |

Display size of the image rectangle.


### `texcoords`

| Property | Value |
|---|---|
| Type | `vec4`, `(u0, v0, u1, v1)` — UV rectangle, top-left to bottom-right |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `dynamic` |
| Applies to | `texture_mode = rgba` or `scalar` only |

Portion of the texture to display. `(0, 0, 1, 1)` shows the full texture.
Use per-item values to implement texture atlases where each item samples a different region.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Fill color of the rectangle.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applies to `texture_mode = none` only. Ignored when a texture is active.


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
| Type | `Scale` reference (kind = color) — see `SCALES.md` |
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
| Default | `0.0` — sharp corners |
| Mutability | `dynamic` |

Rounded corner radius. `0` means rectangular.


### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `screen`.
Use `data` when the image should cover a fixed data-space region and scale with zoom
(e.g., a heatmap tile aligned to data axes).


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `rgba`, `scalar`, `none` | `rgba` |
| `color_mode` | `rgba`, `scalar` | `rgba` |

Both set at visual creation time.

`texture_mode` controls how the texture is sampled:

| Mode | Texture format | Color source |
|---|---|---|
| `rgba` | RGBA `u8` texture | texture sample |
| `scalar` | single-channel `f32` texture | texture sample mapped via `colormap` |
| `none` | no texture | `color` per-item fill |

`color_mode` applies only when `texture_mode = none` and determines how `color` data is encoded.
Standard — see `SHARED_ATTRIBUTES.md`.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
`angle` and `shift` are applied in screen space after the panel transform.
Picking returns the image index as item identity.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Full-panel heatmap covering axes | `image` with `size_space = data`, `texture_mode = scalar` |
| Sprite icons | `image` with `texture_mode = rgba`, per-item `texcoords` |
| Colored rectangles without texture | `image` with `texture_mode = none` |
| 3D volume rendering | `volume` |


## Minimum Cases This Spec Must Support

1. single RGBA image overlay — `texture_mode = rgba`, `CONSTANT` size and texcoords,
2. brain activity heatmap — `texture_mode = scalar` with colormap Scale,
3. texture atlas sprite sheet — `texture_mode = rgba`, `texcoords` `PER_ITEM`,
4. colored rectangle annotations — `texture_mode = none`, `color` `PER_ITEM`,
5. data-aligned heatmap tile — `size_space = data`,
6. rotated image annotations — `angle` `PER_ITEM`,
7. transposed scientific array — `transpose = true`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_image_position` | `position` `PER_ITEM` |
| `dvz_image_size` | `size` `PER_ITEM`, now also `CONSTANT` |
| `dvz_image_anchor` | `anchor` visual-wide |
| `dvz_image_texcoords` | `texcoords` `PER_ITEM`, now also `CONSTANT` |
| `dvz_image_facecolor` | `color` `PER_ITEM` when `texture_mode = none` |
| `dvz_image_texture` | `texture` resource reference |
| `dvz_image_colormap` | `colormap` Scale reference |
| `dvz_image_permutation` | `transpose` (bool) |
| `dvz_image_edgecolor` | `edgecolor` |
| `dvz_image_linewidth` | `linewidth` |
| `dvz_image_radius` | `radius` |

v0.4 adds: `angle`, `shift`, `size_space`, `texture_mode` and `color_mode` variant axes,
`CONSTANT` sources for `size` and `texcoords`, `colormap` as Scale reference instead of enum,
`transpose` replacing `permutation`.
v0.4 drops per-item `anchor` — deferred to future version.


## Deferred Questions

1. whether per-item `anchor` is needed for mixed-alignment image sets,
2. whether per-item `texture` (multiple textures in one visual) should be supported,
3. whether a `color_tint` per-item modifier on top of texture sampling is useful,
4. whether `radius` should be per-item for independently rounded images.
