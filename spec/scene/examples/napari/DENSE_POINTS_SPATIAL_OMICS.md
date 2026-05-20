# Dense Points Spatial Omics

## Summary

Build a dense napari Points-style spatial-omics demo that renders many biological coordinates over
tissue or atlas context with categorical, continuous, and density-like coloring. Stage 1 is a C
showcase example using the active scene -> DRP2 -> app path and current `dvz_point()` attributes:
`position`, `color`, and `size`. Stage 2 upgrades the point renderer for native instanced quads,
shader-side feature coloring, stronger opacity/density modes, and 3D ABC/Zhuang MERFISH data.

The demo mirrors napari Points workflows for cells, transcript spots, detections, centroids, and
spatial-biology annotations. The data model is simple; the renderer pressure is high.

## Feature Pressure

- 1M+ retained points with smooth pan/zoom.
- Categorical, continuous, and density-like color modes.
- Large buffer swaps for point-count and color-mode presets.
- Lightweight style changes for size and opacity.
- Optional image/atlas context.
- Deterministic LOD subsets and visible point-count diagnostics.
- Stage 2 shader-side category/value mapping without position-buffer reupload.

## Data And Resources

Stage 1 primary dataset: SpatialData MERFISH mouse brain, with MIBI-TOF or synthetic stress data as
fallbacks. Stage 2 showcase dataset: Allen Brain Cell Atlas / Zhuang whole-mouse-brain MERFISH.

Runtime must not require SpatialData, Zarr, NPZ, network access, or ABC Python packages. Preparation
tools may use those dependencies and export Datoviz-ready binary caches.

Stage 1 cache:

```text
.cache/datoviz-napari-demos/spatial_points/merfish/
  metadata.json
  positions_f32.bin            # N x 3, normalized scene coordinates
  category_u32.bin             # N
  value_f32.bin                # N
  colors_category_rgba8.bin
  colors_continuous_rgba8.bin
  colors_density_rgba8.bin
  lod_real_u32.bin             # optional index list
  lod_1m_u32.bin
  lod_5m_u32.bin
  lod_10m_u32.bin
  image_rgba8.bin              # optional W x H x 4 background
```

Stage 2 ABC/Zhuang uses the same shape under `abc_zhuang_merfish/`, with registered 3D coordinates,
meaningful categorical annotations, optional 2D atlas slice, and 1M/5M/10M LOD subsets.

`metadata.json` records point count, image dimensions, coordinate bounds, category count/names,
palette entries, dataset/source/license, and cache layout.

Preparation extracts representative coordinates, one categorical feature, one continuous feature,
normalizes world coordinates, exports color buffers and LOD subsets, and may create deterministic
jittered expansions only for Stage 1 stress testing. ABC/Zhuang should prefer real cells and
deterministic downsampling over synthetic expansion.

## Scene And Runtime Behavior

Stage 1 uses only current v0.4 capabilities:

- optional `dvz_image()` background;
- `dvz_point()` with `position`, `color`, and `size`;
- full array swaps for point-count/color-mode changes;
- small range updates only when useful;
- `dvz_panel_set_panzoom()` and `dvz_gui_*` controls;
- ordinary alpha blending for translucent points.

Stage 1 emulation rules:

| Mode | Stage 1 behavior |
|---|---|
| Categorical | precomputed RGBA8 colors from category |
| Continuous | precomputed RGBA8 colors from value |
| Density | low-alpha RGBA8 colors with ordinary blending |
| Opacity | may reupload color buffer |
| Filtering | compact/reupload arrays, or defer if too CPU-heavy |
| LOD | precomputed subsets or prefix counts |

Stage 2 requirements before claiming the full napari-class target:

- instanced quads rather than native point sprites;
- antialiased disc or Gaussian fragment shader;
- 3D point rendering with arcball and depth;
- global size/opacity style updates without position-buffer reupload;
- category/value attributes or equivalent style buffers;
- color-table and colormap texture binding;
- defined density blending;
- deterministic LOD rules visible in diagnostics.

Controls:

| Stage | Controls |
|---|---|
| 1 | dataset, point count, color mode, point size, opacity, LOD, background |
| 2 | SpatialData/ABC dataset, 3D/2D view, category filter, palette, colormap/range, density mode |

The demo should show tissue/brain overview first, zoom until individual points are visible, toggle
category and density modes, and print actual FPS plus rendered point count.

## Minimal Target

Stage 1:

1. Write a preparation tool for SpatialData MERFISH or MIBI-TOF.
2. Export the binary cache and `metadata.json`.
3. Implement a C showcase example under `examples/c/showcase/`.
4. Load cache or deterministic synthetic stress data.
5. Render optional background plus one point visual.
6. Support precomputed categorical, continuous, and density color buffers.
7. Add GUI controls for count, color mode, size, opacity, LOD, and background.
8. Print FPS and rendered point count.

Stage 2:

1. Add ABC/Zhuang MERFISH preparation to the same cache format.
2. Implement native instanced-quad point lowering and antialiasing.
3. Add shader-side feature/style attributes, color tables, colormap lookup, density blending, and
   deterministic LOD.
4. Make ABC/Zhuang the main showcase while retaining Stage 1 as lightweight fallback.

## Validation

Stage 1 acceptance:

- Loads a prepared cache or deterministic synthetic fallback.
- Runs as a C GLFW example through the active v0.4 scene/app path.
- Renders the background image when present.
- Renders at least 1M points interactively on a recent discrete GPU, or reports the measured limit.
- Supports categorical, continuous, and density-like modes via precomputed RGBA8 colors.
- Supports point-count/LOD presets via subsets or prefix counts.
- Prints FPS and rendered point count.

Stage 2 acceptance:

- Uses instanced antialiased point quads in the native app path.
- Shows the ABC/Zhuang 3D point cloud when present.
- Falls back to lightweight SpatialData or synthetic cache when ABC data is absent.
- Changes categorical coloring and opacity without position-buffer reupload.
- Performs continuous colormap lookup without CPU color-buffer regeneration.
- Provides a useful density or LOD mode at overview scale.

References: napari Points layer guide, SpatialData datasets/framework, and ABC Atlas Zhuang MERFISH
tutorials/screenshots.
