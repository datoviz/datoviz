# Diffusion MRI Tractography Example

## Goal

Create a polished Datoviz v0.4 Python example showing **3D diffusion MRI tractography streamlines** as colored paths, tubes, or ribbons.

The example should stress-test the new Datoviz scene API by requiring a first-class representation of **large collections of 3D polylines with variable length**, per-vertex attributes, per-streamline metadata, interactive 3D navigation, picking, transparency, depth handling, and optional LOD.

This should not be a toy scatter/line example. It should look like a real neuroimaging tractography viewer.

Working title:

```text
Diffusion MRI Tractography Viewer
```

Possible filename:

```text
DIFFUSION_TRACTOGRAPHY.md
```

---

## Scientific / visual motivation

Diffusion MRI tractography reconstructs 3D white-matter pathways from diffusion MRI data as sets of streamlines. A tractography dataset typically contains thousands to millions of variable-length 3D curves. Each streamline is a polyline in anatomical coordinates, often stored in `.trk`, `.tck`, `.trx`, `.vtk`, or `.vtp` formats.

A good Datoviz example should show:

- many 3D colored paths;
- smooth arcball interaction;
- realistic neuroimaging data;
- color by local direction, bundle ID, scalar value, or depth;
- multiple rendering modes;
- optional anatomical context;
- good transparency and depth behavior;
- fast rendering without rebuilding geometry every frame.

This is a natural candidate for a built-in Datoviz visual, tentatively named:

```text
Path3DVisual
StreamlineVisual
TubeVisual
TractographyVisual
```

---

## Recommended dataset strategy

Use a small, curated tractography dataset committed or mirrored into `datoviz/data`, rather than downloading a huge raw diffusion MRI dataset at runtime.

Recommended sources:

1. **DIPY fornix example dataset**

   DIPY includes a small tractography example centered on the fornix, explicitly used in its QuickBundles tutorial. It is ideal for a lightweight out-of-the-box example.

2. **Zenodo “Bundles for tractography file format testing and example”**

   This dataset is designed for tractography file-format testing/examples and includes bundle files plus reference anatomy. It is a good candidate for a stable downloadable data source or for mirroring into `datoviz/data`.

3. **TractoInferno**

   TractoInferno is a large open-source multi-site tractography database. It is scientifically strong, but too large for the default example. It can be mentioned as an optional advanced data source, not used by default.

Recommended default:

```text
Use a preprocessed compact `.npz` or `.bin` file derived from DIPY’s fornix or a small bundle dataset.
```

Do **not** make the example depend on full DIPY, nibabel, or raw MRI processing at runtime. The example should download a prepared Datoviz-ready file from `datoviz/data` if it is not already cached.

---

## Data preprocessing pipeline

The data preparation can be implemented as an offline script, not part of the runtime example.

Input formats:

```text
.trk
.tck
.trx
.vtk
.vtp
```

Useful Python tools for offline preprocessing:

```text
nibabel
dipy
numpy
```

The preprocessing script should:

1. Load tractography streamlines from `.trk`, `.tck`, or another common format.
2. Optionally select a subset of streamlines.
3. Resample streamlines to a fixed or bounded point spacing.
4. Normalize or center coordinates.
5. Compute per-vertex tangent directions.
6. Compute per-vertex colors.
7. Compute per-streamline metadata.
8. Save a compact Datoviz-ready file.

Suggested output file:

```text
data/tractography/fornix_streamlines.npz
```

Suggested `.npz` layout:

```text
positions: float32[N, 3]
tangents:  float32[N, 3]
colors:    uint8[N, 4] or float32[N, 4]
offsets:   uint32[M + 1]
bundle_id: uint16[M]
length:    float32[M]
```

Where:

```text
N = total number of vertices over all streamlines
M = number of streamlines
offsets[i]:offsets[i+1] gives the vertex range of streamline i
```

Optional extra arrays:

```text
scalar:        float32[N]      # FA, curvature, speed, depth, etc.
radius:        float32[N]      # per-vertex tube radius
picking_id:    uint32[M]       # one picking ID per streamline
lod_level:     uint8[M]        # precomputed LOD bucket
bbox_min:      float32[M, 3]
bbox_max:      float32[M, 3]
```

---

## Runtime behavior

The Python example should:

1. Locate the dataset in the local Datoviz cache.
2. Download it from `datoviz/data` if missing.
3. Load the `.npz`.
4. Create a 3D scene.
5. Add a 3D panel with arcball/orbit camera.
6. Add a streamline/path/tube visual.
7. Expose rendering controls through ImGui.
8. Allow interactive switching between rendering modes.
9. Keep rendering interactive for at least tens or hundreds of thousands of vertices.

The API is not yet finalized, so the example specification should avoid depending on exact v0.4 function names. Conceptually it should resemble:

```python
scene = dvz.Scene()
panel = scene.panel_3d()
camera = panel.arcball()

data = load_or_download_tractography_dataset()

tracts = scene.visual("streamlines")
tracts.set_positions(data.positions)
tracts.set_offsets(data.offsets)
tracts.set_colors(data.colors)
tracts.set_radius(0.4)
tracts.set_mode("tube")

panel.add(tracts)
scene.run()
```

This is intentionally schematic.

---

## Rendering modes

The example should expose at least four rendering modes.

### 1. Thin polyline mode

Fastest mode.

Implementation:

```text
Primitive: line strip, multi-draw, or indexed line list
Geometry: original streamline vertices
Color: per-vertex direction color
```

Pros:

- simplest;
- very fast;
- good for large tractograms;
- useful as a baseline visual.

Cons:

- line width support is backend-dependent;
- thin lines do not look as good in 3D;
- poor depth perception.

This mode is essential as the fallback implementation.

---

### 2. Screen-space tube impostor mode

Preferred default if feasible.

Implementation idea:

```text
Each segment is rendered as a camera-facing capsule or quad strip.
The fragment shader computes apparent tube shape and lighting.
```

Input per segment:

```text
p0: vec3
p1: vec3
color0: vec4
color1: vec4
radius: float
streamline_id: uint
```

Pros:

- much cheaper than tessellated mesh tubes;
- visually better than raw lines;
- good for many streamlines;
- supports smooth apparent thickness;
- suitable for built-in scene API visual.

Cons:

- caps and joins require care;
- silhouette artifacts possible;
- picking may need special handling.

This should be considered the main target for a built-in `Path3DVisual` or `TubePathVisual`.

---

### 3. Tessellated tube mesh mode

High-quality mode.

Implementation:

```text
Generate a cylindrical tube around each streamline segment.
Use 6–12 sides per tube.
Generate smooth joins where possible.
```

Pros:

- true 3D geometry;
- correct depth;
- compatible with lighting, SSAO, shadows;
- good for screenshots and demos.

Cons:

- many vertices;
- preprocessing cost;
- join handling is nontrivial;
- memory-heavy for large tractograms.

This mode is useful as a quality reference and for testing mesh throughput.

---

### 4. Ribbon mode

Alternative mode for dense bundles.

Implementation:

```text
Generate view-aligned or tangent-frame ribbons along each streamline.
```

Pros:

- cheaper than tubes;
- more visible than lines;
- useful for showing bundles as flowing surfaces.

Cons:

- orientation ambiguities;
- can look artificial;
- requires careful depth/transparency.

---

## Color mapping

The default coloring should use local tangent direction:

```text
R = abs(tangent.x)
G = abs(tangent.y)
B = abs(tangent.z)
A = alpha
```

This is standard in tractography visualization and immediately communicates fiber orientation.

Additional color modes exposed in the UI:

```text
Direction RGB
Bundle ID
Streamline length
Depth
Curvature
Single color with alpha
```

For bundle ID mode, use a categorical palette.

For scalar mode, use a perceptually reasonable continuous colormap.

---

## ImGui controls

The example should include a compact ImGui panel.

Controls:

```text
Dataset:
  - Fornix
  - Small multi-bundle example
  - Optional synthetic stress test

Rendering mode:
  - Lines
  - Tube impostors
  - Tessellated tubes
  - Ribbons

Color mode:
  - Direction RGB
  - Bundle ID
  - Length
  - Curvature
  - Solid color

Geometry:
  - Radius
  - Tube sides
  - Streamline subsampling
  - Maximum streamlines
  - Minimum length

Appearance:
  - Alpha
  - Depth test on/off
  - Weighted OIT on/off
  - Lighting on/off
  - SSAO on/off if available

Interaction:
  - Highlight picked streamline
  - Show streamline ID / length
  - Reset camera
```

---

## Scene API pressure points

This example is valuable because it forces several important scene API decisions.

The v0.4 scene layer is intended to manage visuals, resources, cameras, controllers, framegraphs, shader compilation, LOD, and DRP command generation, while remaining backend agnostic.

This example specifically requires:

### Variable-length geometry

Streamlines are not fixed-size primitives. The visual needs:

```text
positions[N, 3]
offsets[M + 1]
per-streamline metadata[M]
```

This is different from ordinary scatter or mesh visuals.

The scene API should support either:

```text
visual.set_offsets(offsets)
```

or a generic indexed/ragged geometry resource model.

---

### Attribute / constant channels

The scene specification already distinguishes visual parameters as either constants or attributes. This maps well to tractography rendering:

```text
position: attribute
color: attribute or constant
radius: attribute or constant
alpha: constant
bundle_id: attribute
```

This example should validate that the channel model works for path-like visuals, not just scatter or mesh visuals.

---

### Multiple render stages

The visual may need several stages:

```text
Opaque pass
Transparent pass
Picking pass
Overlay/highlight pass
```

Transparent fibers should ideally use weighted blended OIT, already anticipated in the scene framegraph design.

---

### Picking

Picking should be per-streamline, not per-vertex.

Minimum behavior:

```text
Hover or click a streamline.
Highlight the full streamline.
Display streamline index, length, bundle ID.
```

Implementation options:

1. CPU ray-to-polyline distance picking.
2. GPU ID buffer picking.
3. Hybrid: coarse CPU BVH, exact GPU/CPU refinement.

The scene API already anticipates CPU and GPU picking workflows. This example should make them concrete for complex 3D paths.

---

### LOD

A full tractogram can be huge. The example should include simple LOD hooks:

```text
- random streamline subsampling;
- length-based filtering;
- screen-space density reduction;
- distance-based thinning;
- optional precomputed LOD levels.
```

LOD should be scene-side. DRP should only receive the selected buffers/subranges.

---

### Framegraph integration

Rendering modes may require different passes:

```text
MAIN_COLOR
PICKING
OIT_ACCUM
OIT_RESOLVE
SSAO_GBUFFER
SSAO_RESOLVE
```

This makes the example useful for validating that a visual can request framegraph features rather than manually controlling backend details.

---

## Proposed built-in visual: `Path3DVisual`

This example strongly argues for a built-in path visual in the scene API.

Minimal `Path3DVisual` data model:

```text
positions:  float32[N, 3]
offsets:    uint32[M + 1]
colors:     optional float32/uint8[N, 4] or constant
radius:     optional float32[N] or constant
ids:        optional uint32[M]
```

Supported modes:

```text
line
screen_tube
mesh_tube
ribbon
```

Core properties:

```text
radius
alpha
color_mode
join_style
cap_style
depth_test
transparent
picking
lod_policy
```

Possible join styles:

```text
none
miter
round
bevel
```

Possible cap styles:

```text
none
flat
round
```

This visual would be useful beyond tractography:

```text
trajectories
particle paths
streamlines
field lines
molecular bonds
GPS traces
neurite skeletons
vascular trees
```

---

## Expected visual result

The default view should show a compact white-matter bundle, preferably the fornix or a multi-bundle tractogram, centered in a black or dark background.

Default rendering:

```text
Mode: tube impostor if implemented, otherwise line mode
Color: direction RGB
Camera: 3D arcball
Background: dark gray / black
Alpha: 0.7–1.0
Depth test: enabled
```

The visual should look like a real neuroimaging tractography viewer, not a synthetic curve demo.

---

## Performance targets

Minimum target:

```text
10k–100k vertices interactive
```

Good target:

```text
500k–2M vertices interactive in line or impostor mode
```

Stress-test target:

```text
5M–20M vertices with LOD/subsampling
```

The example should print basic dataset information:

```text
Number of streamlines
Number of vertices
Mean streamline length
Rendering mode
Visible streamline count
```

---

## Runtime dependency policy

The runtime example should require only:

```text
numpy
datoviz
```

Optional dependencies may be used only in offline preprocessing:

```text
dipy
nibabel
trx-python
```

The example itself should not require DIPY installation.

---

## Download/cache behavior

The example should use a helper equivalent to:

```python
path = dvz.download_data(
    "tractography/fornix_streamlines.npz",
    url="https://github.com/datoviz/data/..."
)
```

Expected behavior:

1. Check local cache.
2. Download if missing.
3. Verify file size or checksum.
4. Load with NumPy.
5. Fail gracefully with a clear message if unavailable.

---

## Optional anatomical context

A later version may add a translucent anatomical reference:

```text
- low-resolution brain mesh;
- MNI glass brain;
- transparent bounding box;
- slice planes from a T1 image.
```

This should be optional. The first implementation should focus on streamlines.

---

## Optional advanced features

Good stretch goals:

```text
- per-streamline picking and highlighting;
- bundle visibility toggles;
- clipping plane;
- animated sweep along streamlines;
- curvature-based coloring;
- tube lighting;
- SSAO;
- weighted blended OIT;
- mini orientation axes overlay;
- screenshot export.
```

SSAO is particularly interesting for tessellated tube mode, because it improves depth perception in dense bundles and pressures the scene framegraph to support geometry/depth-driven post-processing passes.

---

## Implementation notes for the agent

The implementation agent should not assume a finalized Datoviz v0.4 Python API.

Instead, it should map this specification onto the actual available API.

Required conceptual operations:

```text
Create scene
Create 3D panel
Create arcball/orbit camera
Create path/tube visual
Upload ragged streamline data
Assign per-vertex colors
Assign offsets
Render interactively
Expose ImGui controls
```

Avoid hardcoding fragile API names in this specification.

Where the scene API lacks a built-in path/tube visual, the first implementation may use one of these fallbacks:

```text
1. line-list visual generated from streamline segments;
2. mesh visual generated from tessellated tubes;
3. custom shader visual for screen-space tube impostors.
```

The preferred long-term outcome is to turn the fallback into a reusable built-in visual.

---

## Acceptance criteria

The example is successful if:

```text
- It runs out of the box.
- It downloads/caches its data automatically.
- It displays real tractography data.
- It supports 3D arcball interaction.
- It provides at least line rendering and one enhanced rendering mode.
- It colors streamlines by local direction.
- It exposes useful ImGui controls.
- It keeps interactive performance on a normal GPU.
- It demonstrates why Datoviz needs a built-in path/tube visual.
```

Minimum acceptable first version:

```text
DIPY fornix dataset
Direction-colored 3D line rendering
Arcball camera
ImGui controls for alpha, radius/line width, subsampling, color mode
```

Ideal version:

```text
Small multi-bundle tractography dataset
Screen-space tube impostors
Per-streamline picking
Weighted OIT
Optional SSAO
LOD/subsampling
Highlight selected streamline
```

---

## Recommended final example name

```text
DIFFUSION_TRACTOGRAPHY.md
```

Alternative names:

```text
TRACTOGRAPHY_VIEWER.md
STREAMLINE_TUBES.md
DMRI_TRACTOGRAPHY.md
```
