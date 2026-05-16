# Scene Image, Labels, Colormap, and Layer Compositing Plan for napari-Class Support

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Created on:** `2026-05-16`
> - **Scope:** 2D image visuals, label images, integer textures, colormaps, palettes, filtering,
>   blending, multiscale image data, and napari-style 2D layer compositing.


## Goal

Make Datoviz v0.4 image rendering capable enough to serve as the rendering core for napari-style
`Image` and `Labels` layers.

The target is not to copy napari's Python layer model into Datoviz. The target is to expose a small,
stable, incremental scene/resource contract that a napari adapter can drive:

```text
napari Image / Labels layer state
    -> Datoviz SampledField + Scale + Visual state
    -> FramePlan
    -> DRP2 stream
    -> Vulkan / WebGPU runtime
```

The high-value first outcome is a correct and fast 2D layer stack:

```text
scalar image + contrast limits + colormap + opacity + layer blend mode
RGBA image + opacity + layer blend mode
integer labels + stable id-to-color palette + background id transparency
dirty-region texture updates
probe/readback returning data coordinate and scalar/label value
```


## napari Capability Target

As of the current napari docs/source checked on `2026-05-16`, the relevant behavior is:

1. `Image` layers accept N-dimensional scalar or RGB/RGBA data, list-backed multiscale data, affine
   transforms, opacity, colormap, contrast limits, gamma, interpolation, and projection mode.
2. Scalar `Image` layers map raw values through contrast limits and a colormap. RGB/RGBA images
   ignore contrast limits and colormap.
3. `Image` blending modes include `translucent`, `translucent_no_depth`, `additive`, `minimum`, and
   `opaque`.
4. `Image` 2D interpolation includes nearest, linear, cubic, Lanczos-like and other vispy spatial
   filters, plus a custom 2D kernel path.
5. Thick-slice projection modes include none, sum, mean, max, and min, but GPU projection is out of
   scope for this 2D image/labels plan.
6. `Labels` layers are integer or bool arrays. Label `0` is the transparent background. Label colors
   may be generated from a seed or supplied directly. Labels use nearest sampling and categorical
   rendering semantics.
7. `Labels` controls include opacity, blending, auto/direct color mode, contour display,
   selected-label-only display, editable paint/fill behavior, and label-id readout.

References:

- napari Image guide and API: `https://napari.org/stable/howtos/layers/image.html`,
  `https://napari.org/dev/api/napari.layers.Image.html`
- napari Image source constants:
  `https://raw.githubusercontent.com/napari/napari/main/src/napari/layers/image/_image_constants.py`
- napari Labels guide:
  `https://napari.org/dev/howtos/layers/labels.html`
- napari Layer blending API:
  `https://napari.org/0.4.19/api/napari.layers.Layer.html`
- napari Colormap API:
  `https://napari.org/dev/api/napari.utils.Colormap.html`


## Current Datoviz Starting Point

Datoviz already has several necessary pieces:

1. `DvzSampledField` supports 2D and 3D dimensions, scalar/color/label semantics, many integer and
   float formats, full replacement, resize, and subregion updates.
2. `DvzScale`, `DvzColormap`, and colorbar bookkeeping already exist and image visuals can bind a
   `"colormap"` scale.
3. Image visuals can bind a `"field"` sampled field and emit texture uploads, including dirty
   subregions and texture reallocation after resize.
4. The scene path already emits image draws through FramePlan -> DRP2 -> runtime.
5. Probe/readback exists for a first image-probe path.
6. `DvzAlphaMode` now covers opaque, ordinary source-over blending, WBOIT, depth peeling, and mask.
7. DRP2 and vklite have render pipeline blend-state plumbing.
8. Volume work has a separate plan and remains outside this document.

Important gap: the current image shader is still a simple sampled-RGBA texture shader. It does not
yet encode napari-class scalar colormap lookup, integer label palette lookup, contrast/gamma state,
low/high/nan colors, contour mode, selected-label mode, or napari blend modes beyond ordinary
alpha modes.


## Design Principles

1. `SampledField` is the authoritative 2D image and label data object.
2. Raw source data should stay raw whenever practical. Colormap, contrast, gamma, opacity, and
   selected-label state should be cheap parameter or palette updates.
3. CPU colorization is a fallback and utility path, not the default runtime path for large data.
4. Labels are categorical data, not scalar heatmaps. Integer id preservation is required for
   nearest sampling, palette lookup, painting, selection, and probe readout.
5. Layer compositing must be separate from fragment alpha mode. napari blend modes are layer/output
   compositing policies, not only transparency categories.
6. Datoviz should not own N-dimensional napari slicing first. The adapter can provide display-ready
   2D planes; Datoviz must support the resulting field updates efficiently.
7. Every feature should lower through FramePlan and DRP2. Do not add image-specific Vulkan calls in
   scene or app code.


## Proposed Public Concepts

These names are directional. Keep API shape consistent with the rest of v0.4 when implementing.

### Image Sample Kind

```c
typedef enum
{
    DVZ_IMAGE_SAMPLE_RGBA = 0,
    DVZ_IMAGE_SAMPLE_SCALAR,
    DVZ_IMAGE_SAMPLE_LABEL,
} DvzImageSampleKind;
```

The kind can be inferred from `DvzSampledFieldDesc.semantic`, but an explicit retained visual state
is useful for validation, shader variant selection, and diagnostics.


### Image Sampler Mode

```c
typedef enum
{
    DVZ_IMAGE_FILTER_NEAREST = 0,
    DVZ_IMAGE_FILTER_LINEAR,
    DVZ_IMAGE_FILTER_CUBIC,
    DVZ_IMAGE_FILTER_LANCZOS,
    DVZ_IMAGE_FILTER_CUSTOM_2D,
} DvzImageFilterMode;
```

First implementation should support nearest and linear. Higher-order filters can be shader variants
or CPU/GPU preprocess passes later.


### Layer Blend Mode

```c
typedef enum
{
    DVZ_BLEND_NORMAL = 0,
    DVZ_BLEND_ADDITIVE,
    DVZ_BLEND_MINIMUM,
    DVZ_BLEND_MAXIMUM,
} DvzBlendMode;
```

This is distinct from `DvzAlphaMode`.


### Layer Depth Policy

```c
typedef enum
{
    DVZ_DEPTH_DEFAULT = 0,
    DVZ_DEPTH_TEST_WRITE,
    DVZ_DEPTH_TEST_NO_WRITE,
    DVZ_DEPTH_DISABLED,
} DvzDepthMode;
```

napari 2D layer ordering should usually use deterministic `z_layer` plus disabled depth.


### Image Color State

The image visual needs retained state equivalent to:

```text
sample_kind
filter_mode_2d
blend_mode
depth_mode
opacity
contrast_min / contrast_max
gamma
low_color / high_color / nan_color
label_background_id
selected_label_id
show_selected_only
contour_width
palette_generation
```

Some of this already lives in `DvzScale` / `DvzColormap`; avoid duplicating semantic state. The
visual should carry only rendering policy that is not scale-owned.


## GPU vs CPU Responsibilities

### GPU Default Path

Use GPU lookup for interactive layers:

```text
raw scalar texture + palette texture + params -> final RGBA fragment
raw integer label texture + palette/hash texture + params -> final RGBA fragment
raw RGBA texture + params -> final RGBA fragment
```

GPU path requirements:

1. sampled 2D texture or integer texture,
2. sampler state or manual texel fetch according to filter mode,
3. palette/colormap resource,
4. small parameter uniform,
5. shader variants for scalar, RGBA, and label sample kinds,
6. pipeline variants for blend/depth policy.


### CPU Fallback Path

Use CPU colorization only when:

1. the runtime cannot sample the needed raw format,
2. integer textures are unavailable or unsupported by the backend,
3. high-order filtering is requested before a shader implementation exists,
4. generating thumbnails, static exports, or test oracles,
5. an adapter asks for precomposited data explicitly.

CPU fallback should produce `RGBA8_UNORM` sampled fields and emit a diagnostic that the visual is
not in the preferred raw-data path.


## Staged Implementation Plan

### Stage 0 - Tighten the Existing Image Contract

Purpose: make current behavior explicit before adding variants.

Work:

1. Update `spec/scene/visuals/IMAGE.md` to distinguish `RGBA`, `SCALAR`, and `LABEL` sample kinds.
2. Record the shader binding ABI for image visuals in the shader ABI plan:
   - common set,
   - image data texture,
   - sampler,
   - optional palette/colormap texture,
   - image params uniform.
3. Add tests proving field semantic drives expected image sample kind.
4. Add diagnostics for missing field, missing scale on scalar images, and invalid label formats.

Validation:

```text
just build
just test scene
git diff --check
```


### Stage 1 - Scalar Image Colormap on the GPU

Purpose: render napari-style scalar images without CPU pre-colorization.

Work:

1. Add an image params uniform with contrast limits, gamma, opacity, missing-value colors, and flags.
2. Add a compact palette texture generated from `DvzScale` + `DvzColormap`.
3. Extend image shader variants:
   - RGBA path: current behavior plus opacity.
   - scalar path: sample raw scalar, normalize by contrast/view range, apply gamma, lookup palette.
4. Keep scale/colormap dirty state independent from sampled-field dirty state.
5. Ensure updating contrast limits or colormap emits only a small palette/params update, not a full
   data texture upload.
6. Add WGSL variant at the same time or capability-gate WGSL with a clear diagnostic.

Tests:

1. Scalar `R32_FLOAT` image emits data texture, palette texture, and image params.
2. Contrast update does not emit `WriteTexture` for the raw data texture.
3. Colormap update emits palette/params only.
4. GLSL runtime smoke renders two contrast windows differently.
5. Probe returns the raw scalar value, not the colorized value.


### Stage 2 - Integer Label Textures and Categorical Palette Lookup

Purpose: support napari `Labels` rendering semantics.

Work:

1. Accept `DVZ_FIELD_SEMANTIC_LABEL` with `R8_UINT`, `R16_UINT`, `R32_UINT`, and signed equivalents
   where backend support exists.
2. Force nearest sampling or texel fetch for label images.
3. Implement categorical lookup:
   - direct palette table for bounded id ranges,
   - hash/seed color generation for sparse or very large ids,
   - background id `0` alpha `0` by default,
   - optional explicit color dictionary loaded into a palette table.
4. Add label params:
   - selected label,
   - show selected only,
   - contour width,
   - seed/generation,
   - background id.
5. Add contour rendering. First slice can use a fragment-shader neighbor check over nearest texels.
6. Ensure label probes return `DVZ_PROBE_VALUE_LABEL` and the integer id.

Tests:

1. Label id `0` is transparent.
2. Non-zero ids produce stable colors for a seed.
3. Direct palette overrides hash colors.
4. `show_selected_only` hides non-selected ids.
5. Contour mode draws only boundaries for a simple label patch.
6. Partial painting-style updates emit one texture subregion upload.
7. Probe returns label id and not normalized scalar intensity.


### Stage 3 - Layer Blend Modes Separate From Alpha Modes

Purpose: map napari 2D layer blending accurately enough for image stacks.

Mapping target:

| napari mode | Datoviz alpha/depth/blend target |
|---|---|
| `opaque` | alpha opaque, blend normal/off, depth disabled or deterministic 2D policy |
| `translucent` | source-over normal blend, depth test/write policy configurable |
| `translucent_no_depth` | source-over normal blend, depth disabled |
| `additive` | additive color blend, depth disabled for 2D layers |
| `minimum` | min blend op, depth disabled for 2D layers |

Work:

1. Add `DvzBlendMode` retained visual state.
2. Add `DvzDepthMode` or equivalent retained depth policy.
3. Extend visual pass capability resolution to account for blend mode separately from alpha mode.
4. Emit DRP2 blend state for normal/source-over, additive, and minimum.
5. Keep WBOIT/depth-peel as explicit advanced alpha modes for geometry/volume, not napari's default
   2D image stack behavior.
6. Add deterministic `z_layer` ordering tests for multiple image/label layers.

Tests:

1. Two translucent images compose in z-layer order.
2. Additive mode uses `src_alpha, one` blend factors or equivalent premultiplied policy.
3. Minimum mode emits `VK_BLEND_OP_MIN` or backend equivalent.
4. `translucent_no_depth` disables depth testing/writing.
5. Changing opacity updates params only.
6. Changing blend mode invalidates pipeline/pass state but not texture data.


### Stage 4 - Filtering and Interpolation

Purpose: cover napari's practical image interpolation surface.

First slice:

1. nearest,
2. linear,
3. label-nearest enforcement.

Second slice:

1. cubic,
2. Lanczos,
3. selected vispy-like spatial filters,
4. custom 2D kernel.

Implementation options:

1. Native sampler for nearest/linear.
2. Fragment shader manual sampling for cubic/Lanczos/custom.
3. Compute prefilter/downsample path for expensive kernels.
4. CPU fallback for uncommon filters when correctness matters more than interactivity.

Tests:

1. Label visual rejects linear filtering or coerces it to nearest with a diagnostic.
2. Scalar image nearest and linear produce different captures on a small checkerboard.
3. Custom kernel path is capability-gated until implemented.


### Stage 5 - Multiscale Images, Dirty Regions, and Tile Policy

Purpose: make large napari images practical.

Work:

1. Extend `DvzSampledField` or introduce a `DvzSampledFieldPyramid` wrapper for multiple resolution
   levels.
2. Allow a visual to bind one active field level while preserving stable logical layer identity.
3. Add viewport-driven level selection hooks, but let the napari adapter make the first selection.
4. Preserve dirty-region updates per level.
5. Add stale generation numbers for async tile delivery.
6. Add row pitch and offset validation for every update.

First implementation policy:

```text
adapter picks level -> Datoviz uploads that level -> Datoviz draws one texture
```

Later policy:

```text
Datoviz can select level from panel transform and available field pyramid metadata
```

Tests:

1. Swapping active levels reallocates texture and preserves visual/layer identity.
2. A stale tile generation is rejected.
3. Updating one tile does not re-upload the full level.


### Stage 6 - N-D Slice Replacement Boundary

Purpose: support napari display-ready 2D slices without taking over napari's data model.

Work:

1. Host/adapter provides a 2D slice as a sampled field.
2. Datoviz displays it and returns probes in the current slice coordinate frame.
3. The adapter maps back to full N-D coordinates.
4. Thick-slice projection remains adapter-owned for this plan.

Tests:

1. Slice replacement updates texture without changing visual identity.
2. Probe contains enough coordinate metadata for the adapter to recover N-D indices.


### Out Of Scope

napari 3D image, volume, and 3D labels behavior belongs in
`agents/now/SCENE_VOLUME_RENDERING_PLAN.md`.

This plan only defines the 2D sampled-field, colormap, palette, filtering, probe, and blend-mode
pieces that the volume plan may later reuse.


### Stage 7 - Picking, Probing, and Editing Hooks

Purpose: make images and labels usable interactively.

Work:

1. Image probe returns raw scalar/RGBA value, data coordinate, visual id, field id, and scale.
2. Label probe returns raw label id, selected/direct color metadata where available, and data
   coordinate.
3. Add latest-request-wins hover behavior for image/label probes.
4. Add region update helpers suitable for painting and fill operations.
5. Keep fill/paint algorithms outside Datoviz initially. Datoviz only applies validated region
   updates and provides readback/probe support.

Tests:

1. Hover probe does not retain stale layer pointers after layer removal.
2. Label paint region update is bounds-checked.
3. Fill-style repeated updates do not grow transient runtime resources over many frames.


### Stage 8 - Examples and Adapter Pressure Tests

Examples to add in order:

1. `hello_image_colormap_glfw.c`: scalar image, contrast/gamma/colormap controls.
2. `hello_labels_glfw.c`: integer labels, seed shuffle, direct palette, selected-only, contour.
3. `hello_image_layers_glfw.c`: image + labels + second image with blend-mode controls.
4. `hello_large_image_tiles_glfw.c`: dirty tile updates and level switching.
5. napari adapter smoke outside core repo or under `examples/python/experimental` if bindings are
   available.

Required captures:

1. scalar colormap capture,
2. labels palette capture,
3. blend-mode comparison capture,
4. probe/readout smoke.


## Capability Matrix

| Capability | Required for first napari-class 2D | Existing | Action |
|---|---:|---:|---|
| 2D sampled fields | yes | yes | harden |
| field resize and region updates | yes | yes | harden and add label tests |
| scalar raw texture upload | yes | partial | add shader lookup path |
| RGBA texture upload | yes | yes | add opacity/blend params |
| integer label texture upload | yes | partial | add label shader/palette path |
| colormap/scale object | yes | yes | add GPU palette texture |
| contrast limits without data reupload | yes | partial | params/palette-only updates |
| gamma | yes | no | shader param |
| low/high/nan colors | yes | no | shader param + scale policy |
| label background transparent | yes | no | label shader rule |
| label direct color dictionary | yes | no | palette table |
| label hash/seed colors | yes | no | shader/CPU hash policy |
| selected label display | yes | no | label params |
| contour mode | yes | no | shader neighbor check |
| nearest/linear filtering | yes | partial | sampler policy |
| high-order filters | no, later | no | shader/CPU fallback |
| napari blend modes | yes | partial | add blend mode separate from alpha |
| z-layer order | yes | yes | add image stack tests |
| image probe | yes | partial | raw value semantics |
| label probe | yes | no | return label id |
| multiscale 2D | yes | no | pyramid/active-level policy |
| N-D slice replacement | adapter first | no | host-driven first |


## Recommended Implementation Order

1. Stage 1 scalar colormap GPU path.
2. Stage 3 blend-mode separation for image stacks.
3. Stage 2 label image path.
4. Stage 7 probe/readout completion for image and labels.
5. Stage 5 multiscale and tile updates.
6. Stage 4 high-order filtering.
7. Stage 6 slice replacement boundary.
8. Keep volume/3D labels in `SCENE_VOLUME_RENDERING_PLAN.md`.

Reasoning: scalar images and blending unlock the broadest napari `Image` layer surface. Labels are
the next largest differentiator, but they need the same palette/params/blend infrastructure.


## Validation Policy

For every stage:

```text
git diff --check
just build
focused ./build/testing/dvztest_scene <filter>
focused ./build/testing/dvztest_drp2 <filter> when DRP2 schema/runtime changes
```

For shader/runtime stages:

```text
bounded GLFW/offscreen smoke
capture comparison against a small CPU oracle
Vulkan validation layer smoke when touching runtime, image layout, descriptors, or synchronization
```

For WebGPU/WGSL stages:

```text
just spec-check
webgpu fixture preflight
WGSL fixture export for scalar image and label image
```


## Open Decisions

1. Should labels be an image mode or a separate `label` visual family?
   - Recommended first implementation: image mode.
   - Recommended long-term API: consider a `label` visual only if label-specific retained state
     makes the generic image visual too broad.
2. Should palette lookup use a texture, storage buffer, or uniform array?
   - Recommended first implementation: 1D/2D texture for broad graphics compatibility.
   - Use storage buffers later for sparse direct dictionaries if needed.
3. How exact should napari's high-order filters be?
   - Recommended first implementation: nearest/linear exact, high-order capability-gated.
   - Add test oracles only when those filters become implementation targets.
4. Should `minimum` blend be available for all visuals?
   - Recommended first implementation: image/label visuals only.
   - Generalize after pipeline state coverage is stable.
5. How much N-D slicing belongs in Datoviz?
   - Recommended first implementation: none. Adapter provides 2D display data.
   - Add GPU projection only through a separate volume/projection plan, not this document.


## Success Criteria

The Datoviz scene stack is napari-image-ready when the following are true:

1. A scalar image layer can update contrast limits, gamma, colormap, opacity, and visibility without
   re-uploading the raw data texture.
2. An RGBA image layer can update opacity and blend mode without re-uploading image data.
3. A labels layer can display integer ids with transparent background `0`, deterministic generated
   colors, direct palette overrides, selected-only display, contour mode, nearest sampling, and
   partial paint-style updates.
4. A multi-layer image/labels stack can reproduce napari's practical 2D blend modes: opaque,
   translucent, translucent-no-depth, additive, and minimum.
5. Probe/readout returns semantic raw values: scalar for images, label id for labels, plus
   data-coordinate metadata.
6. The implementation works through FramePlan and DRP2 on the native runtime and has an explicit
   WGSL/WebGPU story for each supported shader variant.
