# Visual Family: `volume`

## Status

Normative target contract for the `volume` visual family. Shared attribute and behavioral
definitions live in `SHARED_ATTRIBUTES.md`; general visual rules live in
`../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`,
`../pipeline/ATTRIBUTE_SOURCES.md`, and `../semantics/VISUAL_CONTRACT.md`.

The active v0.4 path supports retained `volume` visuals backed by 3D `SampledField` resources,
volume opacity/sampling/render-mode/slice/bounds/box-clipping state, and scene -> DRP2 emission.
Default mode is full-volume compositing. MIP and slice rendering are explicit via
`render_mode = mip` and `render_mode = slice`.

Active bounded features:

- normalized `[0, 1]` public slice positions;
- shader-side axis order and axis flip;
- nearest/linear sampler selection in DRP2/vklite;
- 256x1 RGBA transfer texture from shared colormap state plus opacity stops;
- one arbitrary clipping plane plus normalized clipping box;
- GPU-backed proxy item/object picking for the rendered volume box.

DVR/MIP ray-hit picking, slice value probe/readout, isosurfaces, gradient lighting, categorical
label volumes, sparse/bricked fields, out-of-core streaming, full MPR, and WebGPU/WGSL parity are
follow-up work unless explicitly activated by a v0.4 task. Exploratory field/out-of-core
requirements live in
[`../proposals/future/FIELD_VISUALIZATION_ROADMAP.md`](../proposals/future/FIELD_VISUALIZATION_ROADMAP.md)
and [`../proposals/future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md`](../proposals/future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md).

## Purpose

`volume` renders one 3D scalar or RGBA voxel field using direct volume rendering (DVR), maximum
intensity projection (MIP), or an orthographic slice. It is a single-instance visual: all data and
style are visual-wide; there is no per-item attribute concept.

Typical uses: MRI/fMRI, CT, 3D fluorescence microscopy, density fields, scalar fields, and slice
viewers.

## Visual Parameters

| Parameter | Type | Default | Applies to | Notes |
|---|---|---|---|---|
| `field` | 3D `SampledField` | required | all | scalar `f32` or RGBA `u8` per `texture_mode` |
| `bounds_min`, `bounds_max` | `vec3` | `(-1,-1,-1)`, `(1,1,1)` | all | data-space box in visual coordinates |
| `crop_min`, `crop_max` | `vec3` | bounds | all | data-space subregion without reupload |
| `axis_order` | `ivec3` permutation | `(0,1,2)` | all | maps scene axes to texture axes |
| `axis_flip` | `bvec3` | all false | all | flips axes after ordering |
| `value_range` | `vec2` | `(0,1)` | scalar | raw value normalization |
| `alpha_transfer` | up to 8 `(value, alpha)` points | `[(0,0),(1,1)]` | scalar DVR/slice | opacity over normalized value |
| `color_transfer` | up to 8 `(value, remapped)` points | identity | scalar colormap | remaps color lookup independently |
| `opacity_scale` | `float32` | `1.0` | scalar/RGBA | multiplies final alpha |
| `colormap` | color `Scale` | none | scalar colormap | maps remapped value to color |
| `gradient_shading` | `bool` | `false` | scalar DVR | gradient-derived normal for Phong-style cues |
| `clip_plane` | normalized point/normal/side | disabled | all | one arbitrary clipping plane |
| `isosurface_levels` | up to 8 normalized `float32` values | empty | scalar DVR | target capability |
| `isosurface_colors` | up to 8 `rgba_u8` colors | white | scalar DVR | one color per active level |
| `quality` | `float32` in `(0,1]` | `0.5` | DVR | ray-marching step-size control |
| `direction` | `front_back`, `back_front` | `front_back` | DVR | compositing traversal order |
| `slice_axis` | `x`, `y`, `z` | `z` | slice | perpendicular data-space axis |
| `slice_position` | normalized `float32` in `[0,1]` | `0.5` | slice | position along `slice_axis` |

Rules:

- Scalar fields are single-channel `f32`; RGBA fields are `u8`.
- `value_range` maps raw scalar value to normalized `t = clamp((v - min) / (max - min), 0, 1)`.
- `alpha_transfer`, `color_transfer`, and `isosurface_levels` operate on normalized values.
- `opacity_scale` applies after transfer lookup or RGBA alpha sampling.
- `axis_order`/`axis_flip` avoid CPU-side swizzling for common medical/imaging layouts.
- `crop_min`/`crop_max` are in the same data-space coordinates as bounds.
- Slice data-space helper conversion may be added later without changing retained shader state.

## Transfer And Isosurface Behavior

`alpha_transfer` is piecewise-linear and clamps outside the control-point range. Common uses include
thresholds, narrow windows, and low-opacity ramps.

`color_transfer` remaps normalized values before colormap lookup, allowing contrast or histogram
windowing independent from opacity.

When `isosurface_levels` are active, the ray-casting shader detects crossings of
`sample - level`, computes hit position and gradient normal in the fragment loop, and renders the
corresponding `isosurface_colors` entry. No intermediate mesh geometry is produced. Normal alpha
compositing continues for non-surface samples.

## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| sampled volume | required | missing/incompatible field is validation error | no |
| bounds/crop/axis mapping | defaults above | invalid bounds or axis mapping is validation error | yes |
| transfer/range controls | defaults above | NaN sample maps to missing color/opacity policy | yes |
| slice controls | disabled unless slice mode | invalid axis/position is validation error | yes |
| isosurface controls | disabled unless levels set | NaN level ignored with warning | yes |

## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `texture_mode` | `scalar`, `rgba` | `scalar` |
| `color_mode` | `density`, `colormap` | `density` |
| `render_mode` | `dvr`, `mip`, `slice` | `dvr` |

Variants are set at visual creation time. `color_mode` applies only to scalar fields.

| Combination | Behavior |
|---|---|
| scalar + density | normalized value -> white, opacity from `alpha_transfer` |
| scalar + colormap | normalized value -> `color_transfer` -> colormap, opacity from `alpha_transfer` |
| RGBA | sampled directly, `opacity_scale` applied to A |
| MIP | maximum scalar along ray, mapped through `color_transfer`; ignores `alpha_transfer` |

`render_mode = multiplane` is deferred. In v0.4, MPR viewers can use three separate slice-mode
volume visuals.

## Transform, Stages, And Picking

Transform model is standard; see `SHARED_ATTRIBUTES.md`.

Picking support:

| Mode | Status | Payload |
|---|---|---|
| `slice` | proxy item/object picking installed; value probe deferred | visual id and proxy item identity |
| `dvr` | deferred unless isosurface active | none in first slice |
| `isosurface` | target support via depth readback | visual id, level index, reconstructed hit position |
| `mip` | not first-slice supported | none |

For isosurface picking, the fragment shader computes world-space hit depth; the scene reads depth
at the pick coordinate and reconstructs the 3D hit position. The item identity is the 0-based
`isosurface_levels` index.

## Related Families

| Situation | Preferred family |
|---|---|
| 2D heatmap or image overlay | `image` |
| Pre-extracted 3D surface | `mesh` |
| Shader-side isosurface in a voxel field | `volume` with `isosurface_levels` |
| Single volume slice | `volume` with `render_mode = slice` |

## Required Cases

1. scalar MRI volume with native range, e.g. `value_range = (0, 4096)`;
2. RGBA fluorescence volume;
3. threshold opacity transfer;
4. CT windowing, e.g. `value_range = (-1000, 3000)`;
5. gradient-shaded anatomical volume;
6. clipped volume revealing interior;
7. animated orthographic slice;
8. data-space sub-region crop;
9. axis-reordered MRI, e.g. `axis_order = (2, 1, 0)`;
10. mirrored axis, e.g. `axis_flip = (false, true, false)`;
11. low-quality preview, e.g. `quality = 0.1`;
12. density-field isosurface with `isosurface_levels = [0.5]`.

## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_volume_texture` | `field` (`SampledField`) |
| `dvz_volume_bounds` | `bounds_min`, `bounds_max` |
| `dvz_volume_texcoords` | `crop_min`, `crop_max` |
| `dvz_volume_permutation` | `axis_order` + `axis_flip` |
| `dvz_volume_slice` face index | `render_mode = slice`, `slice_axis`, `slice_position` |
| `dvz_volume_transfer` | `value_range`, `opacity_scale`, `alpha_transfer`, `colormap` |
| `VOLUME_TYPE_SCALAR/RGBA` | `texture_mode` |
| `VOLUME_COLOR_DIRECT/COLORMAP` | `color_mode = density/colormap` |
| `VOLUME_DIR_*` | `direction` |
| hardcoded `STEP_SIZE` | `quality` |
| volume_slice `x_cmap/y_cmap` | `color_transfer` |
| - | `gradient_shading`, `clip_plane`, `value_range` |
