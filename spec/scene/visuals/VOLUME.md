# Visual Family: `volume`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`volume` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`volume` renders a 3D scalar or RGBA voxel field using ray-casting direct volume rendering (DVR).

Unlike other families, `volume` is a **single-instance visual** — one visual renders one volume.
There is no per-item attribute concept. All parameters are visual-wide.

Typical uses: MRI/fMRI brain volumes, CT scans, 3D fluorescence microscopy, density fields,
scalar field visualization.


## Visual-Wide Parameters

### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource — 3D texture |
| Mutability | `dynamic` |

The 3D voxel data. Format must match the declared `texture_mode`:
- `scalar`: single-channel `f32` texture
- `rgba`: RGBA `u8` texture


### `bounds`

| Property | Value |
|---|---|
| Type | two `vec3` corners: `(x0, y0, z0)` and `(x1, y1, z1)` in visual space |
| Default | unit cube `(0,0,0)` to `(1,1,1)` |
| Mutability | `dynamic` |

Data-space bounding box of the volume. Aligns the voxel grid to scene coordinates.
For example, an MRI volume covering a physical extent maps directly to data space via these
bounds.


### `texcoords`

| Property | Value |
|---|---|
| Type | two `vec3` UVW corners: `(u0,v0,w0)` and `(u1,v1,w1)` in `[0,1]` |
| Default | `(0,0,0)` to `(1,1,1)` — full texture |
| Mutability | `dynamic` |

Selects a sub-region of the 3D texture to render. Useful for cropping or streaming a
sub-volume without re-uploading the full texture.


### `axis_order`

| Property | Value |
|---|---|
| Type | `ivec3`, e.g. `(0, 1, 2)` for natural order |
| Default | `(0, 1, 2)` — x→U, y→V, z→W |
| Mutability | `dynamic` |

Maps data axes to texture axes. Useful when voxel arrays are stored in a different axis order
than the scene coordinate system (e.g., a `(z, y, x)` array in memory needs `(2, 1, 0)` to
display correctly).

Common MRI orientation corrections are handled by choosing the appropriate permutation.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `color_mode = colormap` |

Maps voxel scalar values to display colors. Domain and palette can be updated without
re-uploading voxel data.


### `opacity_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |

Global multiplier applied to per-voxel alpha after colormap or direct lookup.
Lower values make the volume more transparent, useful for seeing internal structures.


### `slice`

| Property | Value |
|---|---|
| Type | `int32`, face index `0`–`5`, or `-1` to disable |
| Default | `-1` — full ray-cast rendering |
| Mutability | `dynamic` |

When set to a face index (0–5, the six faces of the bounding box), the ray marcher stops at
that face and renders only the intersection plane — producing a 2D slice through the volume.
Useful for interactive slice viewers and orthographic cross-section displays.

Face indices: `0`=−X, `1`=+X, `2`=−Y, `3`=+Y, `4`=−Z, `5`=+Z.


### `direction`

| Property | Value |
|---|---|
| Type | enum: `front_back`, `back_front` |
| Default | `front_back` |
| Mutability | `dynamic` |

Ray marching traversal order. `front_back` is standard (correct for alpha compositing).
`back_front` is useful for certain emission/absorption models.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `scalar`, `rgba` | `scalar` |
| `color_mode` | `direct`, `colormap` | `direct` |

Both set at visual creation time. `color_mode` applies to `texture_mode = scalar` only.

| `texture_mode` | `color_mode` | Behavior |
|---|---|---|
| `scalar` | `direct` | voxel value → white voxel, value used as alpha |
| `scalar` | `colormap` | voxel value → colormap color, value used as alpha |
| `rgba` | — | RGBA voxel sampled directly, alpha from A channel |


## Transform Model, Stage Participation, Picking

Standard transform model — see `SHARED_ATTRIBUTES.md`.
Picking is not supported for `volume` (ray-voxel intersection is not exposed as item identity).


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| 2D heatmap or image overlay | `image` |
| 3D surface from isosurface extraction | `mesh` |
| Volume slice only (no ray casting) | `volume` with `slice` parameter |


## Minimum Cases This Spec Must Support

1. scalar MRI volume with colormap — `texture_mode = scalar`, `color_mode = colormap`,
2. RGBA fluorescence volume — `texture_mode = rgba`,
3. opacity-adjusted brain volume — `opacity_scale < 1`,
4. orthographic slice view — `slice = 2` (−Y face),
5. sub-region crop — `texcoords` set to a sub-cube,
6. axis-reordered MRI — `axis_order = (2, 1, 0)`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_volume_texture` | `texture` resource |
| `dvz_volume_bounds` | `bounds` (xlim/ylim/zlim → two corner vec3) |
| `dvz_volume_texcoords` | `texcoords` (uvw0/uvw1) |
| `dvz_volume_permutation` | `axis_order` |
| `dvz_volume_slice` | `slice` parameter |
| `dvz_volume_transfer` | `opacity_scale` + `colormap` Scale |
| `VOLUME_TYPE_SCALAR/RGBA` specialization | `texture_mode` variant axis |
| `VOLUME_COLOR_DIRECT/COLORMAP` specialization | `color_mode` variant axis |
| `VOLUME_DIR_FRONT_BACK/BACK_FRONT` specialization | `direction` parameter |

v0.4 adds: `colormap` as Scale reference, `direction` as user parameter, renamed `bounds`
and `axis_order` for clarity.
v0.4 splits `transfer` vec4 into `opacity_scale` (float) and `colormap` Scale.


## Deferred Questions

1. whether maximum intensity projection (MIP) should be a `render_mode` variant axis
   (`dvr` vs `mip`) — MIP is common in medical imaging and requires a different accumulation
   loop,
2. whether a full piecewise-linear transfer function (separate color and alpha control points)
   should replace or extend `opacity_scale`,
3. whether picking should be supported via a ray-marching depth readback.
