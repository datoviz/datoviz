# Tracks, Vectors, Shapes, And Cell Motion

> **Agent Pickup**
> - **Category:** `napari`
> - **Implementation target:** Napari-class scene pressure test, usually staged as a current v0.4 demo plus a richer follow-up.
> - **Data policy:** Use public sample data or deterministic synthetic fallback; cache prepared arrays/textures explicitly.
> - **Preprocessing:** Required when data is downloaded, resampled, tiled, labeled, or packed for GPU upload.
> - **Validation:** Stage-specific acceptance criteria covering current v0.4 behavior and the richer napari-class target.


## Summary

Build a napari-style time-lapse cell-motion scene combining image frames, segmentation masks,
tracks, instantaneous motion vectors, and optional path or skeleton overlays. The preferred data is
the Fluo-N2DH-SIM+ Cell Tracking Challenge dataset, with precomputed cached frames, labels, and
track arrays; development may fall back to deterministic synthetic motion data when downloads or
model-generated tracks are unavailable. The first practical slice should use the current v0.4 scene
path to animate a small prepared frame range with retained image data plus point, line/path, and
vector-like overlays. Validate against the staged acceptance criteria: smooth time updates, stable
track tails and overlays, and a clear path toward the richer napari-class target.

## Purpose

This demo shows dynamic cell-tracking data with image frames, segmentation masks, tracks, motion vectors, and optional path/shape overlays. It mirrors napari's `Tracks`, `Vectors`, and `Shapes` workflows while stressing the rendering of many time-dependent geometric primitives.

This demo is important because it moves beyond images and labels into higher-level biological annotations: trajectories, branches, paths, motion, and vector fields.

## Napari use case being mirrored

Napari has a Tracks layer for visualizing trajectories in time and a Vectors layer for displaying many vectors with positions and directions. Napari docs also show 3D skeleton paths through the Shapes layer, while noting that 3D interactivity for Shapes has limitations in some contexts.

Relevant napari concepts:

- `Image` layer: time-lapse microscopy.
- `Labels` layer: segmentation masks.
- `Tracks` layer: cell trajectories.
- `Vectors` layer: instantaneous motion vectors.
- `Shapes` layer: paths/skeletons/line strips.
- Time slider and fading track tails.

References:

- Napari cell tracking gallery example: https://napari.org/stable/gallery/cell_tracking.html
- Napari Vectors layer guide: https://napari.org/stable/howtos/layers/vectors.html
- Cell Tracking Challenge 2D datasets: https://celltrackingchallenge.net/2d-datasets/
- Fluo-N2DH-SIM+ Zenodo record: https://zenodo.org/records/15608207
- Skan 3D skeletons in napari: https://skeleton-analysis.org/stable/examples/visualizing_3d_skeletons.html

## Dataset

Primary dataset: **Fluo-N2DH-SIM+ Cell Tracking Challenge dataset**.

Reason:

- It is a standard cell tracking benchmark.
- It is directly used in napari tracking examples and trackastra examples.
- It contains time-lapse microscopy frames and segmentation/tracking annotations.
- Download size is reasonable for pre-download (~91-99 MB depending on source).

Download:

```bash
mkdir -p data/ctc
cd data/ctc
wget -nc https://data.celltrackingchallenge.net/training-datasets/Fluo-N2DH-SIM+.zip
unzip -n Fluo-N2DH-SIM+.zip -d Fluo-N2DH-SIM+
```

Alternative source:

```text
https://zenodo.org/records/15608207
```

Napari's current cell-tracking gallery also downloads small files from Zenodo/Google Drive and uses trackastra to generate napari tracks. For a demo, prefer precomputed tracks to avoid model download/runtime uncertainty.

## Preprocessing pipeline

The Cell Tracking Challenge format typically contains:

```text
01/t000.tif, t001.tif, ...
01_GT/TRA/man_track.txt
01_GT/TRA/man_trackXXX.tif
01_ST/SEG/man_segXXX.tif  # if available
```

Prepare a compact cache:

```text
~/.cache/datoviz-napari-demos/tracks/fluo_n2dh_sim.npz
```

with:

```text
images: uint8, shape (T, H, W), optionally downsampled
labels: uint32, shape (T, H, W), optional
tracks: float32/int64 table, shape (N, 4), columns [track_id, t, y, x]
graph_edges: optional int64, shape (M, 2)
vectors: float32, shape (K, 2, 2), napari-style [origin, direction]
paths: packed polyline representation for full trajectories
```

Build tracks from CTC lineage files:

1. For each label at each frame, compute centroid.
2. Link centroids using `man_track.txt` lineage IDs.
3. Save table in napari-compatible format:

```text
track_id, t, y, x
```

Build vectors:

```python
for each track_id:
    for consecutive points p_t, p_t1:
        origin = p_t
        direction = p_t1 - p_t
```

Build paths:

- one polyline per track;
- color by track ID, speed, lifetime, or division lineage.

The active visual target is [`path`](../../visuals/PATH.md). Future 3D radius-bearing track
histories should use the spec-only [`tube`](../../visuals/TUBE.md) contract instead of defining a
separate track renderer.

## Datoviz adaptation

### Required visuals

- Image visual for current frame.
- Optional labels visual for current segmentation mask.
- Tracks visual:
  - fading recent tail;
  - full `path` mode;
  - color by track ID or speed.
- Vectors visual:
  - arrows from current position to next frame;
  - optional velocity color.
- Points visual:
  - current cell centroids.

### GPU data layout

For dynamic time display, avoid rebuilding all geometry every frame.

Recommended buffers:

```text
track_vertices: float32x3, [x, y, t]
track_id: uint32
track_offset/count arrays
motion_vectors: float32, [x, y, dx, dy, t, track_id]
```

Shader filters by current time:

```text
visible = abs(vertex_t - current_t) <= tail_length
alpha = 1 - (current_t - vertex_t) / tail_length
```

For arrows, either:

- generate arrow geometry on CPU once;
- or use instanced arrow glyphs in shader.

## Demo UI

Controls:

- Time slider / play-pause.
- Tail length slider.
- Show image / labels / points / vectors / tracks toggles.
- Color mode: track ID / speed / lineage / lifetime.
- Vector scale slider.
- Playback speed.
- Optional selected track ID.

## What to present in the demo

Play the time-lapse. Show segmentation masks and centroids. Enable tracks with fading tails. Enable motion vectors for the current frame. Select or highlight one trajectory. Then increase the number of tracks synthetically if needed to stress the renderer.

Suggested message:

> Napari is not only an image viewer; it also renders biological annotations such as tracks, vectors, paths, and skeletons. A modern backend should handle these geometric layers efficiently and consistently with image/label layers.

## Why this pressures the architecture

- Time-dependent geometry.
- Mixed layer types: image, labels, points, vectors, paths/tracks.
- Per-frame filtering without CPU reallocation.
- Antialiased lines/arrows.
- Feature-based coloring.
- Good test for GPU buffers, indexing, and uniforms.

## Minimal implementation plan

1. Download and cache Fluo-N2DH-SIM+.
2. Convert frames to a compact `uint8` stack.
3. Parse segmentation/tracking annotations.
4. Generate tracks, vectors, and polyline buffers.
5. Implement time-filtered rendering shader.
6. Add GUI timeline and layer toggles.
7. Optionally add synthetic track multiplication for stress testing.

## Acceptance criteria

- Plays a real time-lapse microscopy sequence.
- Shows tracks and/or vectors over the image.
- Current time updates without rebuilding all buffers.
- Supports fading tail and vector scale controls.
- Can show at least one biologically meaningful lineage/trajectory.
