# Visual Family: `volume`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`volume` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`volume` renders a 3D scalar or RGBA voxel field using ray-casting direct volume rendering (DVR),
or displays a 2D slice through the volume.

Unlike other families, `volume` is a **single-instance visual** — one visual renders one volume.
There is no per-item attribute concept. All parameters are visual-wide.

Typical uses: MRI/fMRI brain volumes, CT scans, 3D fluorescence microscopy, density fields,
scalar field visualization, orthographic slice viewers.


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


### `crop_min`, `crop_max`

| Property | Value |
|---|---|
| Type | `vec3` each, UVW coordinates in `[0, 1]` |
| Default | `(0,0,0)` and `(1,1,1)` — full texture |
| Mutability | `dynamic` |

Selects a sub-region of the 3D texture to render. Useful for cropping or streaming a
sub-volume without re-uploading the full texture.


### `axis_order`

| Property | Value |
|---|---|
| Type | `ivec3`, values in `{0, 1, 2}` |
| Default | `(0, 1, 2)` — x→U, y→V, z→W |
| Mutability | `dynamic` |

Maps data axes to texture axes. Corrects for arrays stored in a different axis order than the
scene coordinate system (e.g., a `(z, y, x)` array needs `(2, 1, 0)`).


### `axis_flip`

| Property | Value |
|---|---|
| Type | `bvec3` |
| Default | `(false, false, false)` |
| Mutability | `dynamic` |

Flips individual axes after `axis_order` is applied. Corrects for axes that are stored
high-to-low in memory but should display low-to-high (or vice versa). Common in MRI data
where the Y axis may run inferior→superior in the array but needs to display the other way.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `color_mode = colormap` |

Maps voxel scalar values to display colors. Domain and palette can be updated without
re-uploading voxel data.


### `alpha_transfer`

| Property | Value |
|---|---|
| Type | up to 8 `(value, alpha)` control points, `float32` pairs |
| Default | `[(0, 0), (1, 1)]` — linear ramp |
| Mutability | `dynamic` |

Piecewise-linear opacity mapping from voxel value to alpha. Values between control points
are linearly interpolated; values outside the range clamp to the nearest endpoint.

Common patterns:
- `[(0, 0), (0.1, 0), (0.1, 1), (1, 1)]` — threshold: transparent below 0.1, opaque above,
- `[(0.3, 0), (0.5, 1), (0.7, 1), (0.9, 0)]` — window: show only a value band,
- `[(0, 0), (1, 0.3)]` — low-opacity ramp for seeing internal structure.

Applies to `texture_mode = scalar` only. For `rgba`, alpha comes from the A channel.


### `opacity_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |

Global multiplier applied to per-voxel alpha after the `alpha_transfer` function.
Convenient for quickly adjusting overall transparency without redefining control points.


### `quality`

| Property | Value |
|---|---|
| Type | `float32` in `(0, 1]` |
| Default | `0.5` |
| Mutability | `dynamic` |

Controls the ray-marching step size: `1.0` gives maximum quality (smallest step, most samples),
lower values increase step size and reduce quality proportionally.
Applies to `render_mode = dvr` only; ignored for `slice`.


### `direction`

| Property | Value |
|---|---|
| Type | enum: `front_back`, `back_front` |
| Default | `front_back` |
| Mutability | `dynamic` |
| Applies to | `render_mode = dvr` only |

Ray-marching traversal order. `front_back` is standard (correct for alpha compositing).
`back_front` is useful for emission/absorption models.


### `slice_axis`

| Property | Value |
|---|---|
| Type | enum: `x`, `y`, `z` |
| Default | `z` |
| Mutability | `dynamic` |
| Applies to | `render_mode = slice` only |

Which data-space axis the slice plane is perpendicular to.


### `slice_position`

| Property | Value |
|---|---|
| Type | `float32`, in data-space coordinates |
| Default | midpoint of the corresponding bounds axis |
| Mutability | `dynamic` |
| Applies to | `render_mode = slice` only |

Position of the slice plane along `slice_axis`, in the same units as `bounds`.
Animating this value produces a slice-through effect.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `scalar`, `rgba` | `scalar` |
| `color_mode` | `density`, `colormap` | `density` |
| `render_mode` | `dvr`, `slice` | `dvr` |

All set at visual creation time. `color_mode` applies to `texture_mode = scalar` only.

| `texture_mode` | `color_mode` | Behavior |
|---|---|---|
| `scalar` | `density` | voxel value → white, value mapped through `alpha_transfer` |
| `scalar` | `colormap` | voxel value → colormap color, value mapped through `alpha_transfer` |
| `rgba` | — | RGBA voxel sampled directly, A channel used as alpha |


## Transform Model, Stage Participation, Picking

Standard transform model — see `SHARED_ATTRIBUTES.md`.
Picking is not supported for `volume`.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| 2D heatmap or image overlay | `image` |
| 3D surface from isosurface extraction | `mesh` |
| Single volume slice | `volume` with `render_mode = slice` |


## Minimum Cases This Spec Must Support

1. scalar MRI volume with colormap — `texture_mode = scalar`, `color_mode = colormap`,
2. RGBA fluorescence volume — `texture_mode = rgba`,
3. threshold-based opacity — `alpha_transfer` with step at threshold value,
4. CT windowing — `alpha_transfer` with a narrow band of opaque values,
5. orthographic slice view — `render_mode = slice`, animated `slice_position`,
6. sub-region crop — `crop_min`/`crop_max` set to a sub-cube,
7. axis-reordered MRI — `axis_order = (2, 1, 0)`,
8. mirrored axis — `axis_flip = (false, true, false)`,
9. low-quality preview — `quality = 0.1`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_volume_texture` | `texture` resource |
| `dvz_volume_bounds` | `bounds` |
| `dvz_volume_texcoords` (uvw0/uvw1) | `crop_min`, `crop_max` |
| `dvz_volume_permutation` | `axis_order` + `axis_flip` |
| `dvz_volume_slice` (face index) | `render_mode = slice`, `slice_axis`, `slice_position` |
| `dvz_volume_transfer` (vec4) | `opacity_scale` + `alpha_transfer` + `colormap` Scale |
| `VOLUME_TYPE_SCALAR/RGBA` specialization | `texture_mode` variant axis |
| `VOLUME_COLOR_DIRECT/COLORMAP` specialization | `color_mode = density/colormap` variant axis |
| `VOLUME_DIR_FRONT_BACK/BACK_FRONT` specialization | `direction` parameter |
| `STEP_SIZE` hardcoded | `quality` parameter |

v0.4 improvements: `slice` replaced by axis+position in data space; `permutation` split into
`axis_order` + `axis_flip`; `transfer` vec4 replaced by `alpha_transfer` control points +
`opacity_scale`; `color_mode = direct` renamed to `density`; step size exposed as `quality`.


## Deferred Questions

1. whether maximum intensity projection (MIP) should be a `render_mode` variant (`dvr`,
   `mip`, `slice`) — common in medical imaging, requires a different accumulation loop,
2. whether multiple simultaneous slices (e.g., three orthogonal planes) should be supported
   as a `render_mode = multiplane` variant,
3. whether picking should be supported via ray-marching depth readback.
