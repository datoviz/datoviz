# DENSE POINTS SPATIAL OMICS

## Purpose

This demo shows a high-density `Points`-like napari workflow: millions of detected cells, transcript spots, molecular coordinates, or spatial-omics features over a tissue image. The goal is to demonstrate that a modern GPU backend can render dense biological point data with smooth pan/zoom, categorical coloring, opacity controls, and zoom-dependent level-of-detail.

This is one of the best demos for napari maintainers because the data model is simple but the rendering pressure is very high.

## Implementation stance for current v0.4

Build this demo in two stages.

Stage 1 should be a **C-rendered MVP** using the current v0.4 `scene -> DRP2 -> app` path. The
preparation tool may be Python, but the interactive demo should be a C example under
`examples/napari/` so it exercises the same retained scene, app window, GUI, panzoom, and Vulkan
runtime path as the other active v0.4 examples.

Stage 1 should not depend on the full SpatialData runtime at rendering time and should not require
new point shader semantics. It should precompute point colors and LOD subsets on the CPU, then feed
the current `dvz_point()` visual with the attributes supported today: `position`, `color`, and
`size`.

Stage 2 is the renderer/API upgrade that turns the demo into the full intended napari Points stress
test: native instanced point quads, antialiased disc/Gaussian shaders, shader-side category/value
coloring, point opacity as a lightweight style parameter, and stronger density/LOD modes.

## Napari use case being mirrored

Napari's Points layer displays an `N x D` coordinate array and is commonly used for spots, annotations, detections, centroids, landmarks, and spatial biology data. The napari docs explicitly frame points as useful for spots found automatically or manually annotated over images.

Relevant napari concepts:

- `Image` layer: tissue / microscopy background.
- `Points` layer: cell centers, transcript spots, detected objects.
- Features/properties: cell type, cluster, gene identity, quality score.
- GUI controls: size, opacity, color by feature, filtering.

References:

- Napari Points layer guide: https://napari.org/stable/howtos/layers/points.html
- SpatialData datasets: https://spatialdata.scverse.org/en/latest/tutorials/notebooks/datasets/README.html
- SpatialData framework: https://github.com/scverse/spatialdata

## Dataset

Primary lightweight dataset: **SpatialData MERFISH mouse brain**.

Reason:

- SpatialData provides open spatial-omics datasets already converted to SpatialData Zarr.
- The MERFISH mouse brain example is listed as approximately 50 MB and CC0, making it practical for a demo.
- It contains the kind of spatial point/cell annotations that map well to napari Points/Shapes workflows.

Alternative lightweight dataset: **SpatialData MIBI-TOF colorectal carcinoma**.

Reason:

- Listed around 25 MB and CC BY 4.0.
- More tissue-like and good for cell-type overlays.

Heavier alternatives:

- Visium mouse brain, below 100 MB.
- Visium HD mouse brain, below 200 MB.
- Xenium and larger Visium datasets are visually compelling but probably too large for a demo unless pre-downloaded.

## Download and cache strategy

Use SpatialData's dataset helpers if available:

```python
import spatialdata as sd
from spatialdata.datasets import merfish, mibitof

sdata = merfish()   # or mibitof()
```

Cache under:

```text
.cache/datoviz-napari-demos/spatialdata/merfish.zarr
```

If the helper API changes, use the SpatialData datasets page as the authoritative index and download the corresponding Zarr from the listed S3 bucket.

For the Datoviz demo, do not require the full SpatialData runtime at rendering time. Instead, prepare a compact cache:

```text
points_xy: float32, shape (N, 2)
points_z: optional float32, shape (N,)
category: uint32, shape (N,)
value: float32, shape (N,)  # expression / score / intensity
image: optional uint8/float32 background
metadata.json
```

The Python preparation cache may be stored as:

```text
.cache/datoviz-napari-demos/spatial_points/merfish_points.npz
```

For the Stage 1 C runtime, export a simple Datoviz-ready cache that does not require a Zarr or NPZ
reader in the C example:

```text
.cache/datoviz-napari-demos/spatial_points/merfish/
  metadata.json
  positions_f32.bin      # N x 3 float32, already normalized into scene coordinates
  category_u32.bin       # N uint32
  value_f32.bin          # N float32
  colors_category_rgba8.bin
  colors_continuous_rgba8.bin
  colors_density_rgba8.bin
  lod_real_u32.bin       # optional index list, or omitted when real count is the base set
  lod_1m_u32.bin
  lod_5m_u32.bin
  lod_10m_u32.bin
  image_rgba8.bin        # optional W x H x 4 background
```

`metadata.json` should include at least:

```json
{
  "point_count": 1000000,
  "image_width": 1024,
  "image_height": 1024,
  "x_min": 0.0,
  "x_max": 1.0,
  "y_min": 0.0,
  "y_max": 1.0,
  "category_count": 12,
  "category_names": ["unknown"]
}
```

## Preprocessing pipeline

1. Load the SpatialData object.
2. Extract a representative coordinate table:
   - cell centroids if available;
   - molecule/transcript coordinates if available;
   - region centroids from labels/shapes if only polygons are present.
3. Normalize coordinates into image/world space.
4. Extract one categorical column, such as cell type, cluster, annotation, or gene identity.
5. Extract one continuous column, such as intensity, expression score, area, confidence, or quality.
6. Save the Python preparation cache.
7. Export the Datoviz-ready binary cache for the C example.

If the real dataset extraction is too slow, generate a synthetic stress extension:

```python
# Preserve real tissue shape, but duplicate/jitter points to reach 1M-10M points.
points_big = jittered_replicates(points_xy, repeats=10, sigma=0.5)
category_big = np.tile(category, repeats)
```

This keeps the demo visually biological while reaching stress-test scale.

## Datoviz adaptation

### Stage 1: current v0.4 C demo

Stage 1 should use only features that exist in the active v0.4 path:

- `dvz_image()` for the optional tissue/microscopy background.
- `dvz_point()` for points.
- `dvz_visual_set_data()` for full point-count/color-mode swaps.
- `dvz_visual_set_data_range()` only for small interactive updates when useful.
- `dvz_panel_set_panzoom()` for interaction.
- `dvz_gui_*` controls in the GLFW app.
- ordinary alpha blending for translucent points.

Stage 1 rendering should be explicit about what is emulated:

- Categorical mode uses precomputed RGBA8 colors from `category`.
- Continuous mode uses precomputed RGBA8 colors from `value`.
- Density mode uses precomputed low-alpha RGBA8 colors and ordinary blending.
- Opacity changes may reupload the color buffer in Stage 1.
- Filtering by category may rebuild/reupload compacted position/color/size arrays, or may be
  deferred if it makes the MVP too CPU-heavy.
- LOD uses precomputed subsets or prefix counts, not shader-side culling.

Stage 1 should measure and print FPS while panning/zooming and while switching point-count/color
mode. It should report the actual rendered point count so the demo does not imply more than it is
drawing.

### Required visuals

- Background image visual, optional but strongly recommended.
- Stage 1: current point visual using `position`, `color`, and `size`.
- Stage 2: point visual with instanced circular or Gaussian markers.
- Stage 2 optional: density mode, either additive alpha or screen-space accumulation.

### GPU buffers

Stage 1 uses the current point visual attributes:

```text
position: float32x3
color: rgba8
size: float32
```

Stage 2 should extend the point visual or add a point-style resource so shader-side feature mapping
is possible:

```text
position: float32x2 or float32x3
size: float32
category: uint32
value: float32
opacity: optional float32 or uniform style parameter
```

Rendering modes:

1. **Categorical mode**
   - Stage 1: color by precomputed RGBA8 category colors.
   - Stage 2: color by `category` using a color table.
2. **Continuous mode**
   - Stage 1: color by precomputed RGBA8 continuous colors.
   - Stage 2: color by `value` using a colormap texture.
3. **Density mode**
   - Stage 1: small precomputed alpha with ordinary blending.
   - Stage 2: additive or premultiplied blending, or screen-space accumulation.
4. **LOD mode**
   - Stage 1: precomputed random subsamples or prefix counts.
   - Stage 2: precomputed subsets plus optional screen-space density culling.

Stage 2 point shader:

- Use instanced quads, not native point sprites.
- Generate antialiased discs in fragment shader using signed distance to marker center.
- Use opacity correction for dense zoomed-out views.

### Stage 2 prerequisites

These items are not required for the Stage 1 MVP but are required before claiming the full demo:

- Native GLSL point lowering to instanced quads, matching or superseding the current WGSL fixture
  shape.
- Antialiased disc/Gaussian point fragment shader.
- A point style path that can change global size/opacity without reuploading position buffers.
- Category/value attributes or equivalent style buffers accepted by point visuals.
- Color-table and colormap texture binding for point visuals.
- A well-defined density blending mode.
- LOD selection rules that are deterministic and visible in diagnostics/FPS output.

## Demo UI

Stage 1 controls:

- Dataset selector: MERFISH / MIBI-TOF / synthetic stress.
- Point count: real / 1M / 5M / 10M.
- Color mode: category / continuous / density.
- Point size slider.
- Opacity slider.
- LOD toggle.
- Background image visibility.

Stage 2 controls:

- Filter by category.
- Shader-side categorical palette selector.
- Continuous colormap selector and value-range controls.
- Density/additive mode selector.

## What to present in the demo

Show the tissue-scale overview first, with hundreds of thousands to millions of colored points. Zoom in smoothly until individual points become visible. Toggle between category mode and density mode. Then enable/disable LOD or switch point count to show why this stresses the renderer.

Suggested message:

> This is the napari Points layer at modern spatial-biology scale. The point data model is simple, but the renderer must handle millions of antialiased glyphs, feature-based coloring, filtering, and pan/zoom without forcing plugin authors to think about GPU details.

## Why this pressures the architecture

- Requires efficient large vertex/instance buffers.
- Requires fast updates of visual parameters without reuploading positions.
- Requires feature-based coloring in shader.
- Requires scalable blending and antialiasing.
- Useful for AI outputs: detections, embeddings, clusters, tracking points.
- Good benchmark for backend abstraction because Points are semantically simple.

## Minimal implementation plan

### Stage 1

1. Write `prepare_spatial_points.py` to extract and cache coordinates from SpatialData MERFISH or
   MIBI-TOF.
2. Export Datoviz-ready binary arrays and `metadata.json`.
3. Implement `examples/napari/dense_points_spatial_omics_glfw.c`.
4. Load the binary cache, or fall back to a synthetic stress dataset when the cache is missing.
5. Render the optional image background plus one point visual.
6. Precompute or load color buffers for categorical, continuous, and density modes.
7. Add GUI controls for point-count preset, color mode, point size, opacity, LOD, and background
   visibility.
8. Print FPS and rendered point count during live interaction.

### Stage 2

1. Implement native GLSL instanced-quad point lowering.
2. Add SDF antialiasing for circular/Gaussian markers.
3. Add point feature/style attributes for category, value, and opacity.
4. Implement shader-side feature color table and continuous colormap lookup.
5. Add a stronger density blending mode.
6. Add deterministic LOD or screen-space density culling.
7. Update the demo to use the Stage 2 path and keep the Stage 1 fallback if useful.

## Acceptance criteria

### Stage 1

- Loads a prepared cache or falls back to a synthetic stress dataset.
- Runs as a C GLFW example through the current v0.4 scene/app path.
- Renders the background image when present.
- Renders at least 1 million points interactively on a recent discrete GPU, or reports the measured
  maximum point count if the current point-sprite path falls short.
- Supports categorical, continuous, and density-like modes through precomputed RGBA8 colors.
- Supports point-count/LOD presets through precomputed subsets or prefix counts.
- Prints FPS and rendered point count.

### Stage 2

- Uses instanced antialiased point quads in the native app path.
- Supports categorical coloring and opacity updates without position-buffer reupload.
- Supports continuous colormap lookup without CPU color-buffer regeneration.
- Includes a density or LOD mode useful at zoomed-out scale.
