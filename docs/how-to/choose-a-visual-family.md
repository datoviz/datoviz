# Choose a Visual Family

Pick the right visual type before writing code — the choice determines data layout, GPU cost, and available interactions.

## Overview

Datoviz organizes GPU primitives into *visual families*. Each family has a fixed data model (what attributes it accepts), a dimensionality (2D panel space, 3D world space, or screen overlay), and a characteristic density range. Choosing the wrong family usually means either fighting the API to reshape data or paying GPU cost you don't need.

## Decision table

| If your data is… | Use | Notes |
|---|---|---|
| Irregular 2D/3D point samples | `point` or `marker` | `point` is fastest; `marker` adds shape/size/outline encoding |
| Dense pixel-level data or screenshots | `pixel` or `image` | `pixel` maps one fragment per data point; `image` uploads a texture |
| A regular 2D scalar or RGBA field | `image` | Supports bilinear interpolation and probe readout |
| A 3D sampled scalar field | `volume` | Raycasted; check WebGPU portability status before use |
| Ordered line strips | `path` | Handles joins and caps; use `segment` for unconnected pairs |
| Unconnected line pairs | `segment` | One (start, end) pair per item; faster than `path` for sparse lines |
| Explicit triangle geometry | `mesh` | Requires vertex + index buffers, normals; supports materials and arcball |
| 3D Gaussian splats | `splat` | Alpha-sorted billboards; suited for NeRF-style point clouds |
| Character strings or labels | `text` or `glyph` | `text` for high-level strings; `glyph` for direct font-atlas access |
| Anchored per-item numeric labels | `labels` | Probe-aware; stays in screen space while data pans/zooms |
| Arrow or flow vectors | `vector` | One (position, direction, magnitude) triple per item |
| Geometric primitives (triangles, quads…) | `primitive` | Low-level; use when none of the above fit |

## Visual families at a glance

**point** — the workhorse for scatter plots and particle systems. Uniform round discs. Fastest draw path; scales to millions of items. No shape or stroke encoding.

**marker** — extends `point` with per-item shape (circle, square, diamond, cross, …), outline color, and outline width. Slightly heavier than `point`; use when shape encodes a categorical variable.

**pixel** — one screen pixel per data point. Useful for dense rasters where sub-pixel markers would alias anyway.

**image** — a 2D texture mapped onto a panel-space rectangle. Supports bilinear filtering and colormap remapping. The right choice for heatmaps, microscopy images, and spectrograms.

**volume** — a 3D texture rendered by raycasting. Supports transfer functions and depth-cue compositing. Check `webgpu.status: native-only` before shipping to browser targets.

**path** — a polyline with configurable join style (miter, bevel, round) and cap style. Items are variable-length line strips; all strips share one draw call.

**segment** — a list of independent (start, end) pairs. No joins; cheaper than `path` when continuity is not needed.

**mesh** — indexed triangle geometry with normals, UV coordinates, and material slots. Required for any 3D surface. Pairs with arcball or turntable controllers.

**splat** — alpha-sorted Gaussian splats for 3D point clouds. Each item has a position, covariance, and color.

**text** — high-level string rendering. Each item is a UTF-8 string with a position, anchor, and style. Backed by a font atlas.

**glyph** — low-level access to the font atlas. Each item is a single glyph quad. Use `text` unless you need per-glyph control.

**labels** — screen-space numeric or string labels that follow data-space anchors through pan/zoom. Integrates with the probe/readout system.

**vector** — arrow glyphs encoding direction and magnitude. One item per vector.

**primitive** — raw GPU primitive (points, lines, triangles) without any datoviz encoding layer. Escape hatch for custom geometry.

## Common patterns

Pick a family, then check three things:

**Update pattern.** Static data loads once at creation. Streaming data should use partial updates (`dvz_visual_set_data_range`) rather than full replacement each frame. Some families (mesh, volume) are heavier to update than others (point, pixel).

**Dimensionality and controller.** 2D families (point, marker, pixel, image, path, segment, labels) pair naturally with panzoom. 3D families (mesh, volume, splat) pair with arcball or turntable. Mixing both in one panel is possible but requires explicit `DVZ_DIM_MASK_*` bindings.

**WebGPU portability.** `point`, `marker`, `pixel`, `image`, `path`, `segment`, `text`, and `labels` are portable. `mesh`, `volume`, `splat`, and `vector` are marked `native-only` or `webgpu-planned` in the current release — check `examples/c/MANIFEST.yaml` for the authoritative status.

## See also

- [Add a visual](add-a-visual.md) — how to instantiate any visual family in a panel
- [Use colormaps](use-colormaps.md) — colormap and scalar field encoding
- [Use sampled fields](use-sampled-fields.md) — `image` and `volume` data layout
- [Lighting and materials](lighting-and-materials.md) — `mesh` material slots
