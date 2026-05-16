# TOY_DICOM_VIEWER.md

# Datoviz v0.4 example specification: Toy DICOM Viewer

## Goal

Create a Python example for the Datoviz v0.4 branch demonstrating an interactive medical volume viewer with:

- three synchronized 2D orthogonal slice views;
- one 3D volume rendering view;
- shared interaction state across all views;
- GPU-side texture sampling from a single 3D volume;
- simple but useful medical-image controls such as slice position, window/level, opacity, and sampling step.

The example is intentionally a **toy DICOM viewer**, not a clinical DICOM application. It should feel realistic and useful for research/demo purposes, but it should not attempt to implement the full DICOM standard, PACS integration, diagnostic tools, or regulatory-grade behavior.

The primary purpose is to test Datoviz v0.4 scene capabilities: multi-panel layouts, 2D and 3D cameras, texture resources, custom shader-based visuals, cross-panel state synchronization, UI controls, and efficient GPU volume rendering.

---

## Expected filename

Suggested example filename:

```text
examples/showcase/toy_dicom_viewer.py
```

Suggested documentation/spec filename:

```text
TOY_DICOM_VIEWER.md
```

---

## Scope

### In scope

The example should implement:

1. Loading a small cached 3D medical volume.
2. Four panels in a 2x2 layout:
   - axial slice view;
   - sagittal slice view;
   - coronal slice view;
   - 3D volume rendering view.
3. Three 2D views showing orthogonal slices through the same volume.
4. A 3D volume rendering view using a raymarching shader.
5. Crosshair overlays in the 2D views showing the current 3D cursor position.
6. Shared state:
   - current voxel coordinate `(ix, iy, iz)`;
   - window center;
   - window width;
   - volume opacity;
   - raymarch step size;
   - transfer function preset, if implemented.
7. Basic UI controls:
   - sliders for X/Y/Z slice indices;
   - sliders for window center and window width;
   - slider for global opacity;
   - slider for volume sampling step;
   - reset camera button;
   - optional checkbox for trilinear interpolation.
8. Mouse interaction:
   - clicking/dragging in any 2D slice view updates the shared crosshair and corresponding slice indices;
   - 3D panel supports arcball/orbit camera interaction.
9. Efficient rendering:
   - upload the full 3D volume once as a 3D texture;
   - update uniforms when slice indices or display parameters change;
   - avoid CPU-side slice extraction every frame.

### Out of scope

Do not implement:

- full DICOM conformance;
- PACS, DICOMweb, networking, or study/series browser;
- patient metadata display beyond an optional toy label;
- segmentation overlays;
- measurement tools;
- multiplanar oblique reconstruction;
- arbitrary DICOM orientation edge cases;
- diagnostic accuracy guarantees;
- real clinical workflow.

---

## Data requirements

The example should work out of the box.

It should download a small sample medical volume from the `datoviz/data` GitHub repository if it is not already cached locally.

Recommended cache location:

```text
~/.cache/datoviz/toy_dicom_viewer/
```

Recommended data format:

```text
medical_volume_head_ct.npz
```

The `.npz` file should contain:

```text
volume:        3D NumPy array, shape (nz, ny, nx), dtype uint16 or float32
spacing:       float32 array, shape (3,), voxel spacing in mm, order (sz, sy, sx)
window_center: float scalar, default display window center
window_width:  float scalar, default display window width
modality:      optional string, e.g. "CT" or "MRI"
name:          optional string, short dataset name
```

If no real dataset is available in `datoviz/data`, the example may generate a synthetic medical-like volume locally. This fallback should be deterministic and should resemble a simple head/phantom volume with nested ellipsoids, bone-like high intensity regions, and soft-tissue gradients.

Preferred behavior:

1. Try loading the cached `.npz` file.
2. If missing, download it from `datoviz/data`.
3. If download fails, generate a synthetic fallback volume.
4. Print a short message indicating which source was used.

---

## Recommended visual layout

The window should use a 2x2 grid:

```text
+-----------------------+-----------------------+
| Axial                 | Sagittal              |
| z = current iz        | x = current ix        |
+-----------------------+-----------------------+
| Coronal               | 3D volume             |
| y = current iy        | orbit camera          |
+-----------------------+-----------------------+
```

Each panel should have a dark background.

The three 2D views should use equal aspect ratio in physical coordinates if spacing is available.

The 3D panel should display the volume in physical aspect ratio, i.e. voxel spacing should affect the volume bounding box.

---

## Scene/API assumptions

The exact Datoviz v0.4 Python API is not fixed yet. The implementation should use whatever final API exists, but conceptually it should map to these scene concepts:

- create application/window/canvas;
- create scene;
- create a 2x2 grid of panels;
- attach 2D cameras to the axial/sagittal/coronal panels;
- attach a 3D camera to the volume panel;
- create one shared 3D texture resource containing the volume;
- create three slice image visuals sampling the shared 3D texture;
- create one custom volume visual sampling the same 3D texture;
- create overlay visuals for crosshairs and optional panel labels;
- update uniform resources when UI state changes;
- render continuously or on demand.

Do not hard-code the spec to a specific unfinished Python API. The implementation agent should adapt the following pseudocode to the actual v0.4 API.

---

## High-level pseudocode

```python
import numpy as np
import datoviz as dvz


def main():
    volume_data = load_or_download_volume()

    volume = volume_data["volume"]          # shape (nz, ny, nx)
    spacing = volume_data["spacing"]        # (sz, sy, sx)
    window_center = volume_data["window_center"]
    window_width = volume_data["window_width"]

    state = ViewerState(
        ix=volume.shape[2] // 2,
        iy=volume.shape[1] // 2,
        iz=volume.shape[0] // 2,
        window_center=window_center,
        window_width=window_width,
        opacity=0.25,
        step_size=1.0,
        interpolation=True,
    )

    app = dvz.App()
    canvas = app.canvas(width=1400, height=1000, title="Datoviz v0.4 - Toy DICOM Viewer")
    scene = canvas.scene()

    grid = scene.grid(rows=2, cols=2)

    panel_axial = grid.panel(row=0, col=0, title="Axial")
    panel_sagittal = grid.panel(row=0, col=1, title="Sagittal")
    panel_coronal = grid.panel(row=1, col=0, title="Coronal")
    panel_volume = grid.panel(row=1, col=1, title="3D volume")

    for panel in [panel_axial, panel_sagittal, panel_coronal]:
        panel.camera("panzoom")
        panel.set_aspect("equal")

    panel_volume.camera("arcball")

    tex_volume = scene.texture_3d(volume, format="r16float or r16uint", normalized=True)

    axial = panel_axial.visual("volume_slice", texture=tex_volume, orientation="axial")
    sagittal = panel_sagittal.visual("volume_slice", texture=tex_volume, orientation="sagittal")
    coronal = panel_coronal.visual("volume_slice", texture=tex_volume, orientation="coronal")

    volume_visual = panel_volume.visual("volume_raymarch", texture=tex_volume)

    crosshairs = create_crosshair_overlays(panel_axial, panel_sagittal, panel_coronal)

    gui = canvas.gui()
    setup_controls(gui, state)

    def update():
        update_uniforms(axial, sagittal, coronal, volume_visual, state, volume.shape, spacing)
        update_crosshairs(crosshairs, state, volume.shape, spacing)

    def on_mouse_drag(panel, x, y, modifiers):
        update_cursor_from_panel(panel, x, y, state, volume.shape, spacing)
        update()

    panel_axial.on_mouse_drag(on_mouse_drag)
    panel_sagittal.on_mouse_drag(on_mouse_drag)
    panel_coronal.on_mouse_drag(on_mouse_drag)

    update()
    app.run()


if __name__ == "__main__":
    main()
```

This pseudocode is illustrative only. The implementation should use the actual Datoviz v0.4 API once available.

---

## Volume coordinate conventions

Use NumPy volume shape:

```text
volume[z, y, x]
```

where:

```text
nx = volume.shape[2]
ny = volume.shape[1]
nz = volume.shape[0]
```

Voxel spacing:

```text
spacing = (sz, sy, sx)
```

Physical coordinates:

```text
X = (x - 0.5 * (nx - 1)) * sx
Y = (y - 0.5 * (ny - 1)) * sy
Z = (z - 0.5 * (nz - 1)) * sz
```

The 3D volume should be centered at the origin.

The 3D bounding box dimensions should be:

```text
width_x  = nx * sx
height_y = ny * sy
depth_z  = nz * sz
```

Texture coordinates should be normalized to `[0, 1]^3`.

---

## 2D slice views

The three 2D slice views should sample the shared 3D texture in a fragment shader.

### Axial view

Displays plane:

```text
z = iz
horizontal axis: x
vertical axis: y
```

Texture coordinate:

```text
u = x / (nx - 1)
v = y / (ny - 1)
w = iz / (nz - 1)
```

### Sagittal view

Displays plane:

```text
x = ix
horizontal axis: y
vertical axis: z
```

Texture coordinate:

```text
u = ix / (nx - 1)
v = y / (ny - 1)
w = z / (nz - 1)
```

### Coronal view

Displays plane:

```text
y = iy
horizontal axis: x
vertical axis: z
```

Texture coordinate:

```text
u = x / (nx - 1)
v = iy / (ny - 1)
w = z / (nz - 1)
```

### Rendering method

Each slice view can be rendered as a textured quad filling the panel's data extent.

Each slice visual should receive uniforms:

```text
orientation:     integer enum: axial/sagittal/coronal
slice_index:     integer or normalized float
volume_shape:    vec3
spacing:         vec3
window_center:   float
window_width:    float
interpolation:   bool or sampler choice
```

The fragment shader should:

1. Convert panel coordinates to normalized texture coordinates.
2. Sample the 3D texture.
3. Apply window/level.
4. Output grayscale RGBA.

---

## Window/level mapping

Use standard medical display mapping:

```text
lo = window_center - 0.5 * window_width
hi = window_center + 0.5 * window_width
value01 = clamp((value - lo) / (hi - lo), 0, 1)
color = vec4(value01, value01, value01, 1)
```

If the uploaded texture is normalized before upload, the same mapping must be adapted consistently. Prefer preserving physical/intensity units in CPU state while normalizing only inside the shader or upload conversion layer.

For CT-like data, default values could be:

```text
window_center = 40
window_width = 400
```

For synthetic data, choose defaults that show internal contrast well.

---

## Crosshair overlays

Each 2D panel should show two colored crosshair lines representing the current cursor position.

Suggested mapping:

- axial view:
  - vertical line at `x = ix`;
  - horizontal line at `y = iy`.
- sagittal view:
  - vertical line at `y = iy`;
  - horizontal line at `z = iz`.
- coronal view:
  - vertical line at `x = ix`;
  - horizontal line at `z = iz`.

The overlay should be drawn in panel coordinates above the slice image.

Optional colors:

```text
X axis / sagittal position: red
Y axis / coronal position: green
Z axis / axial position: blue
```

Keep opacity moderate so the image remains visible.

---

## 3D volume rendering

The 3D panel should render the same volume texture using a raymarching shader.

### Geometry

Render a cube representing the volume bounding box.

The cube should be scaled according to physical volume dimensions:

```text
(nx * sx, ny * sy, nz * sz)
```

The raymarch shader should intersect the camera ray with the volume box and march between entry and exit points.

### Sampling

At each step:

1. Convert world-space position inside the box to normalized texture coordinates.
2. Sample the 3D texture.
3. Apply window/level.
4. Map scalar intensity to color and opacity.
5. Composite front-to-back.
6. Stop early when accumulated alpha approaches 1.

Pseudo-shader logic:

```wgsl
accum_color = vec3(0.0)
accum_alpha = 0.0

for t from t_entry to t_exit step step_size:
    p_world = ray_origin + t * ray_dir
    texcoord = world_to_texcoord(p_world)
    value = textureSample(volume_tex, volume_sampler, texcoord).r

    intensity = apply_window_level(value, window_center, window_width)
    sample_color = vec3(intensity)
    sample_alpha = opacity * transfer_alpha(intensity)

    accum_color += (1.0 - accum_alpha) * sample_alpha * sample_color
    accum_alpha += (1.0 - accum_alpha) * sample_alpha

    if accum_alpha > 0.98:
        break

out_color = vec4(accum_color, accum_alpha)
```

### Transfer function

Start with a simple grayscale transfer function:

```text
color = vec3(intensity)
alpha = opacity * smoothstep(alpha_threshold_low, alpha_threshold_high, intensity)
```

Suggested initial parameters:

```text
alpha_threshold_low = 0.15
alpha_threshold_high = 0.85
opacity = 0.25
```

Optional extension: add a small 1D transfer-function texture with presets:

- grayscale soft tissue;
- bone emphasis;
- maximum intensity projection mode;
- x-ray style low-opacity mode.

### Sampling step

The raymarch step size should be exposed in the UI.

A reasonable normalized default:

```text
step_size = 1.0 voxel in physical units
```

The implementation may convert this to texture-space step size using the minimum spacing or box diagonal.

The UI should allow coarser sampling for speed and finer sampling for quality.

---

## UI controls

The GUI should be minimal and immediate-mode friendly.

Suggested controls:

```text
Toy DICOM Viewer
----------------
Dataset: <name>
Shape: nz x ny x nx
Spacing: sz, sy, sx mm

Slice X: [slider 0..nx-1]
Slice Y: [slider 0..ny-1]
Slice Z: [slider 0..nz-1]

Window center: [slider]
Window width:  [slider]

Volume opacity: [slider 0..1]
Step size:      [slider, e.g. 0.25..4.0 voxels]
Interpolation:  [checkbox]

[Reset cameras]
```

Changing any control should update only the relevant uniforms and overlays.

---

## Interaction behavior

### 2D panels

Clicking or dragging inside a 2D slice panel should update the shared cursor position.

For example:

- dragging in axial view updates `(ix, iy)` while preserving `iz`;
- dragging in sagittal view updates `(iy, iz)` while preserving `ix`;
- dragging in coronal view updates `(ix, iz)` while preserving `iy`.

The corresponding slice sliders and crosshair overlays should update immediately.

### 3D panel

The 3D panel should use an orbit/arcball camera.

Expected controls:

- left drag: rotate;
- right drag or shift-drag: pan;
- wheel: zoom;
- reset button restores default view.

Optional: render three faint orthogonal slice planes inside the 3D volume at the current `(ix, iy, iz)` position.

---

## Performance expectations

Target dataset size should be modest enough to run on common GPUs:

```text
128^3 to 256^3 voxels
```

Expected behavior:

- volume texture uploaded once at startup;
- no full volume upload during interaction;
- slice index changes should update only a small uniform buffer;
- window/level changes should update only uniforms;
- crosshair updates should update small overlay vertex buffers or uniforms;
- 3D raymarching should remain interactive at default resolution and step size.

Acceptable fallback:

- If volume rendering is too slow, allow increasing step size via UI.
- Optionally render the 3D volume at lower internal resolution if the scene system supports offscreen panels or render-to-texture.

---

## Implementation notes for Datoviz v0.4

This example should exercise the following v0.4 concepts:

### Scene and panels

- One scene.
- Four panels.
- Three 2D cameras.
- One 3D camera.
- Per-panel input routing.
- Shared viewer state.

### Resources

- One 3D texture resource for the volume.
- Uniform resource for viewer parameters.
- Optional small vertex resources for crosshair overlays.
- Optional 1D texture resource for transfer function.

### Visuals

- Slice visual:
  - textured quad;
  - custom shader sampling a 3D texture at a fixed coordinate.
- Volume visual:
  - cube proxy geometry;
  - custom raymarching fragment shader.
- Overlay visual:
  - simple 2D lines for crosshairs.

### Framegraph

Minimum pass structure:

```text
Panel axial:     MAIN_COLOR pass with image slice + overlay
Panel sagittal:  MAIN_COLOR pass with image slice + overlay
Panel coronal:   MAIN_COLOR pass with image slice + overlay
Panel 3D volume: MAIN_COLOR pass with volume raymarch visual
```

Optional pass structure:

```text
Panel 3D volume:
    OPAQUE pass for bounding box / guides
    TRANSPARENT pass for volume raymarch
    OVERLAY pass for labels or orientation gizmo
```

The 3D volume visual should be treated as transparent unless a dedicated volume pipeline handles compositing internally.

---

## Shader requirements

### Slice shader

The slice shader should support:

- three orientations;
- window/level;
- texture coordinate bounds checking;
- optional nearest/linear sampling;
- grayscale output.

Inputs:

```text
volume_texture: texture_3d
volume_sampler: sampler
params:
    orientation
    slice_index or normalized_slice
    shape
    spacing
    window_center
    window_width
```

Output:

```text
rgba grayscale color
```

### Volume raymarch shader

The volume shader should support:

- camera ray reconstruction;
- ray-box intersection;
- texture-space stepping;
- front-to-back alpha compositing;
- window/level;
- global opacity;
- early ray termination;
- physical aspect ratio.

Inputs:

```text
volume_texture: texture_3d
volume_sampler: sampler
camera uniforms:
    view matrix
    projection matrix
    inverse view-projection matrix or equivalent
volume uniforms:
    box bounds
    spacing
    shape
viewer uniforms:
    window_center
    window_width
    opacity
    step_size
```

---

## Data normalization strategy

Preferred approach:

1. Load volume as `float32` in physical units if possible.
2. Store display range separately.
3. Upload either:
   - `r16float`, if supported; or
   - normalized `r16unorm`/`r8unorm` plus metadata to recover display mapping; or
   - `r32float`, if acceptable for the backend.
4. Keep window/level logic consistent with the uploaded representation.

For simplicity, the initial implementation may normalize the loaded volume to `[0, 1]` before upload and adapt the default window/level controls to normalized units.

However, the spec should encourage preserving CT-like intensity semantics when feasible.

---

## Synthetic fallback dataset

If no real dataset is available, generate a synthetic 3D phantom.

Suggested generation:

```python
z, y, x = np.mgrid[-1:1:nz*1j, -1:1:ny*1j, -1:1:nx*1j]
r_head = (x / 0.75) ** 2 + (y / 0.95) ** 2 + (z / 0.85) ** 2
r_brain = (x / 0.60) ** 2 + (y / 0.75) ** 2 + (z / 0.65) ** 2

volume = np.full((nz, ny, nx), -1000.0, dtype=np.float32)  # air
volume[r_head < 1.0] = 40.0                                # soft tissue
volume[(r_head < 1.0) & (r_brain > 1.0)] = 700.0            # skull shell
volume[r_brain < 1.0] = 35.0                               # brain

# Add ventricles or low-density ellipsoids.
vent1 = ((x + 0.15) / 0.12) ** 2 + (y / 0.08) ** 2 + (z / 0.20) ** 2
vent2 = ((x - 0.15) / 0.12) ** 2 + (y / 0.08) ** 2 + (z / 0.20) ** 2
volume[(vent1 < 1.0) | (vent2 < 1.0)] = 5.0

# Add mild noise.
rng = np.random.default_rng(0)
volume += rng.normal(0, 5, volume.shape).astype(np.float32)
```

Default synthetic metadata:

```text
spacing = (1.2, 1.0, 1.0)
window_center = 40
window_width = 400
modality = "synthetic CT"
```

---

## Acceptance criteria

The example is successful if:

1. It runs without manual data preparation.
2. It opens one Datoviz window with four panels.
3. The three 2D panels show mutually orthogonal slices through the same volume.
4. The fourth panel shows an interactive 3D volume rendering.
5. Slice sliders update the 2D views and crosshairs immediately.
6. Clicking or dragging in 2D views updates the shared crosshair and slice indices.
7. Window/level controls affect both slice views and volume rendering.
8. The 3D view uses the same 3D texture as the slice views.
9. The full volume is not reuploaded during ordinary interaction.
10. The example remains interactive on a typical discrete GPU for a 128^3 or 256^3 volume.

---

## Optional enhancements

These are useful but not required for the first implementation.

### Orientation labels

Add labels to each 2D panel:

```text
L/R, A/P, S/I
```

For the toy example, these may be approximate and based on the assumed coordinate convention.

### 3D slice planes

Render three semi-transparent colored planes inside the 3D volume corresponding to the current axial, sagittal, and coronal slice positions.

### Maximum intensity projection

Add a rendering mode:

```text
Volume mode: Composite / MIP
```

MIP shader:

```text
max_value = max(max_value, sampled_value)
```

### Transfer function texture

Use a 1D RGBA transfer function texture instead of hard-coded grayscale alpha.

### Screenshot/export

Allow saving a screenshot of the full 2x2 viewer.

---

## Suggested minimal dependencies

Required:

```text
numpy
requests or urllib.request
Datoviz v0.4 Python bindings
```

Optional:

```text
pydicom
nibabel
```

Avoid requiring `pydicom` for the core example unless a real DICOM directory is added later. The first implementation should prefer `.npz` for reliability and portability.

---

## Why this example matters

This example is a compact but meaningful test of the Datoviz v0.4 scene architecture.

It combines:

- scientific image visualization;
- multiple synchronized views;
- GPU texture resources;
- custom shaders;
- transparency/compositing;
- 2D and 3D camera interaction;
- real-time UI-driven parameter updates.

It is more representative of real scientific visualization workflows than a static image or scatter plot, while remaining small enough to implement as a showcase example.
