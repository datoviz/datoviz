# VOLUME CLIPPING 3D

## Purpose

This demo shows a 3D microscopy volume rendered with transfer function, clipping plane, and optional label/point overlays. It should resemble napari's 3D image/volume workflows while demonstrating why a modern explicit GPU backend is useful for volume rendering, plane rendering, and large 3D data.

This is the visually strongest demo, but it should be scoped as an architecture stress test rather than a promise of full 3D napari parity.

## Napari use case being mirrored

Napari supports 3D image display and volume rendering modes. The napari gallery includes a volume-plane rendering example where a 3D volume is shown together with a plane depiction whose position, normal, and thickness can be modified interactively.

Relevant napari concepts:

- `Image` layer in `ndisplay=3`.
- Volume rendering modes: MIP, average, translucent-like rendering.
- Plane depiction: position, normal, thickness.
- 3D camera, axes, clipping, transfer function.
- Optional labels/points/surface overlays.

References:

- Napari volume plane rendering example: https://napari.org/0.4.16/gallery/volume_plane_rendering.html
- Napari Image layer guide: https://napari.org/stable/howtos/layers/image.html
- Large 3D rendering issue example: https://github.com/napari/napari/issues/7773
- OME-Zarr / IDR example via napari-ome-zarr: https://napari-hub.org/plugins/napari-ome-zarr.html

## Dataset

Primary dataset: **scikit-image `cells3d`**.

Reason:

- It is small enough to be reliable in a live demo.
- It is real 3D microscopy data.
- It requires no large download and works offline once dependencies are installed.
- It maps well to napari tutorials and common bioimage demos.

Load:

```python
from skimage import data
vol = data.cells3d()  # usually shape (Z, C, Y, X)
```

Use one channel as grayscale volume, or two channels with different transfer colors.

Optional open large-data dataset: **IDR OME-Zarr**.

Example URL used by napari-ome-zarr documentation:

```text
https://uk1s3.embassy.ebi.ac.uk/idr/zarr/v0.3/9836842.zarr/
```

This should be treated as an optional stretch dataset because remote OME-Zarr behavior can be network-dependent.

Fallback synthetic dataset:

- `skimage.data.binary_blobs(length=128, n_dim=3)` as in napari's volume-plane example.

## Preprocessing pipeline

For `cells3d`:

1. Load volume.
2. Select channel:

```python
vol = data.cells3d()[:, 1]  # nuclei/channel choice depends on data version
```

3. Normalize to `uint8` or `float16`:

```python
v = vol.astype(np.float32)
v = (v - np.percentile(v, 1)) / (np.percentile(v, 99) - np.percentile(v, 1))
v = np.clip(v, 0, 1)
```

4. Optionally downsample to 128^3 or 256^3 for stable performance.
5. Cache:

```text
~/.cache/datoviz-napari-demos/volumes/cells3d_128.npz
```

with:

```text
volume: float16 or uint8, shape (Z, Y, X)
voxel_size: float32[3]
metadata.json
```

For optional labels:

- Generate a crude segmentation by thresholding + connected components.
- Or show points extracted from local maxima.

## Datoviz adaptation

### Required visuals

- `VolumeVisual` using a 3D texture.
- `PlaneSliceVisual` sampling the same 3D texture on an arbitrary plane.
- Optional `PointsVisual` for detected nuclei centers.
- Optional `BoundsVisual` for volume bounding box.

### Volume rendering implementation

Use raymarching in fragment shader:

1. Render bounding cube faces.
2. Compute ray entry/exit through volume box.
3. Step through 3D texture.
4. Apply transfer function.
5. Composite front-to-back.

For a demo, implement only:

- MIP mode;
- alpha-composited mode;
- plane slice mode.

### Clipping plane

Plane parameters:

```text
plane_position: vec3
plane_normal: vec3
plane_thickness: float
```

Modes:

- show only slice plane;
- show volume clipped by plane;
- show volume + highlighted slice plane.

## Demo UI

Controls:

- Rendering mode: MIP / alpha / plane / clipped volume.
- Transfer function: grayscale / green / magenta / fire.
- Intensity window: min/max.
- Step count.
- Plane normal: X / Y / Z / camera-facing.
- Plane position slider.
- Plane thickness slider.
- Camera orbit toggle.

## What to present in the demo

Show a 3D volume, rotate it smoothly, then enable the plane. Move the plane through the volume. Switch to clipped-volume mode. Emphasize that this is a controlled test of 3D texture sampling, transfer functions, render passes, and camera interaction.

Suggested message:

> I would not claim full napari 3D replacement in this grant, but 3D volume rendering is an important stress test. It exposes exactly the GPU features where a modern backend can help: 3D textures, transfer functions, clipping planes, and robust camera semantics.

## Why this pressures the architecture

- Requires 3D texture support.
- Requires transfer-function textures or shader parameters.
- Requires correct 3D camera and transforms.
- Requires blending and depth handling.
- Useful for future labels/surfaces/points overlays in 3D.
- Directly maps to napari volume-plane examples.

## Minimal implementation plan

1. Prepare `cells3d` cache.
2. Implement volume texture upload.
3. Implement MIP raymarch shader.
4. Add plane slice shader.
5. Add GUI controls for transfer function, clipping, plane position.
6. Optional: add local maxima points overlay.

## Acceptance criteria

- Loads real 3D microscopy volume.
- Displays volume interactively in 3D.
- Supports at least MIP and plane-slice modes.
- Plane position/normal updates live.
- Demonstrates at least one overlay or bounding box.
