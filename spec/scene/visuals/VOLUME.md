# Visual Family: `volume`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`volume` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Active Implementation Status

The active v0.4 path supports retained `volume` visuals backed by 3D `SampledField` resources,
volume opacity/sampling/render-mode/slice/bounds/box-clipping state, and scene -> DRP2 emission.
The default public mode is full-volume composite rendering. Slice rendering is explicit through
`render_mode = slice`.

The active v0.4 volume contract is intentionally powerful but bounded:

1. normalized `[0, 1]` slice positions for the public setter;
2. shader-side axis order and axis flip so examples do not swizzle large volumes on the CPU;
3. real nearest/linear sampler selection in the DRP2/vklite runtime;
4. a 256x1 RGBA transfer texture built from shared colormap state plus volume opacity stops;
5. one arbitrary clipping plane in addition to the normalized clipping box;
6. CPU slice probe/readout returning UVW, object coordinate, and sampled value.

MIP and DVR picking remain follow-up work.

Isosurfaces, gradient lighting, categorical label volumes, bricking/out-of-core streaming, full MPR,
and WebGPU/WGSL parity remain follow-up work unless a specific v0.4 task activates them.

Categorical label volumes, sparse voxel fields, and bricked/out-of-core volume residency are future
extensions of the sampled-field/volume direction. Their exploratory requirements are collected in
[`../proposals/future/FIELD_VISUALIZATION_ROADMAP.md`](../proposals/future/FIELD_VISUALIZATION_ROADMAP.md) and
[`../proposals/future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md`](../proposals/future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md).


## Semantic Purpose

`volume` renders a 3D scalar or RGBA voxel field using ray-casting direct volume rendering (DVR),
or displays a 2D slice through the volume.

Unlike other families, `volume` is a **single-instance visual** — one visual renders one volume.
There is no per-item attribute concept. All parameters are visual-wide.

Typical uses: MRI/fMRI brain volumes, CT scans, 3D fluorescence microscopy, density fields,
scalar field visualization, orthographic slice viewers.


## Visual-Wide Parameters

### `field`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource — 3D sampled field |
| Mutability | `dynamic` |

The 3D voxel data. Format must match the declared `texture_mode`:
- `scalar`: single-channel `f32` field
- `rgba`: RGBA `u8` field


### `bounds_min`, `bounds_max`

| Property | Value |
|---|---|
| Type | `vec3` each, in visual space |
| Default | `(-1,-1,-1)` and `(1,1,1)` — centered cube |
| Mutability | `dynamic` |

Data-space bounding box corners of the volume. Aligns the voxel grid to scene coordinates.
For example, an MRI volume spanning 256×256×180 mm maps to the corresponding data-space extent.


### `crop_min`, `crop_max`

| Property | Value |
|---|---|
| Type | `vec3` each, in data-space coordinates (same units as `bounds`) |
| Default | equal to `bounds_min` and `bounds_max` — no crop |
| Mutability | `dynamic` |

Restricts rendering to a sub-region of the volume in data space. Useful for focusing on a
region of interest without re-uploading the full texture.


### `axis_order`

| Property | Value |
|---|---|
| Type | `ivec3`, values in `{0, 1, 2}` |
| Default | `(0, 1, 2)` — x→U, y→V, z→W |
| Mutability | `dynamic` |

Maps scene axes to texture axes. Corrects for arrays stored in a different axis order than the
scene coordinate system (e.g., a `(z, y, x)` array needs `(2, 1, 0)`).


### `axis_flip`

| Property | Value |
|---|---|
| Type | `bvec3` |
| Default | `(false, false, false)` |
| Mutability | `dynamic` |

Flips individual axes after `axis_order` is applied. Corrects for axes stored high-to-low in
memory that should display low-to-high, or vice versa. Common in MRI data.


### `value_range`

| Property | Value |
|---|---|
| Type | `vec2` — `(min, max)` |
| Default | `(0.0, 1.0)` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar` only |

Normalizes raw voxel values to `[0, 1]` before any transfer function or colormap is applied:
`t = clamp((v - min) / (max - min), 0, 1)`.

Allows uploading data in its native range without pre-normalizing:
- MRI: `(0, 4096)` for 12-bit data
- CT: `(-1000, 3000)` for Hounsfield units
- fMRI: arbitrary float range

All `alpha_transfer` and `color_transfer` control points operate on the normalized value `t`.


### `alpha_transfer`

| Property | Value |
|---|---|
| Type | `vec2[8]` — up to 8 `(value, alpha)` control points |
| Default | `[(0, 0), (1, 1)]` — linear ramp |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar` |

Piecewise-linear opacity mapping from normalized voxel value (after `value_range`) to alpha.
Values outside the control point range clamp to the nearest endpoint.
Active for both `render_mode = dvr` and `render_mode = slice`.

Common patterns:
- `[(0, 0), (0.1, 0), (0.1, 1), (1, 1)]` — threshold: transparent below 0.1,
- `[(0.3, 0), (0.5, 1), (0.7, 1), (0.9, 0)]` — window: show only a value band,
- `[(0, 0), (1, 0.3)]` — low-opacity ramp for revealing internal structure.


### `color_transfer`

| Property | Value |
|---|---|
| Type | `vec2[8]` — up to 8 `(value, remapped_value)` control points |
| Default | `[(0, 0), (1, 1)]` — identity (no remapping) |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `color_mode = colormap` |

Piecewise-linear remapping of the normalized voxel value before colormap lookup.
Allows gamma correction, contrast enhancement, or histogram windowing on color independently
from opacity.

Example: `[(0, 0), (0.5, 0.8), (1, 1)]` — compress high values, expand low values.


### `opacity_scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |

Global multiplier applied to per-voxel alpha after `alpha_transfer` (for scalar) or after the
A channel lookup (for rgba). Applies to both `texture_mode = scalar` and `rgba`.
Convenient for quickly dimming the whole volume without redefining control points.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `semantics/SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `color_mode = colormap` |

Maps the remapped voxel value (after `color_transfer`) to a display color.


### `gradient_shading`

| Property | Value |
|---|---|
| Type | `bool` |
| Default | `false` |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `render_mode = dvr` |

When `true`, the scene estimates the local gradient of the scalar field at each sample point
and uses it as a surface normal for Phong shading. This gives strong depth cues that make the
volume appear three-dimensional rather than a flat transparent cloud.

Current shading is per-visual material/Phong/depth-cue state rather than scene-owned lights.
Requires slightly more GPU work per sample.


### `clip_plane`

| Property | Value |
|---|---|
| Type | one plane defined by normalized `point`, `normal`, and kept side |
| Default | disabled |
| Mutability | `dynamic` |

One arbitrary clipping plane in normalized volume coordinates. The plane discards one side of
`dot(normal, uvw - point)` and keeps either the positive or negative side. It combines with
`crop_min`/`crop_max` for simple cut-away anatomy and oblique cross-sections without requiring
CPU-side resampling.


### `isosurface_levels`

| Property | Value |
|---|---|
| Type | `float32[8]` — up to 8 normalized isovalue levels in `[0, 1]` (after `value_range`) |
| Default | empty — isosurface disabled |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `render_mode = dvr` |

Isovalue levels at which to render opaque surfaces inside the volume.
Values are in the normalized domain (after `value_range` remapping), matching the same scale
used by `alpha_transfer` control points.

Surfaces are computed analytically inside the fragment shader ray-casting loop — the ray
integrator detects zero-crossings of `(sample - level)` and computes the hit position and
gradient-based normal directly, without producing intermediate mesh geometry.
Multiple levels may be active simultaneously; each is rendered in the corresponding
`isosurface_colors` entry.

Setting `isosurface_levels` implicitly enables opaque surface hits at those values.
Normal alpha compositing continues for non-surface samples.


### `isosurface_colors`

| Property | Value |
|---|---|
| Type | `rgba_u8[8]` — one per `isosurface_levels` entry |
| Default | white `(255, 255, 255, 255)` for each active level |
| Mutability | `dynamic` |
| Applies to | `texture_mode = scalar`, `render_mode = dvr` |

Per-level surface color. The Phong shading model applies when `gradient_shading = true`;
otherwise the surface is flat-shaded in this color.
Entries beyond the active level count are ignored.


### `quality`

| Property | Value |
|---|---|
| Type | `float32` in `(0, 1]` |
| Default | `0.5` |
| Mutability | `dynamic` |
| Applies to | `render_mode = dvr` only |

Controls ray-marching step size. `1.0` = maximum quality (smallest steps, most samples per
ray). Lower values increase step size proportionally, trading quality for performance.


### `direction`

| Property | Value |
|---|---|
| Type | enum: `front_back`, `back_front` |
| Default | `front_back` |
| Mutability | `dynamic` |
| Applies to | `render_mode = dvr` only |

Ray-marching traversal order. `front_back` is correct for standard alpha compositing.
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
| Type | `float32`, normalized in `[0, 1]` |
| Default | `0.5` |
| Mutability | `dynamic` |
| Applies to | `render_mode = slice` only |

Position of the slice plane along `slice_axis`, normalized from the minimum to maximum coordinate
of the selected axis. Animating this produces a slice-through effect. Data-space helper conversion
can be added later without changing the retained shader state.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| sampled volume | required | missing or incompatible field is validation error | no |
| bounds/crop/axis mapping | defaults described above | invalid bounds or axis mapping is validation error | yes |
| transfer/range controls | defaults described above | NaN scalar sample maps to missing color/opacity policy | yes |
| slice controls | disabled unless `render_mode = slice` | invalid slice axis/position is validation error | yes |
| isosurface controls | disabled unless levels are present | NaN level ignored with warning | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `scalar`, `rgba` | `scalar` |
| `color_mode` | `density`, `colormap` | `density` |
| `render_mode` | `dvr`, `mip`, `slice` | `dvr` |

All set at visual creation time. `color_mode` applies to `texture_mode = scalar` only.

`render_mode = multiplane` (multiple simultaneous orthogonal slices, MPR viewer) is deferred
to v0.4+. MPR viewers can be approximated in v0.4 with three separate `volume` visuals each
using `render_mode = slice` on different axes.

| `texture_mode` | `color_mode` | Behavior |
|---|---|---|
| `scalar` | `density` | normalized value → white, opacity from `alpha_transfer` |
| `scalar` | `colormap` | normalized value → `color_transfer` → colormap; opacity from `alpha_transfer` |
| `rgba` | — | RGBA voxel sampled directly; `opacity_scale` applied to A channel |

`render_mode = mip` (maximum intensity projection) accumulates the maximum voxel value along
each ray rather than compositing with alpha. The result is mapped through `color_transfer` for
display. `alpha_transfer` is ignored in MIP mode. MIP applies to `texture_mode = scalar` only.


## Transform Model, Stage Participation, Picking

Standard transform model — see `SHARED_ATTRIBUTES.md`.

**Picking for `render_mode = slice`**: supported for probe/readout workflows.
The result should identify the volume visual, the active slice state, slice-local coordinates,
scene-domain coordinates when available, and the sampled value at the queried position.
Persistent slice sample selection may reuse the same payload shape when enabled.

**Picking for `render_mode = dvr`**: deferred for the first implementation slice unless an
isosurface level is active.

**Picking for isosurface** (`isosurface_levels` non-empty): supported via depth readback.
The fragment shader computes the world-space hit position when a ray intersects an isosurface
level. The scene reads back the depth buffer at the pick coordinate and reconstructs the 3D
hit position. The returned item identity is the isosurface level index (0-based index into
`isosurface_levels`). This is a single-object pick — one volume, one level index.

**Picking for `render_mode = mip`**: not supported in the first implementation slice.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| 2D heatmap or image overlay | `image` |
| Pre-extracted 3D surface mesh | `mesh` |
| Isosurface rendered inside the volume | `volume` with `isosurface_levels` (fragment shader, no intermediate mesh) |
| Single volume slice | `volume` with `render_mode = slice` |


## Minimum Cases This Spec Must Support

1. scalar MRI volume, native 12-bit range — `value_range = (0, 4096)`, `color_mode = colormap`,
2. RGBA fluorescence volume — `texture_mode = rgba`,
3. threshold opacity — `alpha_transfer` with step at threshold,
4. CT windowing — `alpha_transfer` narrow band + `value_range = (-1000, 3000)`,
5. gradient-shaded anatomical volume — `gradient_shading = true`,
6. clipped volume to reveal interior — one `clip_planes` entry set,
7. orthographic slice, animated — `render_mode = slice`, animated `slice_position`,
8. sub-region crop — `crop_min`/`crop_max` in data space,
9. axis-reordered MRI — `axis_order = (2, 1, 0)`,
10. mirrored axis — `axis_flip = (false, true, false)`,
11. low-quality preview — `quality = 0.1`,
12. isosurface of a density field — `isosurface_levels = [0.5]`, `isosurface_colors = [(200, 200, 200, 255)]`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_volume_texture` | `field` (`SampledField`) resource |
| `dvz_volume_bounds` | `bounds_min`, `bounds_max` |
| `dvz_volume_texcoords` (uvw0/uvw1) | `crop_min`, `crop_max` (data space) |
| `dvz_volume_permutation` | `axis_order` + `axis_flip` |
| `dvz_volume_slice` (face index 0–5) | `render_mode = slice`, `slice_axis`, `slice_position` |
| `dvz_volume_transfer` (vec4) | `value_range` + `opacity_scale` + `alpha_transfer` + `colormap` |
| `VOLUME_TYPE_SCALAR/RGBA` | `texture_mode` variant axis |
| `VOLUME_COLOR_DIRECT/COLORMAP` | `color_mode = density/colormap` variant axis |
| `VOLUME_DIR_*` | `direction` parameter |
| `STEP_SIZE` hardcoded | `quality` parameter |
| volume_slice `x_cmap/y_cmap` | `color_transfer` |
| — | `gradient_shading`, `clip_planes` (up to 4), `value_range` (new in v0.4) |
