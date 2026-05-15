# LARGE LABELS SEGMENTATION

## Purpose

This demo shows the most napari-relevant rendering problem: a large microscopy image with a large instance-segmentation label map on top, rendered interactively with opacity, boundary rendering, random label coloring, and selected-label highlighting.

The goal is to make the case that a modern GPU backend should treat labels as a first-class rendering primitive, not as a CPU-recolored RGBA image. This is directly relevant to Cellpose/StarDist/ilastik/empanada-style workflows and to manual proofreading of AI segmentation results.

## Napari use case being mirrored

Napari's Labels layer is intended for displaying integer-valued segmentation masks, with label 0 as transparent background and nonzero integer IDs mapped to colors. It also supports interaction such as painting, filling, selection, and opacity. This demo should look like a napari Labels workflow, not like a generic shader demo.

Relevant napari concepts:

- `Image` layer: grayscale or multichannel microscopy image.
- `Labels` layer: integer instance map.
- Layer controls: opacity, colormap/random colors, selected label, visibility.
- Large-data stress: many labels, large texture, rapid pan/zoom.

References:

- Napari Labels layer guide: https://napari.org/stable/howtos/layers/labels.html
- BBBC038 dataset page: https://bbbc.broadinstitute.org/BBBC038
- Broad Bioimage Benchmark Collection: https://bbbc.broadinstitute.org/

## Dataset

Primary dataset: **BBBC038 / Kaggle 2018 Data Science Bowl nuclei segmentation dataset**.

Reason:

- It is a real biological segmentation benchmark.
- It contains microscopy images and per-nucleus binary masks.
- The BBBC page describes it as a large collection of segmented nuclei images with tens of thousands of nuclei, from different biological contexts, organisms, stains, magnifications, and illumination conditions.
- It directly maps to the napari image + labels workflow.

Suggested download locations:

```bash
mkdir -p data/bbbc038
cd data/bbbc038
wget -nc https://data.broadinstitute.org/bbbc/BBBC038/stage1_train.zip
unzip -n stage1_train.zip -d stage1_train
```

If that exact zip URL changes, fall back to the BBBC038 landing page and scrape/download the listed files from:

```text
https://data.broadinstitute.org/bbbc/BBBC038/
```

Fallback dataset:

- `skimage.data.human_mitosis()` or `skimage.data.cells3d()` with generated labels using thresholding + connected components.
- This is acceptable for development, but the demo should use BBBC038 if possible.

## Preprocessing pipeline

BBBC038 stores each image in an `ImageId` folder with:

```text
<ImageId>/images/<ImageId>.png
<ImageId>/masks/*.png
```

For each selected image:

1. Load the microscopy image as grayscale or RGB.
2. Load all binary masks from `masks/`.
3. Convert masks to a single integer label map:

```python
labels = np.zeros((H, W), dtype=np.uint32)
for label_id, mask_path in enumerate(mask_paths, start=1):
    mask = imageio.imread(mask_path) > 0
    labels[mask] = label_id
```

4. Store converted data in a cache file:

```text
~/.cache/datoviz-napari-demos/bbbc038_labels/<image_id>.npz
```

with fields:

```text
image: uint8 or float32, shape (H, W) or (H, W, 3)
labels: uint32, shape (H, W)
metadata: JSON string with image_id, source, mask_count
```

5. Optionally create a larger stress-test mosaic:

```python
image_big = tile_random_images(images, grid=(8, 8))
labels_big = tile_labels_with_id_offsets(labels_list, grid=(8, 8))
```

This produces a realistic large-label workload without requiring huge external files.

## Datoviz adaptation

### Required visuals

- `ImageVisual` or textured quad for the microscopy image.
- `LabelsVisual` implemented as an integer texture sampled in the fragment shader.
- Optional `BoundaryVisual`, computed either on CPU once or in shader from neighboring label samples.
- Optional `PointsVisual` for centroids, computed from labels.

### GPU resources

- Image texture:
  - format: `r8unorm`, `rgba8unorm`, or `r16float` depending on source.
  - sampled with linear filtering for grayscale image.
- Labels texture:
  - format: `r32uint` if available, or `r16uint` for small labels.
  - sampled with nearest filtering only.
- Label color lookup:
  - either hash label ID in shader;
  - or upload a `rgba8unorm` / `rgba32float` color table texture.

Recommended shader behavior:

```text
base = sample_image(image_tex, uv)
label_id = textureLoad(labels_tex, pixel_coord).r
label_color = label_hash_color(label_id)
alpha = 0 if label_id == 0 else labels_opacity
out = composite(base, label_color, alpha)
```

Boundary mode:

```text
id0 = label(x, y)
id1 = label(x+1, y)
id2 = label(x, y+1)
boundary = id0 != id1 or id0 != id2
```

Selected-label highlighting:

```text
if label_id == selected_label:
    boost alpha, edge width, or color saturation
```

## Demo UI

Controls:

- Dataset/image selector.
- Toggle labels visibility.
- Labels opacity slider.
- Color mode:
  - random hash;
  - categorical table;
  - selected label only;
  - boundary only.
- Boundary toggle.
- Stress-test size: original / 2x2 / 4x4 / 8x8 mosaic.
- Optional hover readout: pixel coordinate and label ID.

## What to present in the demo

Start with a normal microscopy image and labels overlay. Then switch to a large mosaic with thousands to tens of thousands of label IDs. Pan and zoom smoothly. Toggle boundary mode. Change opacity. Select one label and show that the selected object can be highlighted without recoloring the whole texture on the CPU.

Suggested message:

> This is a direct napari Labels workflow: microscopy image, instance segmentation, opacity, boundaries, and selection. The backend opportunity is to keep labels as integer GPU data and do coloring, boundaries, and interaction in the renderer instead of repeatedly materializing RGBA images on the CPU.

## Why this pressures the architecture

- Requires integer textures and nearest sampling.
- Requires compositing with another image layer.
- Requires low-latency uniform updates for opacity/selection.
- Requires correct pixel-to-world transforms and pan/zoom.
- Enables future GPU picking by reading label ID under cursor.
- Shows how napari layer semantics can map to a backend-neutral rendering primitive.

## Minimal implementation plan

1. Write `prepare_bbbc038_labels.py` to download and convert 10 representative BBBC038 images.
2. Implement a Datoviz textured image visual.
3. Implement a labels overlay shader using a `uint` texture or packed integer texture.
4. Add GUI controls for opacity, boundaries, and selected label.
5. Add stress-test mosaic generation.
6. Record a 30-second demo video and measure FPS/latency.

## Acceptance criteria

- Runs from a clean checkout with one command.
- Downloads/caches data automatically.
- Shows at least one real BBBC038 image with true instance labels.
- Supports at least 4x4 mosaic mode.
- Labels are colored in the shader from integer IDs.
- Opacity and selected label update without reuploading the full labels texture.
