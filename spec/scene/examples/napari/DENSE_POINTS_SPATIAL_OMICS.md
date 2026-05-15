# DENSE POINTS SPATIAL OMICS

## Purpose

This demo shows a high-density `Points`-like napari workflow: millions of detected cells, transcript spots, molecular coordinates, or spatial-omics features over a tissue image. The goal is to demonstrate that a modern GPU backend can render dense biological point data with smooth pan/zoom, categorical coloring, opacity controls, and zoom-dependent level-of-detail.

This is one of the best demos for napari maintainers because the data model is simple but the rendering pressure is very high.

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
~/.cache/datoviz-napari-demos/spatialdata/merfish.zarr
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

Store it as:

```text
~/.cache/datoviz-napari-demos/spatial_points/merfish_points.npz
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
6. Save compact arrays for Datoviz.

If the real dataset extraction is too slow, generate a synthetic stress extension:

```python
# Preserve real tissue shape, but duplicate/jitter points to reach 1M-10M points.
points_big = jittered_replicates(points_xy, repeats=10, sigma=0.5)
category_big = np.tile(category, repeats)
```

This keeps the demo visually biological while reaching stress-test scale.

## Datoviz adaptation

### Required visuals

- Background image visual, optional but strongly recommended.
- Point visual with instanced circular or Gaussian markers.
- Optional density mode, either additive alpha or screen-space accumulation.

### GPU buffers

Use one structure-of-arrays or interleaved instance buffer:

```text
position: float32x2 or float32x3
size: float32
category: uint32
value: float32
```

Rendering modes:

1. **Categorical mode**
   - color by `category` using a color table.
2. **Continuous mode**
   - color by `value` using a colormap texture.
3. **Density mode**
   - small alpha, additive or premultiplied blending.
4. **LOD mode**
   - precomputed random subsamples or screen-space density culling.

Point shader:

- Use instanced quads, not native point sprites.
- Generate antialiased discs in fragment shader using signed distance to marker center.
- Use opacity correction for dense zoomed-out views.

## Demo UI

Controls:

- Dataset selector: MERFISH / MIBI-TOF / synthetic stress.
- Point count: real / 1M / 5M / 10M.
- Color mode: category / continuous / density.
- Point size slider.
- Opacity slider.
- LOD toggle.
- Filter by category.
- Background image visibility.

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

1. Write `prepare_spatial_points.py` to extract and cache coordinates from SpatialData MERFISH or MIBI-TOF.
2. Implement point visual with instanced quads and SDF antialiasing.
3. Implement feature color table and continuous colormap.
4. Implement stress duplication to 1M/5M/10M points.
5. Add GUI controls.
6. Record FPS while panning and zooming.

## Acceptance criteria

- Loads a real spatial-omics dataset or prepared cache.
- Renders at least 1 million points interactively on a recent GPU.
- Supports categorical coloring and opacity updates without point-buffer reupload.
- Includes an LOD or density mode useful at zoomed-out scale.
