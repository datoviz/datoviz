# Scene Volume Rendering Implementation Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the v0.4 implementation path for the napari-style 3D volume clipping demo
>   without porting the v0.3 volume visual directly.


## Context

The target example is `spec/scene/examples/napari/VOLUME_CLIPPING_3D.md`. It should demonstrate a
small but real 3D microscopy volume, live 3D camera navigation, MIP or translucent volume rendering,
an arbitrary clipping/slice plane, and optional overlays.

This should be implemented through the active v0.4 path:

```text
SampledField -> scene frame plan -> DRP2 command stream -> vklite/canvas runtime
```

Do not create a parallel Vulkan renderer, presentation loop, or volume-private data model.


## Existing v0.3 Reference

The v0.3 tree contains useful reference code:

- `v0.3/src/scene/visuals/volume.c`
- `v0.3/include/datoviz/scene/visuals/volume.h`
- `v0.3/src/scene/glsl/graphics_volume.vert`
- `v0.3/src/scene/glsl/graphics_volume.frag`
- `v0.3/include/datoviz/scene/glsl/utils_volume.glsl`
- `v0.3/src/scene/visuals/slice.c`
- `v0.3/src/scene/glsl/graphics_slice.frag`
- `v0.3/src/scene/glsl/graphics_volume_slice.frag`
- `v0.3/examples/visuals/volume.py`
- `v0.3/tests/scene/visuals/test_volume.c`

Useful ideas to keep:

1. render a cube proxy around the volume bounds,
2. compute ray entry/exit against the volume box in the fragment shader,
3. sample a `sampler3D`,
4. expose volume bounds separately from texture coordinates,
5. support axis permutation / field-axis mapping,
6. keep a separate slice visual that samples the same 3D data,
7. use arcball + camera in the example,
8. include a small synthetic or downloaded-volume smoke test.

Parts not to port as-is:

1. fixed `STEP_SIZE = 0.005`,
2. hardcoded `MAX_ITER`,
3. single alpha-scale transfer parameter,
4. face-index based pseudo-slicing,
5. incorrect or ambiguous `gl_FragDepth` assignment,
6. v0.3 batch/resource ownership,
7. direct `DvzTexture*` public binding,
8. shader specialization constants as the main public mode API,
9. scalar/RGBA-only format assumptions baked into the visual.


## Target First-Class v0.4 Objects

The first implementation should introduce or extend these concepts:

1. `DvzSampledField` remains the authoritative 3D data object.
2. `VolumeVisual` binds a `DVZ_FIELD_DIM_3D` sampled field through the `"field"` slot.
3. `PlaneSliceVisual` or a first narrow slice visual samples the same 3D field on an arbitrary plane.
4. `DvzScale` / colormap state provides scalar-to-color mapping where practical.
5. Volume-specific retained state stores render mode, intensity window, opacity, step policy, and
   clipping-plane parameters.
6. DRP2 owns backend-agnostic texture creation, upload, bind groups, and render commands.
7. vklite executes the emitted DRP2 stream with 3D sampled textures and ordinary panel render passes.


## Rendering Modes

Implement modes in this order:

1. `MIP`: maximum intensity projection. This is the best first correctness target because it avoids
   opacity integration details and is visually useful for microscopy.
2. `ALPHA`: front-to-back alpha compositing with early ray termination.
3. `PLANE`: sample along a finite-thickness ray perpendicular to a plane, or render an explicit plane
   quad through the volume.
4. `CLIPPED_ALPHA` or `CLIPPED_MIP`: same raymarcher as above, but with one active clipping plane.

Defer these until the base path is stable:

1. isosurface mode,
2. gradient lighting,
3. pre-integrated transfer functions,
4. multi-volume intermixing,
5. out-of-core bricking,
6. empty-space skipping acceleration structures.


## Categorical Label Volumes

napari `Labels` layers can also be displayed in 3D, but that should be a follow-up after scalar
volume rendering is stable. The image/labels 2D plan deliberately keeps these requirements out of
the 2D image visual path.

Required semantics:

1. integer 3D `DvzSampledField` with `DVZ_FIELD_SEMANTIC_LABEL`,
2. nearest sampling or integer texel fetch only,
3. categorical palette lookup using the same direct-palette/hash-color policy as 2D labels,
4. background label `0` transparent by default,
5. probe/readback returning the raw label id,
6. selected-label-only display where practical,
7. no interpolation between neighboring label ids.

Render modes to consider after scalar MIP/alpha/plane/clipping are stable:

1. translucent categorical volume rendering,
2. categorical plane slice using the same field and palette,
3. categorical isosurface rendering,
4. optional fast/smooth gradient modes for label-surface lighting.

Implementation note: label volumes should reuse the sampled-field, palette, and blend-mode
infrastructure from `SCENE_NAPARI_IMAGE_LABELS_PLAN.md`, but they should not broaden the 2D image
shader into a volume renderer. Keep the 3D raymarch/proxy-cube and clipping behavior in the volume
visual path.


## Step-By-Step Implementation

### 1. DRP2 3D Texture Format Support

Current DRP2 has `dvz_drp2_stream_create_texture_3d()`, but the helper hardcodes
`VK_FORMAT_R8G8B8A8_UNORM`. Add an explicit-format variant and validate it through the semantic and
vklite runtime layers.

Expected work:

1. add `dvz_drp2_stream_create_texture_3d_format_usage()`,
2. map `DvzFieldFormat` scalar cases to supported runtime texture formats,
3. ensure `WriteTexture` with `depth > 1` validates byte layout and overflow checks,
4. add DRP2 tests for 3D `R8_UNORM`, `R16_UNORM`, and `R32_FLOAT` creation/upload where supported,
5. keep portable recording/replay support for the new command shape.

Validation:

```text
just build
./build/testing/dvztest_drp2 drp2
git diff --check
```


### 2. Scene 3D Field Runtime Realization

Extend the scene field lowering path so a 3D sampled field can become one retained runtime texture.
Do this generically enough for both volume and plane-slice consumers.

Expected work:

1. extend field format helpers to report runtime texture format and texel byte size for 3D scalar
   formats,
2. emit create/upload commands for full 3D fields,
3. emit subregion uploads for 3D dirty boxes,
4. track dirty state once per field even if several visuals bind it,
5. add scene tests proving one 3D field can be uploaded and reused by two visual consumers.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_sampled_field
./build/testing/dvztest_drp2 drp2
git diff --check
```


### 3. Public Volume Visual Skeleton

Make `DVZ_VISUAL_TYPE_VOLUME` real in v0.4 rather than only an internal enum value.

Expected API shape:

```c
DvzVisual* dvz_volume(DvzScene* scene, uint32_t flags);
bool dvz_volume_set_mode(DvzVisual* visual, DvzVolumeMode mode);
bool dvz_volume_set_window(DvzVisual* visual, double min, double max);
bool dvz_volume_set_opacity(DvzVisual* visual, float opacity);
bool dvz_volume_set_step_count(DvzVisual* visual, uint32_t step_count);
bool dvz_volume_set_bounds(DvzVisual* visual, const double bounds[6]);
```

The first slice may keep some setters internal if the public API needs more design time, but the
retained state should exist as typed scene state rather than ad-hoc shader constants.

Expected work:

1. add public enum for volume mode,
2. add `dvz_volume()` in the scene public header and implementation,
3. allow `dvz_visual_set_field(volume, "field", field)` only for 3D fields,
4. generate cube proxy vertices from semantic bounds,
5. add JSON/debug output for retained volume state,
6. add tests for binding rejection of 2D fields and cross-scene fields.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_volume
git diff --check
```


### 4. First MIP Raymarch Shader

Add the first executable volume shader pair. Use v0.3 only as a conceptual reference, not as a
direct port.

Shader requirements:

1. compute object-space or volume-space ray origin/direction correctly from panel MVP/camera state,
2. intersect the ray with the volume bounds,
3. derive step size from volume dimensions and requested quality,
4. sample the 3D scalar texture,
5. apply intensity window,
6. accumulate maximum scalar value,
7. map the final scalar through a simple grayscale or bound scale,
8. discard rays that miss the volume.

First-slice simplifications:

1. no gradient lighting,
2. no empty-space skipping,
3. no depth write,
4. one scalar field binding,
5. one sampler,
6. one uniform block for volume parameters.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_volume_emits_mip_drp2
./build/examples/c/<first-volume-example> --frames 2
git diff --check
```


### 5. Front-To-Back Alpha Compositing

Add a second mode after MIP is correct.

Shader requirements:

1. use front-to-back compositing:
   `acc.rgb += (1 - acc.a) * sample.a * sample.rgb`,
   `acc.a += (1 - acc.a) * sample.a`,
2. apply opacity correction when step size changes,
3. terminate early when accumulated alpha reaches a high threshold,
4. support ordinary grayscale transfer first,
5. add a path for a 1D RGBA transfer texture later.

The v0.3 shader's `alphaAcc += alpha` plus post-normalization should not be reused.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_volume_alpha_mode
./build/examples/c/<first-volume-example> --frames 2
git diff --check
```


### 6. Plane Slice Visual

Implement the napari-style plane as a separate consumer of the same 3D field unless the public API
design strongly pushes it into the volume visual.

Required state:

```text
plane_position: vec3, in field/world coordinates
plane_normal: vec3
plane_thickness: float
plane_extent or auto-fit-to-volume flag
```

Expected work:

1. compute the plane/box intersection polygon on CPU or generate a conservative quad and clip in
   shader,
2. sample the 3D field at interpolated field coordinates,
3. reuse scalar window/scale logic from volume where possible,
4. update the plane live without re-uploading the field,
5. add a bounds/outline visual for orientation.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_volume_plane_slice
./build/examples/c/<first-volume-example> --frames 2
git diff --check
```


### 7. Clipping Plane Mode

Add one retained clipping plane to the volume visual.

Required state:

```text
enabled: bool
position: vec3
normal: vec3
thickness: float
mode: keep_positive / keep_negative / highlight_slice
```

Implementation rule:

1. clipping operates in the same semantic/field coordinate frame as the volume bounds,
2. shader converts the ray sample position into that frame,
3. samples outside the active half-space or thickness band are skipped,
4. the plane visual and clipped volume share the same plane parameters.

Validation:

```text
just build
./build/testing/dvztest_scene test_scene_volume_clipping_plane
./build/examples/c/<first-volume-example> --frames 2
git diff --check
```


### 8. Napari Demo Example

Create a focused example for `cells3d`.

Expected behavior:

1. load or consume a cached `cells3d` volume,
2. normalize to `R8_UNORM`, `R16_UNORM`, or `R32_FLOAT`,
3. create a 3D `DvzSampledField`,
4. bind it to a volume visual,
5. bind the same field to a plane-slice visual,
6. add arcball camera,
7. add optional points or bounding-box overlay,
8. add GUI controls for mode, window, step count, plane position, plane normal, and thickness.

The demo should say explicitly that this is a controlled GPU rendering stress test, not full napari
3D parity.

Validation:

```text
just build
./build/examples/c/<napari-volume-example> --frames 2
```

For a live GLFW smoke on graphics-capable machines:

```text
direnv exec . ./build/examples/c/<napari-volume-example> 300
```


## State-Of-The-Art Direction

The baseline should be GPU volume raycasting over a 3D texture. That is the practical common core
used by current visualization stacks such as VTK GPU volume rendering and VisPy/napari-style volume
visuals.

Initial quality/performance features should be conservative:

1. MIP and front-to-back compositing,
2. correct sample-distance policy,
3. intensity window and opacity controls,
4. early ray termination,
5. jittered ray starts if banding is visible.

Only after the simple path works should Datoviz add heavier acceleration:

1. empty-space skipping,
2. bricking,
3. multiresolution / out-of-core streaming,
4. pre-integrated transfer functions,
5. gradient lighting,
6. GPU-side derived min/max occupancy textures.

For `cells3d` and other small demo volumes, a single in-core 3D texture is the right first target.
For OME-Zarr / IDR-size data, the architecture should eventually move toward bricked fields and
streamed working sets, but that is not required for the first napari clipping demo.


## Acceptance Criteria

The first complete slice is done when:

1. a real 3D scalar sampled field uploads through scene -> DRP2 -> vklite,
2. MIP volume rendering works in an app/offscreen or GLFW example,
3. plane position and normal can update live without re-uploading the field,
4. clipping plane mode works on the same volume,
5. at least one overlay, preferably bounds or points, renders coherently with the volume,
6. focused scene and DRP2 tests cover 3D texture creation, upload, field binding, and emission,
7. `git diff --check` passes.
