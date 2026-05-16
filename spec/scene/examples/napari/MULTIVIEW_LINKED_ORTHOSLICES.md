# Multiview Linked Orthoslices

> **Agent Pickup**
> - **Category:** `napari`
> - **Implementation target:** Napari-class scene pressure test, usually staged as a current v0.4 demo plus a richer follow-up.
> - **Data policy:** Use public sample data or deterministic synthetic fallback; cache prepared arrays/textures explicitly.
> - **Preprocessing:** Required when data is downloaded, resampled, tiled, labeled, or packed for GPU upload.
> - **Validation:** Stage-specific acceptance criteria covering current v0.4 behavior and the richer napari-class target.


## Summary

Build a linked multiview volume viewer with XY, XZ, YZ, and 3D overview panels sharing one
underlying dataset and synchronized cursor or slice state. The main data source is the cached
`cells3d` volume reused from the volume-clipping demo; optional OME-Zarr data is only a stretch path
when already cached. The first implementation slice should load the prepared 3D volume, derive three
2D slice textures from the current coordinate, render them in separate panels, and propagate
crosshair or scroll updates across views. Validation should follow the file's acceptance criteria:
all panels remain synchronized, the 3D overview reflects the active slice planes, and interaction
does not duplicate the underlying volume state.

## Purpose

This demo shows a multi-panel viewer with linked orthogonal slice views and a 3D overview: XY, XZ, YZ, and 3D. It is meant to connect directly to napari's architectural discussions around multiple canvases, linked layers, shared data, and synchronized state.

This demo is less flashy than a volume renderer, but it is highly strategic for napari maintainers because it stresses the viewer/backend boundary, event routing, shared GPU resources, and multi-view synchronization.

## Napari use case being mirrored

Napari supports linked layers and has examples for multiple viewers / linked layers, but robust native multi-canvas workflows are still an important architectural topic. Multi-view orthoslice layouts are standard in medical imaging, microscopy, electron microscopy, and volume annotation.

Relevant napari concepts:

- Multiple viewer/canvas layout.
- Linked layers and synchronized attributes.
- One dataset, several views.
- Shared dims/current slice state.
- Crosshair and cursor position.
- 2D and 3D views of the same data.

References:

- Napari linked layers example: https://napari.org/gallery/linked_layers.html
- Multiple viewer example discussion: https://forum.image.sc/t/multiple-viewer-in-one-napari-window-example/69627
- Napari Dask/large-dataset tutorial: https://napari.org/stable/tutorials/dask.html
- OME-Zarr visualization with napari: https://imaging.epfl.ch/field-guide/sections/image_data_visualization/notebooks/visualization_zarr.html

## Dataset

Primary dataset: **scikit-image `cells3d`**.

Reason:

- Real 3D microscopy stack.
- Lightweight and reliable.
- Same dataset as the 3D volume demo, reducing data-preparation time.
- Good for orthogonal slices.

Optional dataset:

- Any OME-Zarr volume opened through `napari-ome-zarr`, preferably a small IDR volume.
- Use only if already cached.

## Preprocessing pipeline

Reuse the cached volume from `VOLUME_CLIPPING_3D`:

```text
~/.cache/datoviz-napari-demos/volumes/cells3d_128.npz
```

Extract:

```text
volume: float16 or uint8, shape (Z, Y, X)
```

For each frame, slice planes are generated from the current cursor/slice coordinate:

```text
XY: volume[z, :, :]
XZ: volume[:, y, :]
YZ: volume[:, :, x]
```

Optionally cache downsampled 3D texture for overview.

## Datoviz adaptation

### Required panels

Use a 2x2 grid:

```text
+----------------+----------------+
| XY slice       | XZ slice       |
+----------------+----------------+
| YZ slice       | 3D overview    |
+----------------+----------------+
```

### Shared GPU resources

Ideal implementation:

- Upload volume once as a 3D texture.
- All panels sample the same 3D texture.
- Slice views draw textured quads using different coordinate mappings.
- 3D overview draws volume bounding box and optional slice planes.

Simpler implementation:

- Upload three 2D slice textures per update.
- Still demonstrate linked state, but less compelling.

The more strategic version is the shared 3D texture approach.

### Event and interaction behavior

- Mouse move/click in any 2D panel updates crosshair coordinate.
- The other 2D panels update their crosshairs.
- The 3D panel updates three slice planes.
- Scroll wheel changes slice index in the panel orientation.
- Optional drag moves crosshair.

## Demo UI

Controls:

- Current x/y/z sliders.
- Toggle crosshair.
- Toggle 3D overview slice planes.
- Link zoom toggle.
- Colormap/window controls.
- Dataset selector if multiple volumes are available.

## What to present in the demo

Show the four views. Move the crosshair in the XY panel and show the XZ/YZ views and 3D slice planes updating immediately. Then interact with the 3D overview. Explain that one volume resource is shared across all views.

Suggested message:

> This demo is about backend architecture, not just rendering. A future napari backend needs to support multiple coordinated views of the same data without duplicating GPU resources or hard-coding one canvas per viewer.

## Why this pressures the architecture

- Requires multiple panels/canvases/viewports.
- Requires shared resources across panels.
- Requires synchronized camera/dims/cursor state.
- Requires event routing from panel to global scene state.
- Requires per-panel transforms and texture-coordinate mappings.
- Directly relevant to napari multi-view / linked-layer workflows.

## Minimal implementation plan

1. Reuse `cells3d` volume cache.
2. Create a Datoviz scene with four panels.
3. Upload volume as a 3D texture.
4. Implement XY/XZ/YZ slice shaders sampling the 3D texture.
5. Implement crosshair overlay in panel coordinates.
6. Implement 3D overview with slice planes.
7. Add event routing to update shared x/y/z state.

## Acceptance criteria

- Four panels render simultaneously.
- One shared dataset drives all views.
- Crosshair/slice coordinate updates propagate to all panels.
- 3D overview shows current slice planes.
- Demo is stable enough for live use.
