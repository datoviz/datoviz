# Diffusion MRI Tractography Example

> **Example status:** informative pressure test
> **Target:** Python 3D scientific viewer
> **Data:** prepared tractography cache, synthetic/prepared fallback if needed
> **Validation:** smoke, visual, picking/probe, and performance checks

## Summary

Build a polished neuroimaging viewer for large collections of variable-length 3D streamlines. The
first implementation may use the active path/line fallback; the long-term pressure is a reusable
tube/ribbon visual family, not a tractography-specific renderer.

## User-Visible Result

- Dark 3D scene showing a compact white-matter bundle, preferably fornix or a small multi-bundle
  tractogram.
- Direction-RGB default coloring: `R = abs(tangent.x)`, `G = abs(tangent.y)`, `B = abs(tangent.z)`.
- Arcball/orbit navigation with reset.
- Rendering modes: lines, tube impostors, tessellated tubes, and ribbons.
- Controls for dataset, rendering mode, color mode, radius/stroke width, alpha, subsampling,
  maximum streamlines, minimum length, depth test, lighting, optional weighted OIT, optional SSAO,
  and optional picking/highlight.

## Feature Pressure Points

- Ragged 3D geometry with `positions[N, 3]` and `offsets[M + 1]`.
- Per-vertex and per-streamline attributes.
- Path fallback today, tube/ribbon pressure for future visuals.
- Transparent dense 3D curves with depth handling.
- Per-streamline picking rather than per-vertex picking.
- LOD/subsampling by random selection, length, distance, screen density, or precomputed buckets.
- Optional OIT, SSAO, picking, and highlight stages.

## Required Data And Resources

Preferred prepared source: a compact Datoviz-ready file derived from DIPY's fornix example or a
small Zenodo tractography-format testing bundle. TractoInferno is too large for the default example
and should remain an advanced source only.

Runtime should not require DIPY, nibabel, or raw MRI processing. Offline preprocessing may use
`.trk`, `.tck`, `.trx`, `.vtk`, or `.vtp` inputs and save:

```text
data/tractography/fornix_streamlines.npz
```

Required layout:

```text
positions float32[N, 3]
tangents  float32[N, 3]
colors    uint8[N, 4] or float32[N, 4]
offsets   uint32[M + 1]
bundle_id uint16[M]
length    float32[M]
```

Optional:

```text
scalar float32[N]
radius float32[N]
picking_id uint32[M]
lod_level uint8[M]
bbox_min, bbox_max float32[M, 3]
```

The example should print dataset statistics: streamlines, vertices, mean length, visible
streamlines, and active mode.

## Scene Shape And Runtime Behavior

Scene shape:

```text
Scene
└── 3D Panel
    ├── Arcball camera
    ├── Path visual or tube/ribbon visual
    ├── Optional highlight overlay
    └── Optional anatomical context
```

Rendering modes:

| Mode | Purpose |
|---|---|
| Thin polylines | Required fallback and large-data baseline |
| Screen-space tube impostors | Preferred default if feasible |
| Tessellated tube mesh | Screenshot/quality reference and mesh-throughput stress |
| Ribbons | Dense-bundle alternative |

Color modes:

```text
Direction RGB
Bundle ID
Streamline length
Depth
Curvature
Solid color with alpha
```

Packed curve contract pressure:

```text
positions float32[N, 3]
offsets uint32[M + 1]
colors optional per-vertex or constant
radius optional per-vertex or constant
ids optional uint32[M]
```

Picking minimum: hover or click a streamline, highlight the full streamline, and show streamline
index, length, and bundle ID. CPU ray-to-polyline distance is acceptable initially; GPU ID picking
or BVH-assisted picking can follow.

## Minimal Implementation Target

- DIPY fornix-derived prepared `.npz` or deterministic prepared streamline fallback.
- One 3D panel with arcball camera.
- Direction-colored 3D line rendering through path/primitive facilities.
- ImGui controls for alpha, stroke/radius, subsampling, maximum streamlines, and color mode.
- No runtime DIPY/nibabel dependency.

Ideal next slice:

- Small multi-bundle dataset.
- Screen-space tube impostors.
- Per-streamline picking/highlight.
- Weighted OIT, optional SSAO, and LOD/subsampling.

## Validation / Acceptance Criteria

- Runs out of the box and resolves/caches its prepared data.
- Displays real or realistic tractography data, not unrelated random curves.
- Supports 3D arcball interaction.
- Provides line rendering and at least one enhanced mode when available.
- Direction coloring is correct and visually recognizable.
- Subsampling/LOD keeps interaction smooth for large bundles.
- Picking, when enabled, returns whole-streamline metadata.
- Minimum `10k-100k` vertices are interactive; `500k-2M` vertices are a good line/impostor target.

## Links

- [Shared example policies](../POLICIES.md)
- [Path visual](../../visuals/PATH.md)
- [Tube visual](../../visuals/TUBE.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
