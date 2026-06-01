# Scene Transform Pipeline

Status: normative v0.4 scene pipeline spec.

This document defines scene coordinate spaces, transform stages, panel domains, attachment-space
rules, and CPU precision policy. DRP2 should receive visual-ready resources plus panel transform
state, not scientific coordinate semantics.


## Purpose

The transform pipeline keeps these concerns separate:

1. user/domain coordinates in scene state;
2. data-to-visual normalization;
3. panel-local navigation and camera state;
4. final render-facing clip/NDC transforms.


## Core Rules

1. Stage A, data-to-visual normalization, is separate from Stage B, visual-to-panel viewing.
2. Data normalization belongs above DRP2 and is usually CPU-side and cached.
3. Panel navigation belongs to panels/controllers and must not usually re-normalize or reupload
   visual resources.
4. All CPU-side normalization and geometry utility operations run in F64; downcast to F32 happens
   only at `UploadNode` time after normalization.


## Coordinate Spaces

| Space | Meaning |
|---|---|
| `DataSpace` | user or domain coordinates: measurement, voxel, geographic, simulation, plot |
| `VisualSpace` | family-ready coordinates after normalization/layout, shared across panels when possible |
| `PanelSpace` | after panel-local panzoom, camera, framing, or viewport interpretation |
| `ClipSpace` / `NDC` | final render-facing normalized device coordinates |


## Transform Stages

| Stage | Input -> Output | Owner | Invalidated by | Not invalidated by |
|---|---|---|---|---|
| A: normalization | `DataSpace` -> `VisualSpace` | scene/resource/family | source data, domain, scale, family interpretation | pan, zoom, camera |
| B: viewing | `VisualSpace` -> `PanelSpace` -> `NDC` | panel/controller | panzoom, camera, viewport, framing | source data or normalization policy |

`FramePlan` resolves Stage A first when dirty, then combines normalized resources with current
panel-local transform state before DRP2 emission.


## Family Notes

| Family | Stage A expectation |
|---|---|
| `pixel`, `point`, `marker`, `segment` | normalize per-item positions/endpoints |
| `path` | normalize grouped source coordinates while preserving spans |
| `glyph` | perform layout/family placement before panel navigation |
| `image` | place sampled field in visual-ready coordinates; volume slice remains a `volume` mode |
| `mesh` | normalize source geometry before camera transforms |
| `sphere` | normalize centers/radii; impostor versus mesh variant does not change semantics |
| `volume` | frame volumetric data in `VisualSpace`; traversal remains family logic |


## Panel Domains

The panel owns a data-space domain per dimension for visuals attached with `DVZ_COORD_DATA` unless a
per-visual override is provided. `DvzDataDomain` has:

| Field | Description |
|---|---|
| `min` | lower bound in data-space units; may be greater than `max` for inversion |
| `max` | upper bound in data-space units |
| `scale` | `DVZ_SCALE_LINEAR` or `DVZ_SCALE_LOG` |

Domain source modes:

| Mode | Behavior |
|---|---|
| explicit | user declares bounds; normalization and axis ticks use them |
| fit-to-data | one-time scan sets bounds, then behaves as explicit |
| pass-through | no domain exists; data is already `VisualSpace`; axes may suppress labels or show normalized coordinates |

Rules:

1. Log domains map values through `log10` before interpolation; nonpositive values are diagnostic.
2. `min > max` inverts the axis; ticks follow the declared order from `min` toward `max`.
3. All visuals using the same panel domain must already share the same data coordinate system; the
   scene does not reconcile mixed physical units.


## Coordinate Transform Stage

Status: deferred future work. v0.4 does not install a public API that applies this stage inside the
scene. Users should pre-project nonlinear coordinates on the CPU before upload when they need polar,
geographic, or other nonlinear projections.

A future visual attachment or panel projection descriptor may apply a coordinate transform before
panel-domain normalization:

```text
DataSpace -> coord_transform -> Cartesian DataSpace -> domain normalization -> VisualSpace
          -> controller -> PanelSpace -> NDC
```

| Transform | Input | Output | Notes |
|---|---|---|---|
| `DVZ_TRANSFORM_NONE` | Cartesian | unchanged | default |
| `DVZ_TRANSFORM_POLAR` | `(r, theta)` | `(x, y)` | angle in radians unless overridden |
| `DVZ_TRANSFORM_SPHERICAL` | `(r, theta, phi)` | `(x, y, z)` | physics convention |
| `DVZ_TRANSFORM_GEO_MERCATOR` | `(lon, lat)` | `(x, y)` | Web Mercator by default |
| `DVZ_TRANSFORM_GEO_GLOBE` | `(lon, lat, alt)` | `(x, y, z)` | unit sphere plus radial altitude |

`DvzTransformParams` may set `sphere_radius`, `center_lon`, `center_lat`, and `angles_degrees`.
Zero-initialized params use defaults. Panel domains always refer to the post-transform Cartesian
space. Polar axes, graticules, map tiling/LOD, and geographic tick formatting are deferred.

The installed v0.4 public ABI is intentionally limited to already-supported attachment fields:
`struct_size`, `flags`, `z_layer`, and `controller_mode`. Future coordinate-space,
domain-override, or transform descriptor fields should be appended to `DvzVisualAttachDesc` or
introduced through a new growable descriptor with the same `struct_size`/`flags` prologue. v0.4
must reject unknown attachment flags rather than accepting a no-op projection request.


## Aspect Ratio

Aspect ratio is a panzoom controller property, not a normalization property.

| Value | Behavior |
|---|---|
| `DVZ_ASPECT_FREE` | X and Y scale independently |
| `DVZ_ASPECT_EQUAL` | zoom uses the same data-unit-per-pixel ratio for X and Y |

The constraint applies after normalization in `VisualSpace`. It represents equal data units only when
the declared X/Y domains use matching physical extents.


## Visual Attachment

`DvzVisualAttachDesc` currently declares controller application and draw order. The future
attachment contract may also declare coordinate interpretation and optional domain overrides.

| Field | Rule |
|---|---|
| `coord_space` | future field: `DVZ_COORD_DATA`, `DVZ_COORD_NDC`, or `DVZ_COORD_PIXEL` |
| `coord_transform` / `transform_params` | future field: optional pre-normalization transform |
| `controller_mode` | `DVZ_CONTROLLER_APPLY` or `DVZ_CONTROLLER_FIXED` |
| `domain_x/y/z` | future field: `NULL` uses panel domain; non-`NULL` overrides that dimension |
| `z_layer` | signed draw order; lower draws first; same layer uses insertion order |

Coordinate-space meanings:

| Value | Meaning |
|---|---|
| `DVZ_COORD_DATA` | normalize through panel or per-visual domain |
| `DVZ_COORD_NDC` | already normalized to `[-1, 1]` |
| `DVZ_COORD_PIXEL` | panel pixel coordinates converted using current panel size |

`coord_space` and `controller_mode` are independent. Typical combinations include
`DATA+APPLY` for data visuals, `NDC+FIXED` for static overlays, and `PIXEL+FIXED` for legends,
scale bars, or panel-corner annotations.


## Dual-Axis And Mixed-Space Overlays

Per-visual domain overrides enable:

1. dual axes (`twinx`/`twiny`), where visuals share one dimension but normalize another against a
   separate domain;
2. mixed-space overlays, where visuals in one panel use different data coordinate systems.

The scene normalizes each visual independently into the same `VisualSpace` and applies the same
controller rules. Semantic consistency between unlike domains remains the caller's responsibility.
Secondary axes bind to the override domain, not the primary panel domain.


## CPU Precision Policy

Stage A uses F64 on the CPU. F64 source positions from C or Python/NumPy must not be silently
downcast at ingestion. F32 source data is accepted and remains F32 until normalization needs its
declared type. After normalization to a bounded `VisualSpace`, F32 GPU coordinates are sufficient for
rendering. GPU-side double-single/F64 emulation is reserved for future compute paths that must work
on unnormalized high-precision coordinates.

Geometry utilities follow the same F64 policy; see
[`../semantics/GEOMETRY_UTILITIES.md`](../semantics/GEOMETRY_UTILITIES.md).


## Relationships

| Spec | Relationship |
|---|---|
| [RESOURCE_MODEL.md](RESOURCE_MODEL.md) | stores source and normalized resources plus panel-derived parameter state |
| [INVALIDATION_AND_CACHING.md](INVALIDATION_AND_CACHING.md) | defines `NormalizationDirty` and `PanelTransformDirty` |
| [`../semantics/AXES.md`](../semantics/AXES.md) | axes choose ticks in `DataSpace` and place geometry in `VisualSpace` |
| [`../semantics/ANNOTATIONS.md`](../semantics/ANNOTATIONS.md) | anchors may originate in data, visual, panel, or viewport space |
