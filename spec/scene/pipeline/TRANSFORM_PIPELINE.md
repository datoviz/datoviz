# Scene Transform Pipeline

This document defines how coordinate transforms should work in the future scene layer.

It exists to make explicit a boundary that is already implied elsewhere in the scene spec:

1. data-space normalization is a scene concern,
2. panel navigation and camera transforms are panel concerns,
3. DRP2 should not need to understand scientific coordinate semantics.


## Purpose

The transform pipeline should:

1. keep scientific data coordinates visible at the scene level,
2. make normalization into visual-ready coordinates explicit,
3. separate data normalization from panel-local navigation,
4. support multiple panels viewing the same visual differently,
5. work for both 2D panzoom and 3D camera families.


## Core Rule

The scene layer should distinguish at least two transform stages:

1. data-to-visual normalization,
2. visual-to-panel viewing transform.

Those stages are not the same and should not be collapsed into a single vague “transform”.


## Why This Matters

In a scientific visualization application, the usual flow is:

1. the user defines data in a domain-specific coordinate system,
2. that data is normalized into a visual-ready coordinate space,
3. the normalized data is uploaded,
4. panel-local transforms such as panzoom or 3D camera movement are applied later.

This split is important because:

1. data normalization often changes rarely,
2. panel navigation may change every frame,
3. the same normalized visual data may be reused across many panels,
4. backend runtimes should not be asked to understand domain coordinate systems.


## Coordinate Spaces

The scene spec should recognize the following conceptual spaces.


### 1. Data Space

`DataSpace` is the user or domain coordinate system.

Examples:

1. measurement coordinates,
2. voxel coordinates,
3. geographic coordinates,
4. simulation coordinates,
5. abstract plot coordinates.

This is where the user’s data semantically lives.


### 2. Visual Space

`VisualSpace` is the visual-family-ready coordinate space derived from data space.

This is the coordinate space that a visual family expects for its primary renderable geometry or item
positions before panel-local navigation is applied.

In many current Datoviz-style cases, this will often be normalized around `[-1, 1]`, but the key
contract is semantic, not numeric:

1. it is visual-ready,
2. it is family-aware,
3. it is shared across panels unless a family explicitly needs panel-local derivation.


### 3. Panel Space

`PanelSpace` is the coordinate space after panel-local navigation or viewing transforms.

Examples:

1. 2D panzoom-adjusted coordinates,
2. 3D camera/view-adjusted coordinates,
3. panel-specific framing of the same shared visual data.

This is where one panel may differ from another even when they display the same visual.


### 4. Clip/NDC Space

`ClipSpace` or `NDC` is the final render-facing normalized device coordinate space.

This is not a scene-semantic space.
It is the final render-space result of panel-local view/projection logic.


## Two Main Stages


### Stage A: Data-To-Visual Normalization

This stage transforms data from `DataSpace` into `VisualSpace`.

This is typically where:

1. data ranges are normalized,
2. plot domains are mapped into visual-ready extents,
3. data-axis conventions are resolved,
4. visual-family-specific spatial preparation happens.

This stage is primarily a scene concern.

It should usually be:

1. CPU-side by default,
2. cached,
3. recomputed when data or normalization policy changes,
4. represented as scene resources rather than hidden shader behavior.


### Stage B: Visual-To-Panel Viewing Transform

This stage transforms `VisualSpace` into `PanelSpace` and finally into `ClipSpace`/`NDC`.

This is where:

1. 2D panzoom acts,
2. 3D camera/view/projection acts,
3. panel-local framing and navigation happen.

This stage is panel-local and typically dynamic.

It should usually be:

1. recomputed frequently,
2. independent from data normalization,
3. reusable across many visuals shown in the same panel.


## Ownership Boundary

The preferred ownership split is:

1. `Scene` owns data-space semantics and normalization policy,
2. `Visual` declares the transform inputs it needs,
3. `Resource` stores normalized or source data as needed,
4. `Panel` owns panzoom/camera state,
5. `FramePlan` consumes both normalized visual data and panel-local transform state.


## Caching Rules

The transform pipeline should support different invalidation behavior for different stages.


### Data Normalization Cache

Data-to-visual normalization should usually be recomputed when:

1. source data changes,
2. domain bounds change,
3. normalization policy changes,
4. a visual-family-specific interpretation changes.

It should usually not be recomputed when:

1. the user pans in 2D,
2. the user zooms in 2D,
3. the user moves a 3D camera,
4. another panel views the same visual differently.


### Panel Transform Cache

Panel-local transforms should usually be recomputed when:

1. panzoom changes,
2. camera state changes,
3. viewport size changes,
4. panel-local framing changes.

They should usually not force re-normalization of the underlying data resources.


## Relationship To Resources

The resource model should support this split directly.

Typical pattern:

1. source data lives in scene resources in `DataSpace`,
2. normalized visual-ready data lives in scene resources in `VisualSpace`,
3. panel-local transforms are separate panel-derived state or parameter resources,
4. final render-facing transforms are emitted during planning or draw contribution assembly.

This means that the scene resource system should be comfortable holding both:

1. source semantic data,
2. derived visual-ready data.


## Relationship To `FramePlan`

`FramePlan` is where the two transform stages come together for a given frame.

The expected order is:

1. source data or normalization policy changes are resolved first,
2. normalized visual-ready resources are updated if needed,
3. panel-local transforms are updated,
4. visual contributions are assembled using normalized resources plus current panel transforms,
5. DRP2 emission follows from that planned result.

`FramePlan` should not need to rediscover domain semantics.
It should consume already-decided normalized resources and current panel-local transform state.


## 2D Panels

For 2D panels, the normal model is:

1. data in `DataSpace`,
2. normalization into a visual-ready 2D `VisualSpace`,
3. panzoom maps that visual space into panel view,
4. final projection produces render-facing coordinates.

The key rule is:

1. changing pan or zoom should usually not require rebuilding normalized visual resources.


## 3D Panels

For 3D panels, the normal model is:

1. data in `DataSpace`,
2. normalization into a visual-ready 3D `VisualSpace`,
3. camera/view/projection maps that visual space into panel view,
4. final projection produces render-facing coordinates.

Again, the key rule is:

1. camera movement should usually not require rebuilding normalized visual resources.


## Family-Specific Notes


### `pixel`, `point`, `marker`, `segment`

These families usually want:

1. source positions in `DataSpace`,
2. normalization into visual-ready item positions,
3. panel-local viewing on top of those normalized positions.


### `path`

`path` typically needs:

1. grouped source coordinates in `DataSpace`,
2. grouped normalization into visual-ready path coordinates,
3. panel-local navigation applied later.


### `glyph`

`glyph` is slightly more complex because:

1. layout semantics may be family-specific,
2. some placement may be derived after grouping,
3. panel-local viewing still remains a separate stage.

Even here, the principle remains:

1. family layout and normalization come before panel navigation.


### `image`

`image` typically combines:

1. sampled field content,
2. image placement in visual-ready coordinates,
3. panel-local viewing.

For `volume.render_mode = slice` backed by volumetric sampling:

1. the sampling source may be volumetric,
2. the resulting slice remains part of the `volume` family,
3. the panel-local transform stage stays separate from the volumetric sampling semantics.


### `mesh`

`mesh` typically wants:

1. source geometry in data coordinates,
2. normalized visual-ready geometry,
3. panel-local camera transforms applied later.


### `sphere`

For `sphere`, impostor-first semantics fit this model well:

1. sphere centers and radii may originate in `DataSpace`,
2. they are normalized into visual-ready sphere data,
3. panel-local transforms are applied later,
4. variant choice between impostor and mesh-backed path should not change the high-level transform
   pipeline.


### `volume`

`volume` typically involves:

1. volumetric data in source domain coordinates,
2. a visual-ready volume framing in `VisualSpace`,
3. panel-local camera/view transforms on top of that framing.

The family may still have richer internal traversal logic, but that should not collapse the distinction
between normalization and panel-local viewing.


## CPU Versus GPU Boundary

The default scene-side preference should be:

1. perform domain normalization on the CPU when practical,
2. upload normalized visual-ready data as scene resources,
3. use GPU-side transforms primarily for panel-local viewing and family render logic.

Exceptions are allowed when:

1. the data volume is too large,
2. a family explicitly benefits from compute-assisted preparation,
3. capability-driven fallback forces a different path.

But the semantic boundary should remain the same even when implementation shifts.


## What DRP2 Should See

DRP2 should generally see:

1. normalized visual-ready resources,
2. panel-local transform state,
3. final render work implied by the visual family and current panel state.

DRP2 should not need to know:

1. the original scientific coordinate system,
2. the domain normalization policy,
3. why some visual-ready data ended up in `[-1, 1]` or another normalized range.


## Panel Domain

The panel owns a data-space domain per dimension.
This domain is the single source of truth for normalization from `DataSpace` to `VisualSpace`
for all visuals in the panel that use `DVZ_COORD_DATA` without a per-visual domain override.

`DvzDataDomain` carries three fields:

| Field | Description |
|---|---|
| `min` | lower bound in data-space units; may be greater than `max` for an inverted axis |
| `max` | upper bound in data-space units |
| `scale` | `DVZ_SCALE_LINEAR` (default) or `DVZ_SCALE_LOG` |

### Domain Source Modes

1. **Explicit** — the user declares the domain directly.
   Normalization is fixed against it.
   Axis ticks are generated from this domain.

   ```text
   dvz_panel_set_domain(panel, DVZ_DIM_X, &(DvzDataDomain){.min=0, .max=10, .scale=DVZ_SCALE_LINEAR})
   dvz_panel_set_domain(panel, DVZ_DIM_Y, &(DvzDataDomain){.min=0, .max=1})
   ```

2. **Fit-to-data** — a one-time scan of the data extents of all visuals currently attached to the
   panel sets the domain, after which it behaves as explicit.
   It is not a live binding.

   ```text
   dvz_panel_fit_domain(panel, DVZ_DIM_X)
   dvz_panel_fit_domain(panel, DVZ_DIM_Y)
   ```

3. **Pass-through (default)** — no domain is declared.
   Visual data is treated as already in `VisualSpace`; no normalization is applied.
   Axes have no `DataSpace` domain and may suppress tick labels or show normalized coordinates.
   This preserves v0.3 behavior for callers who pre-normalize their data.

### Log Scale

When `scale = DVZ_SCALE_LOG`, values are mapped through `log10` before linear interpolation to
`VisualSpace`.
Negative or zero values in a log-scale domain are undefined and should produce a diagnostic.
Axis tick values are chosen at decade boundaries or suitable log-spaced positions.

### Inverted Axis

Setting `min > max` inverts the axis: the minimum data value maps to the top or right of the
panel in `VisualSpace`.
This is standard in image analysis and medical imaging where y = 0 is at the top.

```text
dvz_panel_set_domain(panel, DVZ_DIM_Y, &(DvzDataDomain){.min=480, .max=0})  // y=0 at top
```

The axis tick direction follows the declared order: ticks increase from `min` toward `max` in
`VisualSpace`.

### User Constraint

All visuals attached with `DVZ_COORD_DATA` and no per-visual domain override must express their
positions in the same data-space coordinate system as the panel domain.
The scene does not detect or reconcile mismatched coordinate systems.


## Coordinate Transform Stage

Before panel-domain normalization, an optional per-visual coordinate transform converts the
visual's `DataSpace` into a Cartesian `DataSpace` that the domain normalization can handle.

The full pipeline for a visual with a coordinate transform is:

```text
DataSpace (user coords) → [coord_transform] → Cartesian DataSpace → [domain normalization] → VisualSpace
                                                                  → [controller] → PanelSpace → NDC
```

The transform is declared on the attachment descriptor alongside `coord_space`:

```text
dvz_panel_add_visual(panel, visual, &(DvzVisualAttachDesc){
    .coord_space      = DVZ_COORD_DATA,
    .coord_transform  = DVZ_TRANSFORM_POLAR,
    .transform_params = {},    // optional per-transform parameters
    ...
})
```

### Named Transforms

| Value | Input | Output | Notes |
|---|---|---|---|
| `DVZ_TRANSFORM_NONE` | any Cartesian | unchanged | default |
| `DVZ_TRANSFORM_POLAR` | `(r, θ)` | `(x, y)` | θ in radians |
| `DVZ_TRANSFORM_SPHERICAL` | `(r, θ, φ)` | `(x, y, z)` | physics convention: θ polar, φ azimuthal |
| `DVZ_TRANSFORM_GEO_MERCATOR` | `(lon, lat)` | `(x, y)` | Web Mercator (EPSG:3857) by default |
| `DVZ_TRANSFORM_GEO_GLOBE` | `(lon, lat, alt)` | `(x, y, z)` | on unit sphere; alt scales radially |

All angles are in radians unless a `DvzTransformParams` field overrides the convention.

### Transform Parameters

`DvzTransformParams` carries optional per-transform configuration:

| Field | Used by | Description |
|---|---|---|
| `sphere_radius` | `GEO_GLOBE` | radius of the reference sphere; default `1.0` |
| `center_lon` | `GEO_MERCATOR` | projection center longitude; default `0.0` |
| `center_lat` | `GEO_MERCATOR` | reference latitude for scale; default `0.0` |
| `angles_degrees` | `POLAR`, `SPHERICAL`, `GEO_*` | if true, angles are in degrees |

Zero-valued fields use defaults; the entire struct may be zero-initialized for defaults.

### Coordinate Transform And Panel Domain

The panel domain always refers to the **post-transform Cartesian** space, not the original user
coordinate space.

For `DVZ_TRANSFORM_POLAR` the panel domain is declared in Cartesian `(x, y)` units.
For `DVZ_TRANSFORM_GEO_MERCATOR` the panel domain is declared in projected map units.
For `DVZ_TRANSFORM_GEO_GLOBE` the panel domain is typically not used for normalization — the
globe controller manages the view directly and data lands on the unit sphere.

### Polar Notes

`DVZ_TRANSFORM_POLAR` converts `(r, θ)` to `(x, y)` before normalization.
Axes in a polar panel should show radial and angular gridlines rather than Cartesian ticks.
Polar axis geometry (circular gridlines, radial labels) is deferred; for the initial spec a
polar panel may suppress default axes or show Cartesian axes over the normalized Cartesian
space.

### Geographic Notes

`DVZ_TRANSFORM_GEO_MERCATOR` maps longitude and latitude to 2D projected coordinates.
Standard Web Mercator is the default; the Mercator projection diverges near the poles
(lat ≥ ~85.05°).

`DVZ_TRANSFORM_GEO_GLOBE` maps longitude, latitude, and altitude to a point on or above a
sphere surface.
Altitude `0` lands exactly on the sphere; positive altitude scales radially outward.
The natural paired controller is `GlobeController` (see `CONTROLLERS.md`).

Tiled satellite or street-map imagery for geographic visualization is a data-pipeline concern
(tile loading, LOD management, cache) and is deferred.
The `image` family must support efficient per-tile partial updates to enable it.

Geographic axes (graticule lines, lon/lat tick formatting) are deferred.


## Aspect Ratio Policy

Aspect ratio is a panzoom controller property, not a normalization property.

```text
panzoom = dvz_panzoom(scene, 0)
dvz_panzoom_set_aspect(panzoom, DVZ_ASPECT_EQUAL)
```

| Value | Behavior |
|---|---|
| `DVZ_ASPECT_FREE` | X and Y scale independently (default) |
| `DVZ_ASPECT_EQUAL` | zoom constrains X and Y to the same data-unit-per-pixel ratio |

When `DVZ_ASPECT_EQUAL` is active, any zoom gesture that would scale X and Y differently is
adjusted so both dimensions use the same scale factor.
Pan is unconstrained.

The constraint is applied after normalization — the controller operates on `VisualSpace`, not
`DataSpace`.
This means equal aspect is expressed in data units only when the X and Y panel domains cover
the same physical extent; if they differ, the user should account for that in the domain
declaration.


## Visual Attachment And Coordinate Space

When attaching a visual to a panel, a `DvzVisualAttachDesc` declares the coordinate space of
the visual's position data and whether the panel controller applies to it.

```text
dvz_panel_add_visual(panel, visual, &(DvzVisualAttachDesc){
    .coord_space      = DVZ_COORD_DATA,
    .coord_transform  = DVZ_TRANSFORM_NONE,   // see Coordinate Transform Stage
    .transform_params = {},
    .controller_mode  = DVZ_CONTROLLER_APPLY,
    .domain_x         = NULL,                 // NULL = use panel domain
    .domain_y         = NULL,
    .domain_z         = NULL,
    .z_layer          = 0,                    // see Z-Layer below
})
```

### Coordinate Space

| Value | Description |
|---|---|
| `DVZ_COORD_DATA` | `DataSpace` — normalized via panel domain (or per-visual override) |
| `DVZ_COORD_NDC` | pre-normalized to `[-1, 1]` — no panel-domain mapping applied |
| `DVZ_COORD_PIXEL` | pixel coordinates — converted to NDC using the current panel pixel size |

`DVZ_COORD_DATA` is the default when the panel has an explicit domain.
`DVZ_COORD_NDC` is appropriate for pre-normalized data and for fixed overlay elements.
`DVZ_COORD_PIXEL` is appropriate for screen-space decorations such as scale bars and fixed
annotations.

### Controller Mode

| Value | Description |
|---|---|
| `DVZ_CONTROLLER_APPLY` | panzoom or camera applies to this visual (default) |
| `DVZ_CONTROLLER_FIXED` | visual is unaffected by navigation — stays in place |

`coord_space` and `controller_mode` are independent.
All four combinations are valid:

| Combination | Typical use |
|---|---|
| `DATA + APPLY` | scatter, path, image data (standard case) |
| `DATA + FIXED` | a reference marker at a fixed data position, no pan/zoom |
| `NDC + APPLY` | a visual in NDC that should still pan with the scene |
| `NDC + FIXED` | crosshair, border, or static NDC overlay |
| `PIXEL + FIXED` | scale bar, pixel-exact annotation, legend at panel corner |
| `PIXEL + APPLY` | rare; pixel-space element that follows camera — unusual but not forbidden |

### Z-Layer

`z_layer` is a signed integer that controls draw order within a panel.
Visuals are drawn in ascending `z_layer` order — lower values draw first (behind), higher values
draw last (in front).
Default is `0`. Negative values are valid and useful for explicit background visuals.

```text
dvz_panel_add_visual(panel, background, &(DvzVisualAttachDesc){.z_layer = -1})
dvz_panel_add_visual(panel, data,       &(DvzVisualAttachDesc){.z_layer =  0})
dvz_panel_add_visual(panel, overlay,    &(DvzVisualAttachDesc){.z_layer =  1})
```

Visuals at the same `z_layer` are drawn in insertion order (the order they were added to the
panel via `dvz_panel_add_visual`). This is deterministic and predictable without requiring
explicit z values for every visual.

A separate `visible` flag handles show/hide without changing `z_layer`.


### Per-Visual Domain Override

`domain_x`, `domain_y`, `domain_z` allow a visual to use a different normalization domain
than the panel default on specific dimensions.
`NULL` means use the panel domain.

This enables dual-axis and mixed-space overlay patterns described in the next section.


## Dual-Axis And Mixed-Space Overlays

Two patterns in scientific visualization require per-visual domain overrides.


### Dual Axis (twinx / twiny)

Two visuals in the same panel share one axis but have independent normalization on the other.

```text
// Panel Y domain: temperature 0–100 °C
dvz_panel_set_domain(panel, DVZ_DIM_Y, &(DvzDataDomain){.min=0, .max=100})

// Primary visual uses the panel domain
dvz_panel_add_visual(panel, temp_visual, &(DvzVisualAttachDesc){
    .coord_space = DVZ_COORD_DATA,
})

// Secondary visual: pressure 900–1100 hPa — per-visual Y override
DvzDataDomain pressure_y = {.min=900, .max=1100}
dvz_panel_add_visual(panel, pressure_visual, &(DvzVisualAttachDesc){
    .coord_space = DVZ_COORD_DATA,
    .domain_y    = &pressure_y,
})
```

A secondary Y axis is declared separately and attached to `pressure_y` rather than the panel
domain.
Panzoom applies to both visuals' `VisualSpace` positions.
With `DVZ_ASPECT_FREE` (default), zooming X does not couple to Y — each visual's Y
normalization remains independent.

The scene treats the two visuals as occupying the same `VisualSpace`; it does not know that
their Y domains represent different physical quantities.
The user is responsible for the semantic consistency of the combined view.


### Mixed-Space Overlays

Two visuals in the same panel use different `DataSpace` coordinate systems.

```text
// Panel domain: voxel space
dvz_panel_set_domain(panel, DVZ_DIM_X, &(DvzDataDomain){.min=0, .max=256})
dvz_panel_set_domain(panel, DVZ_DIM_Y, &(DvzDataDomain){.min=0, .max=256})

// Atlas image in voxel coordinates — uses panel domain
dvz_panel_add_visual(panel, atlas, &(DvzVisualAttachDesc){
    .coord_space = DVZ_COORD_DATA,
})

// Electrode positions in physical mm — per-visual override on both dimensions
DvzDataDomain mm_x = {.min=-10, .max=10}
DvzDataDomain mm_y = {.min=-10, .max=10}
dvz_panel_add_visual(panel, electrodes, &(DvzVisualAttachDesc){
    .coord_space = DVZ_COORD_DATA,
    .domain_x    = &mm_x,
    .domain_y    = &mm_y,
})
```

The scene normalizes each visual independently.
It has no knowledge of the physical relationship between voxels and mm.
Aligning the two coordinate systems correctly is the caller's responsibility.


## CPU Precision Policy

All Stage A (data-to-visual normalization) work must be performed in **64-bit floating point
(F64)** on the CPU.

F32 downcast happens exactly once: at `UploadNode` time in the `FramePlan`, after normalization
to `VisualSpace`.
The GPU receives F32 data whose values are already near-zero or in a well-conditioned range.

This is the standard solution to the **large world coordinates (LWC)** problem, known in game
engine literature as "floating origin" or "camera-relative rendering": by computing all
arithmetic in F64 and subtracting the reference origin before downcasting, precision loss at
high zoom or far from the coordinate origin is eliminated.
Without it, floating-point cancellation at large coordinate values produces visible positional
jitter — sometimes called "precision swimming" — as the user zooms in.

### Why F32 On The GPU Is Fine After Normalization

After Stage A, all position values are mapped into `VisualSpace`, typically `[-1, 1]` or
another bounded range.
F32 provides ~7 decimal digits of precision over that range, which is more than sufficient for
rendering.
The cancellation error only arises when large coordinates are differenced in the shader —
which never happens if normalization was done correctly in F64 first.

### GPU-Side F64 Emulation

For cases where the GPU itself must operate on high-precision un-normalized coordinates
(e.g., compute shaders doing distance queries on raw astronomical or geospatial data), the
**Olano-Greer double-single technique** encodes each F64 value as two F32 values and emulates
double arithmetic in the shader.
This is a niche path and is not needed for standard rendering once CPU normalization is correct.
It is reserved as a future compute path and is not part of the v0.4 baseline.

### Data Ingestion

The scene accepts position data as F64 arrays natively.
Python/NumPy data is F64 by default; the scene must not silently downcast at ingestion time.
F32 source data is also accepted and passes through without promotion.

### Geometry Utility Precision

All geometry utility operations (triangulation, curve tessellation, line simplification, hull
computation, boolean polygon operations) also operate in F64 throughout.
Their output is F64 vertex data that enters the normal upload path and is downcast at
`UploadNode` time.
See `GEOMETRY_UTILITIES.md` for the full geometry utility specification.


## Rules

1. Data normalization belongs above DRP2.
2. Panel-local navigation belongs below family semantics but above DRP2 emission.
3. The same normalized visual data should be reusable across multiple panels whenever practical.
4. Panzoom or camera changes should usually not invalidate normalized visual resources.
5. Source-data changes may invalidate normalized resources, but need not invalidate panel-local state.
6. Family semantics should describe transform needs without leaking backend matrix or handle types.
7. All visuals using `DVZ_COORD_DATA` without a per-visual domain override share the panel domain
   for that dimension.
8. Per-visual domain overrides apply only to normalization — they do not change which controller
   applies or how `VisualSpace` is shared.
9. Log-scale and inverted-axis behavior are normalization-side policies declared on `DvzDataDomain`,
   not controller or shader concerns.
10. Aspect ratio is a controller-side policy; it does not change the panel domain or normalization.
11. All CPU-side normalization and geometry utility operations run in F64; F32 downcast happens
    only at `UploadNode` time after normalization is complete.


## Follow-On Spec Work

This document should eventually be connected to:

1. worked examples showing data-to-visual-to-panel flow for several families,
2. family-specific notes where transform behavior is especially distinctive.
