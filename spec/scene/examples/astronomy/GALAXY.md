# Gaia Galaxy

## Summary

Example name: `gaia_galaxy`

Build a polished Python scene example that renders a Gaia-derived Milky Way star sample as an
interactive 3D transparent point cloud with per-star color, size, arcball/orbit control, and slow
default rotation. Runtime should load a bounded prepared `.npz` from the Datoviz cache, download it
from `datoviz/data` when missing, and fall back to a deterministic synthetic spiral-galaxy sample
if download or validation fails.

This is a rendering and interaction pressure test for high-density transparent points, per-point
attributes, animation, camera control, resource upload, and transparency frame planning. Present it
as a Gaia-derived star sample, not a complete physical galaxy simulation.

## Feature Pressure

- 100k-300k transparent 3D points with per-point position, color, and size.
- One-time static buffer upload; animation updates only camera or transform state.
- Arcball/orbit interaction over continuous slow rotation.
- Transparency stage participation, preferably weighted blended OIT when available.
- Cache/download/fallback behavior that keeps the example runnable from a clean checkout.
- Optional screenshot/headless capture with deterministic composition.

## Data And Resources

Preferred dataset: preprocessed Gaia DR3 `gaiadr3.gaia_source` subset stored in `datoviz/data`, for
example `gaia/gaia_galaxy_100k.npz`.

The runtime `.npz` must be render-ready and must not require `astropy`, `astroquery`, or Gaia TAP
access. Recommended fields:

```text
pos          float32[N, 3]   normalized Cartesian positions
color        float32[N, 4]   RGBA star color
size         float32[N]      point size
mag_g        float32[N]      optional metadata/debug
bp_rp        float32[N]      optional metadata/debug
source_id    uint64[N]       optional picking/debug
```

Recommended dataset size is 100k stars, acceptable 50k-300k, ideally under 20 MB compressed.

Preparation should query Gaia once offline, convert RA/DEC/parallax to approximate Cartesian
coordinates (`d = 1000 / parallax_mas`), center by median, robust-scale the 99th percentile radius
to about 1.0, clamp outliers, derive color from BP-RP, derive size/brightness from G magnitude, and
write float32 arrays.

Runtime loading:

1. Resolve a stable cache path, preferably through an existing Datoviz data helper.
2. Download from `datoviz/data` if missing.
3. Load with NumPy.
4. Validate fields, dtypes, shapes, and finite values.
5. Fall back to deterministic synthetic spiral data with a clear console warning.

## Scene And Runtime Behavior

Use the high-level Datoviz v0.4 scene layer, not low-level Vulkan/vklite calls.

Required scene:

- one window/canvas, about 1280 x 900 by default;
- one full-window 3D panel;
- perspective camera, default `position = (0, -2.6, 1.2)`, `target = (0, 0, 0)`, `fov = 45`;
- transparent point/marker/sprite visual with `position`, `color`, and `size`;
- arcball/orbit controller;
- continuous slow rotation around the vertical axis;
- optional minimal overlay with dataset name and star count.

Preferred visual is a transparent point sprite or marker visual. If unavailable, use instanced
billboard quads; plain point lists are a last resort.

Rendering style:

- black or very dark background;
- subtle accumulated density from transparency;
- visible warm/cool star colors without saturation;
- no axes or grid by default;
- oblique centered camera view.

Transparency:

- mark the star visual transparent;
- render through the transparent stage;
- use weighted blended OIT when available;
- otherwise use standard alpha blending with suitable depth settings.

Animation:

- smooth frame-rate-independent rotation;
- deterministic initial phase;
- update only model transform or camera azimuth;
- reduce/pause auto-rotation during user drag and resume after an idle delay.

Controls: left drag orbit, wheel zoom, optional pan gesture, `Space` pause/resume, optional `R`
reset, and normal example quit key.

Optional enhancements: two-layer core/halo stars, faint disk guide, density-adaptive alpha, LOD
switches, or picking of `source_id`, G magnitude, and BP-RP.

## Minimal Target

1. Load cached Gaia arrays or deterministic fallback arrays.
2. Create one scene/window/panel through the actual v0.4 Python API.
3. Configure a 3D camera and arcball/orbit controller.
4. Upload positions, colors, and sizes once.
5. Draw transparent points.
6. Animate only camera or transform state.
7. Keep implementation readable enough for the examples gallery.

## Validation

- Clean checkout run downloads the dataset when missing.
- Script starts without command-line arguments.
- A 3D star cloud appears quickly after data loading.
- Stars use individual Gaia-derived colors and sizes.
- Transparency accumulates into a dense galaxy-like cloud.
- Default rotation is smooth and arcball/orbit remains interactive.
- Static star buffers are not reuploaded every frame.
- Fallback path works when download or validation fails.
- Headless/screenshot mode, when supported, captures a centered oblique view with visible colors
  and no UI clutter.

Gallery metadata:

```yaml
title: Gaia Galaxy
tags: [3D, scatter, transparency, animation, astronomy, Gaia]
description: Interactive transparent 3D star cloud from a Gaia DR3-derived Milky Way sample.
```

References: Gaia Archive, Gaia DR3 documentation, Astroquery Gaia TAP+ access, and Gaia DR3 summary
papers.
