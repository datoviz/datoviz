# Scene Transform Pipeline

Status: normative v0.4 scene pipeline spec.

This document defines scene coordinate spaces, transform stages, panel data domains, panel view
framing, attachment-space rules, and CPU precision policy. DRP2 should receive visual-ready
resources plus resolved panel transform state, not scientific coordinate semantics.


## Purpose

The transform pipeline keeps these concerns separate:

1. user/domain coordinates in scene state;
2. data-to-visual normalization;
3. panel-local navigation and camera state;
4. final render-facing clip/NDC transforms.


## Core Rules

1. Panel data domains, panel view/framing, and controller navigation are separate concepts.
2. Data normalization belongs above DRP2 and is usually CPU-side and cached.
3. Panel view/framing resolves 2D aspect policy and DATA-to-VIEW state from panel domains and the
   plot rectangle.
4. Controller navigation mutates visible view state and gesture policy; it must not usually
   re-normalize or reupload visual resources.
5. All CPU-side normalization and geometry utility operations run in F64; downcast to F32 happens
   only at `UploadNode` time after normalization.


## Coordinate Spaces

User-facing scene coordinates use a Cartesian, right-handed convention:

1. `+X` points right in the default panel view.
2. `+Y` points up in the default panel view.
3. `+Z` points from back to front, toward the default camera.

This is the convention for `DataSpace` and `VisualSpace`. Backend clip-space differences, such as
Vulkan's framebuffer Y direction and depth range, are handled below the scene layer and are not
visible to users.

| Space | Meaning |
|---|---|
| `DataSpace` | user or domain coordinates: measurement, voxel, geographic, simulation, plot |
| `ViewSpace` | metric 2D panel view coordinates after panel DATA-to-VIEW mapping and view framing |
| `PanelSpace` | normalized panel coordinates over the full panel rectangle, intentionally viewport-shaped |
| `VisualSpace` | family-ready coordinates after normalization/layout, used by non-panel-specific or 3D visuals |
| `ClipSpace` / `NDC` | final render-facing normalized device coordinates |


## Transform Stages

| Stage | Input -> Output | Owner | Invalidated by | Not invalidated by |
|---|---|---|---|---|
| A: normalization | source data -> family-ready resources | scene/resource/family | source data, scale, family interpretation | pan, zoom, camera |
| B: panel view | `DataSpace` -> `ViewSpace` and `ViewSpace` -> plot clip | panel view resolver | panel domains, plot rect, view aspect/framing policy | controller pan or zoom that only changes visible extent |
| C: navigation | `ViewSpace` or `VisualSpace` -> `ClipSpace` / `NDC` | panel/controller | panzoom, camera, viewport, controller state | source data or normalization policy |

`FramePlan` resolves Stage A first when dirty, resolves panel view state from panel domains and
plot geometry, then combines normalized resources with current panel-local transform state before
DRP2 emission.


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

The panel owns a source data-space domain per dimension for visuals attached with `DVZ_VISUAL_COORD_DATA`
unless a per-visual override is provided. The panel view resolver may derive fitted and visible
domains from this source state, but it must not rewrite the source domain to apply view aspect
policy. `DvzDataDomain` has:

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
4. For ordinary `min < max` linear domains, smaller X values appear to the left, larger X values
   appear to the right, smaller Y values appear lower, and larger Y values appear higher.


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

The installed v0.4 public ABI supports `coord_space=DVZ_VISUAL_COORD_VIEW`,
`coord_space=DVZ_VISUAL_COORD_DATA`, `coord_space=DVZ_VISUAL_COORD_PANEL`, and
`coord_space=DVZ_VISUAL_COORD_PANEL_PIXEL` in `DvzVisualAttachDesc`. Future domain-override or
nonlinear transform descriptor fields should be appended to `DvzVisualAttachDesc` or introduced
through a new growable descriptor with the same `struct_size`/`flags` prologue. v0.4 must reject
unknown attachment flags or unsupported coordinate-space values rather than accepting no-op
projection requests.

`dvz_visual_attach_desc()` and `dvz_panel_add_visual(panel, visual, NULL)` default to
`coord_space=DVZ_VISUAL_COORD_DATA`. Without an explicit panel domain, DATA uses the default `[-1, +1]`
domain and therefore maps normalized examples like VIEW. With explicit domains or axes, uploaded
positions are interpreted as semantic data coordinates. Use `DVZ_VISUAL_COORD_VIEW` explicitly for
pre-normalized view-space/reference visuals.


## Aspect Ratio

Aspect ratio is a panel view/framing property, not a data-domain or controller-storage property.

| Value | Behavior |
|---|---|
| `DVZ_ASPECT_FREE` | X and Y scale independently |
| `DVZ_ASPECT_EQUAL` | zoom uses the same data-unit-per-pixel ratio for X and Y |

The constraint is resolved by the panel view resolver using the panel's source DATA domains and the
current plot rectangle. Controllers may expose gesture policy for preserving equal aspect during
navigation, but they do not own viewport aspect or rewrite panel domains.


## Visual Attachment

`DvzVisualAttachDesc` declares controller application, draw order, coordinate interpretation, and
optional render rectangle routing. The future attachment contract may also declare optional domain
overrides.

| Field | Rule |
|---|---|
| `coord_space` | `DVZ_VISUAL_COORD_VIEW`, `DVZ_VISUAL_COORD_DATA`, `DVZ_VISUAL_COORD_PANEL`, or `DVZ_VISUAL_COORD_PANEL_PIXEL` |
| `clip_rect` | `DVZ_VISUAL_CLIP_AUTO`, `DVZ_VISUAL_CLIP_PANEL`, or `DVZ_VISUAL_CLIP_PLOT` |
| `viewport_rect` | `DVZ_VISUAL_VIEWPORT_AUTO`, `DVZ_VISUAL_VIEWPORT_PANEL`, `DVZ_VISUAL_VIEWPORT_PLOT`, or `DVZ_VISUAL_VIEWPORT_TARGET` |
| `coord_transform` / `transform_params` | future field: optional pre-normalization transform |
| `controller_mode` | `DVZ_CONTROLLER_APPLY`, `DVZ_CONTROLLER_FIXED`, or `DVZ_CONTROLLER_APPLY_VIEW_PROJ` |
| `domain_x/y/z` | future field: `NULL` uses panel domain; non-`NULL` overrides that dimension |
| `z_layer` | signed draw order; lower draws first; same layer uses insertion order |

Coordinate-space meanings:

| Value | Meaning |
|---|---|
| `DVZ_VISUAL_COORD_VIEW` | metric panel view coordinates, affected by equal-aspect panel view fit |
| `DVZ_VISUAL_COORD_DATA` | mapped through panel DATA domains and the resolved DATA-to-VIEW model |
| `DVZ_VISUAL_COORD_PANEL` | normalized panel coordinates over the full panel rectangle, intentionally viewport-shaped |
| `DVZ_VISUAL_COORD_PANEL_PIXEL` | panel-local logical pixels with a top-left origin, positive X right, and positive Y down |

`coord_space` and `controller_mode` are independent. Typical combinations include
default `DATA+APPLY` for data visuals, `PANEL+FIXED` for normalized static panel overlays,
`PANEL_PIXEL+FIXED` for generated text and pixel-sized adornments, and
`VIEW+APPLY_VIEW_PROJ` for reference aids that follow camera view/projection but ignore
controller/object model transforms. Panel geometry is resolved into the panel-pixel attachment MVP
at frame-plan time; pixel-authored visual resources do not need regeneration when the panel moves
or resizes.

`clip_rect=AUTO` and `viewport_rect=AUTO` preserve the default routing policy: generated visual
roles use their explicit role policy, panel-clipped visual families use panel rectangles, and normal
DATA/VIEW visuals use plot rectangles. Non-AUTO values override those inferred defaults for custom
overlays, composites, and generated helpers that do not need a dedicated generated-visual role.


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
