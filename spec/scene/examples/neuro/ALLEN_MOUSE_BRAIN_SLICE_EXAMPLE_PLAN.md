# Allen Mouse Brain Slice Example Plan

> **Agent Pickup**
> - **Category:** `neuro`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.


## Summary

Build a native GLFW C example that loads the bundled Allen Mouse Brain RGBA volume and displays a
movable antero-posterior slice through the active v0.4 scene -> DRP2 -> vklite/app path. The asset
is `data/volumes/allen_mouse_brain_rgba.npy.gz`, interpreted as the documented C-order RGBA volume;
no new public download is required for the first slice. The practical starting point is explicit
retained volume slice state, a shader path driven by slice axis and normalized position, an AP GUI
slider, and a bounded-frame command-line mode. Validate with the listed smoke command and manual
checks that the slice appears, moves live, and does not require rebuilding the scene.

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the v0.4 implementation path for an interactive Allen Mouse Brain
>   antero-posterior slicing example backed by `data/volumes/allen_mouse_brain_rgba.npy.gz`.


## Target Outcome

Add a native GLFW C example that loads the Allen Mouse Brain RGBA volume and displays one movable
slicing plane through the active v0.4 scene -> DRP2 -> vklite/app path.

Expected user experience:

1. open a window with an arcball-controlled 3D volume view,
2. render a single plane slice through the RGBA brain volume,
3. expose a GUI slider for the antero-posterior coordinate,
4. update the retained volume slice position live without rebuilding the scene,
5. keep a bounded-frame command-line mode for smoke validation.

Likely example name:

```text
examples/c/allen_mouse_brain_slice_glfw.c
```

Register it in `examples/c/CMakeLists.txt` under the `DVZ_HAS_GLFW` and `datoviz_gui` block, next to
`hello_volume_glfw`.


## Data Facts

The source data is:

```text
data/volumes/allen_mouse_brain_rgba.npy.gz
```

The NPY payload currently has:

```text
descr: |u1
fortran_order: False
shape: (528, 456, 320, 4)
```

Interpret this as C-order RGBA volume storage with:

```text
depth  = 528
height = 456
width  = 320
channels = 4
```

This matches the v0.3 constants:

```text
MOUSE_W = 320
MOUSE_H = 456
MOUSE_D = 528
```

For the first example slice, treat the 528-sample depth axis as the antero-posterior axis. If later
anatomical metadata gives a more explicit axis convention, adjust the helper defaults and document
the mapping in the example.


## v0.3 Reference

The old helper lives in:

```text
v0.3/tests/scene/scene_testing_utils.h
```

`load_brain_volume()` did only four important things:

1. build the `DATA_DIR/volumes/allen_mouse_brain_rgba.npy.gz` path,
2. decompress with `dvz_read_gz()`,
3. strip the NPY header with `dvz_parse_npy()`,
4. upload as a `DVZ_FORMAT_R8G8B8A8_UNORM` 3D texture with dimensions `320 x 456 x 528`.

Do not port the v0.3 `DvzTexture*` binding model. In v0.4 the example should create a scene-owned
`DvzSampledField` and bind it to `dvz_volume()` through the `"field"` slot.


## Public API Addition

Add a clearer public helper instead of making example code manipulate a thin clipping box directly.

Proposed first-slice API:

```c
typedef enum
{
    DVZ_VOLUME_AXIS_X = 0,
    DVZ_VOLUME_AXIS_Y,
    DVZ_VOLUME_AXIS_Z,
} DvzVolumeAxis;

DVZ_EXPORT int dvz_volume_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis);

DVZ_EXPORT int dvz_volume_set_slice_position(DvzVisual* visual, double position);
```

Semantics:

1. `position` is normalized in `[0, 1]`.
2. `DVZ_VOLUME_AXIS_Z` is the default, matching the current shader's `uvw.z` slice behavior.
3. The helper is meaningful when `render_mode == DVZ_VOLUME_RENDER_SLICE`.
4. For the first implementation, the retained state may lower to a thin slab internally, but that
   should remain an implementation detail.
5. Invalid axis, non-finite position, or out-of-range position returns `-1` and leaves state
   unchanged.

Implementation storage should extend `DvzVolumeState` with:

```c
DvzVolumeAxis slice_axis;
double slice_position;
```

Default state:

```text
slice_axis = DVZ_VOLUME_AXIS_Z
slice_position = 0.5
```

Update the volume uniform payload so the shader receives the axis and position explicitly instead
of inferring the slice plane from `clip_min` / `clip_max`.


## Shader Path

Current `src/scene/glsl/volume_slice.frag` slices at:

```glsl
float slice_z = 0.5 * (box_min.z + box_max.z);
```

Change the slice shader to derive the plane from explicit retained state:

1. read `slice_axis` and `slice_position` from the volume uniform,
2. intersect the camera ray with the selected constant-UVW plane,
3. reject samples outside the clipping box,
4. keep the existing depth-occlusion behavior,
5. keep MIP and composite clipping behavior unchanged.

The example only needs an antero-posterior slider, but the public helper should support all three
axes so the API is not anatomically hardcoded.


## RGBA Volume Handling

The Allen volume is already RGBA. The current volume metadata path must not require a colormap scale
to render RGBA data as color.

Adjust volume lowering so RGBA fields are treated as RGBA when:

```text
field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM
```

Scalar fields should keep the existing behavior:

1. without a colormap scale, sample scalar data as grayscale,
2. with a colormap scale, stage the scalar field into an RGBA texture for transfer-function display.

Add a focused scene test proving an RGBA 3D sampled field bound to `dvz_volume()` lowers with RGBA
sampling enabled without attaching a colormap scale.


## Example Loading Path

Implement a local example helper rather than a new public data loader:

1. read `data/volumes/allen_mouse_brain_rgba.npy.gz` with `dvz_read_gz()`,
2. validate the NPY magic/header enough to confirm dtype, shape, and C-order expectation,
3. parse payload bytes with `dvz_parse_npy()` or a slightly stricter local parser,
4. create a `DvzSampledField`:

```c
DvzSampledFieldDesc desc = {
    .dim = DVZ_FIELD_DIM_3D,
    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
    .width = 320,
    .height = 456,
    .depth = 528,
};
```

5. upload with:

```c
DvzFieldDataView view = {
    .data = rgba,
    .bytes_per_row = 320 * 4,
    .rows_per_image = 456,
};
```

6. bind the field with `dvz_visual_set_field(volume, "field", field)`,
7. set `DVZ_VOLUME_RENDER_SLICE`,
8. set `DVZ_VOLUME_SAMPLING_LINEAR`,
9. drive `dvz_volume_set_slice_position(volume, state->ap_position)` from the GUI slider.


## Example Controls

Keep the GUI narrow and explicit:

1. `AP slice` slider in `[0, 1]`,
2. optional `Opacity` slider in `[0, 1]`,
3. optional `Sampling` toggle between linear and nearest,
4. optional `Reset` button restoring AP position to `0.5`.

Avoid adding transfer-function controls in this example because the source volume is already RGBA.
Use `hello_volume_glfw` for scalar transfer-function controls.


## Validation

Run the focused checks:

```bash
just build
./build/testing/dvztest_scene volume
./build/examples/c/allen_mouse_brain_slice_glfw 2
git diff --check
```

If the environment cannot open a GLFW/Vulkan window, report that explicitly and keep the bounded
example smoke as the validation gap.


## Follow-Ups

Potential later slices:

1. add a true oblique slicing-plane helper with normal plus offset,
2. expose field-axis geometry metadata so anatomical axes are first-class,
3. add an image-style 2D viewport companion showing the selected slice without 3D perspective,
4. add keyboard shortcuts for stepping through AP slices,
5. support downloaded or externally supplied data paths through an example CLI option.
