# Toy DICOM Viewer

> **Example status:** informative pressure test
> **Target:** Python multi-panel volume viewer
> **Data:** cached small volume with deterministic synthetic fallback
> **Validation:** smoke, linked slice, crosshair, texture, and raymarch checks

## Summary

Build a toy medical-volume viewer with three synchronized orthogonal slice panels and one 3D volume
panel. All four panels share one 3D texture and viewer state for cursor, slice indices,
window/level, opacity, interpolation, and sampling step. This is a research/demo viewer, not a
clinical DICOM application.

## User-Visible Result

```text
+-----------------------+-----------------------+
| Axial                 | Sagittal              |
| z = current iz        | x = current ix        |
+-----------------------+-----------------------+
| Coronal               | 3D volume             |
| y = current iy        | orbit camera          |
+-----------------------+-----------------------+
```

- Three 2D views show orthogonal slices with colored crosshairs.
- The 3D view raymarches the same volume texture inside a physically scaled box.
- Sliders update X/Y/Z slice indices, window center, window width, opacity, step size, and optional
  interpolation.
- Clicking or dragging in any 2D slice view updates the shared cursor and all crosshairs.
- The 3D panel uses arcball/orbit interaction.

## Feature Pressure Points

- 2x2 multi-panel layout with shared state and per-panel input routing.
- Three 2D cameras plus one 3D camera.
- One retained 3D texture sampled by several visuals.
- Slice shader with orientation-dependent coordinates.
- Raymarch volume shader with transparency and early termination.
- Small uniform/overlay updates without full volume reupload.
- Linked UI controls, mouse interaction, and cross-panel redraw.

## Required Data And Resources

Runtime cache:

```text
~/.cache/datoviz/toy_dicom_viewer/medical_volume_head_ct.npz
```

Required `.npz` content:

```text
volume        uint16 or float32[nz, ny, nx]
spacing       float32[3]  # (sz, sy, sx), millimeters
window_center float scalar
window_width  float scalar
modality      optional string
name          optional string
```

If no cached/downloaded volume is available, generate a deterministic synthetic CT-like phantom:
air around nested ellipsoids, soft tissue near `40`, skull-like shell near `700`, brain near `35`,
low-density ventricles near `5`, mild fixed-seed noise, spacing `(1.2, 1.0, 1.0)`, window center
`40`, and window width `400`.

Preferred texture upload formats, in order of practicality:

```text
r16float
r16unorm or r8unorm with display metadata
r32float
```

## Scene Shape And Runtime Behavior

Coordinate convention:

```text
volume[z, y, x]
spacing = (sz, sy, sx)
X = (x - 0.5 * (nx - 1)) * sx
Y = (y - 0.5 * (ny - 1)) * sy
Z = (z - 0.5 * (nz - 1)) * sz
```

Slice planes:

| View | Plane | Horizontal | Vertical |
|---|---|---|---|
| Axial | `z = iz` | x | y |
| Sagittal | `x = ix` | y | z |
| Coronal | `y = iy` | x | z |

Slice visual uniforms:

```text
orientation
slice_index or normalized slice
shape, spacing
window_center, window_width
interpolation or sampler choice
```

Window/level mapping:

```text
lo = window_center - 0.5 * window_width
hi = window_center + 0.5 * window_width
value01 = clamp((value - lo) / (hi - lo), 0, 1)
```

Volume rendering:

- Render a cube scaled to `(nx * sx, ny * sy, nz * sz)`.
- Intersect camera rays with the box.
- March in physical or texture-space steps.
- Sample the volume, apply window/level, map to grayscale color and opacity, composite
  front-to-back, and terminate near full alpha.
- Default transfer: grayscale with `opacity = 0.25` and smooth alpha threshold around
  `0.15..0.85`.

Crosshair mapping:

| View | Crosshair lines |
|---|---|
| Axial | x = `ix`, y = `iy` |
| Sagittal | y = `iy`, z = `iz` |
| Coronal | x = `ix`, z = `iz` |

## Minimal Implementation Target

- One canvas with four panels.
- One uploaded 3D texture.
- Three slice image visuals sampling that texture.
- One basic volume raymarch visual.
- Sliders for X/Y/Z, window center/width, opacity, and step size.
- 2D drag-to-update cursor and 3D arcball.
- Crosshair overlays in the three 2D panels.
- Synthetic fallback volume if cache/download is unavailable.

## Validation / Acceptance Criteria

- Runs without manual data preparation.
- The three 2D panels show mutually orthogonal slices through the same volume.
- The fourth panel shows a nonblank 3D volume rendering.
- Slice sliders and 2D dragging update all views and crosshairs immediately.
- Window/level controls affect both slices and volume rendering.
- Ordinary interaction updates uniforms and overlay buffers only; the full volume is not reuploaded.
- A `128^3` to `256^3` volume remains interactive at the default sampling step.
- Missing optional metadata or transfer-function assets do not break the fallback path.

## Links

- [Shared example policies](../POLICIES.md)
- [Dashboard rendering roadmap](../../dashboards/DASHBOARD_RENDERING_ROADMAP.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
