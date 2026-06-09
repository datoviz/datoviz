# v0.4 Interaction And Showcase Plan

Status: handoff plan. Updated: 2026-06-09.

This plan records the v0.4 interaction and showcase work agreed after the broader examples/API
refactor plan. It is intended for a future agent to execute end to end.


## Baseline Assumptions

Before starting this plan, assume
[`API_AND_EXAMPLES_REFACTOR_PLAN.md`](API_AND_EXAMPLES_REFACTOR_PLAN.md) has already been
implemented. If that is not true, stop and finish or rebase onto that work first.

Expected baseline:

1. public C examples use the v0.4 taxonomy:
   `examples/c/visuals/`, `examples/c/features/`, `examples/c/composites/`,
   `examples/c/showcases/`, `examples/c/advanced/`, and `examples/c/lab/`;
2. examples use public scene/app APIs rather than v0.3 compatibility shims;
3. descriptor conventions, view sizing, device scale, user scale, and scenario-runner scale fields
   are settled;
4. weak picking examples have been merged or renamed into a strong public picking feature example;
5. common geometry builders, especially cube, sphere, arrow/gizmo, plane, and surface-grid helpers,
   live in public `geom` APIs when they are used by public examples;
6. example manifests, smoke tests, and browser route metadata follow the refactor plan's naming and
   taxonomy.

Do not put large datasets, generated binary payloads, downloaded archives, or derived showcase
artifacts in git. In particular, keep raw data outside the repository or in explicitly ignored
cache locations. Do not stage or commit the `data` submodule unless the user explicitly approves it
in the current turn.


## Goals

1. Make v0.4 interaction feel complete for point-like, pixel, sphere, and instanced mesh items.
2. Add public scene aids that make 3D scientific scenes easier to read: orientation gizmo and
   reference grid.
3. Add three polished v0.4 showcases: embeddings, lipid brain atlas, and animated mouse.
4. Keep browser parity as a release-candidate stretch lane, not a blocker unless time allows.
5. Defer box/lasso selection and mesh face selection to v0.5.


## Public API Decisions

### Generic Widget Placement

Do not create gizmo-specific positioning fields.

Use a generic placement object for panel/figure-attached widgets. Prefer promoting or refining
`DvzPlacement` if it is already suitable. Only introduce a new `DvzWidgetPlacement` if the existing
type is too colorbar/legend-specific to make generic without churn.

The orientation gizmo descriptor should contain placement as one field:

```c
DvzOrientationGizmoDesc desc = dvz_orientation_gizmo_desc();
desc.placement = dvz_placement_panel_corner(
    DVZ_HORIZONTAL_ANCHOR_RIGHT,
    DVZ_VERTICAL_ANCHOR_BOTTOM,
    150, 150,
    -16, -16);
```

If helper constructors are not available in the first implementation, direct fields are acceptable:

```c
desc.placement.space = DVZ_PLACEMENT_SPACE_PANEL;
desc.placement.horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT;
desc.placement.vertical_anchor = DVZ_VERTICAL_ANCHOR_BOTTOM;
desc.placement.width_px = 150;
desc.placement.height_px = 150;
desc.placement.offset_x_px = -16;
desc.placement.offset_y_px = -16;
```

Keep placement semantics generic:

1. attachment space: panel first; figure later if cheap;
2. horizontal/vertical anchor: left, center, right and top, center, bottom;
3. fixed pixel size for v0.4 widgets;
4. pixel offsets or padding from the anchor;
5. predictable behavior under device scale and user scale.

The gizmo descriptor should own only gizmo semantics: source controller, axes/rings visibility,
style, size defaults, and placement.


### Reference Grid

Add a public reference grid helper, not a floor-only helper.

Preferred C shape:

```c
DvzReferenceGridDesc desc = dvz_reference_grid_desc();
desc.plane = DVZ_REFERENCE_GRID_XZ;
desc.origin[1] = -0.62f;
desc.size[0] = 4.0f;
desc.size[1] = 4.0f;
desc.spacing = 0.25f;
desc.major_every = 4;

DvzReferenceGrid* grid = dvz_reference_grid(panel, &desc);
```

Recommended descriptor fields:

1. `plane`: `DVZ_REFERENCE_GRID_XY`, `DVZ_REFERENCE_GRID_XZ`, `DVZ_REFERENCE_GRID_YZ`,
   `DVZ_REFERENCE_GRID_CUSTOM`;
2. `origin`, `axis_u`, `axis_v` for custom placement;
3. `size[2]`, `spacing`, `major_every`;
4. minor, major, and axis colors;
5. line widths;
6. visibility toggles for minor lines, major lines, and axes;
7. depth-test policy.

Implementation should lower to retained line/segment/path visuals. Do not create a new renderer
path.


### Instance-Level Mesh Interaction

For v0.4, object-level mesh interaction on instanced meshes must resolve the instance, not the
whole visual.

The required model is:

```text
one mesh geometry
N instance_transform records
N selectable instance ids
N item_state values
```

Click and hover should operate on `visual_id + instance_id`. Non-instanced meshes are treated as a
single implicit selectable instance. Face, triangle, vertex, or region selection is deferred.

Required implementation pieces:

1. mesh query returns a stable `instance_id`;
2. mesh visual accepts per-instance `item_state` when `instance_transform` is present;
3. mesh shader applies hover/selection styling per instance;
4. selection state stores and readbacks carry the instance identity;
5. empty click clears selection according to the standard selection controller behavior;
6. tests prove query identity and item-state upload agree on instance indices.

Do not let object-level selection on one instance mean "select the entire visual".


## v0.4 Feature Work

### Orientation Gizmo

Add a public v0.4 orientation gizmo feature using the existing legacy example as reference material,
not as the final API.

Deliverables:

1. public API: `DvzOrientationGizmo`, descriptor, create/destroy, visibility update;
2. placement via generic `DvzPlacement`;
3. passive axis triad linked to a panel controller's rotation;
4. overlay/inset rendering that cannot be obscured by scene depth;
5. example: `examples/c/features/orientation_gizmo.c`;
6. smoke/screenshot validation.

Implementation notes:

1. start from the old arcball gizmo geometry and controller-link logic if useful;
2. prefer public `geom` builders for arrows/rings once available;
3. keep transform-gizmo/manipulator behavior out of v0.4;
4. support arcball first; turntable can follow if the controller-link model is already generic.


### Reference Grid

Add a public reference-grid scene aid.

Deliverables:

1. public API and descriptor;
2. example: `examples/c/features/reference_grid.c`;
3. use it in 3D showcases where helpful, especially the mouse showcase;
4. screenshot validation in an arcball scene.

The old v0.3 Python `horizontal_grid(elevation=...)` behavior is provenance only. The v0.4 API
should be plane-oriented and suitable for XY, XZ, YZ, and custom planes.


### Pixel Hover And Selection

Add full-feature pixel interaction.

Deliverables:

1. query path for pixel hover and click selection;
2. retained `item_state` update for pixels;
3. shader feedback for hover and selected pixels;
4. public example: `examples/c/features/selection_pixel.c`, unless the refactor plan's final
   naming prefers extending `features/picking.c`;
5. tests for pixel query identity, empty hit behavior, and item-state update.

User-facing behavior:

1. hover highlights the current pixel/cell;
2. click toggles or replaces selected pixels according to the selection mode;
3. background click clears;
4. optional overlay readout shows pixel index and value.


### Sphere Hover And Selection

Add item-level sphere interaction if the native sphere query path is already reliable.

Deliverables:

1. sphere query identity wired to hover and selection;
2. retained per-sphere `item_state`;
3. shader feedback via tint, alpha, and scale where applicable;
4. public example: `examples/c/features/selection_sphere.c`;
5. tests for sphere hit/miss and selected-state upload.

Do not block v0.4 on advanced sphere outlines. A simple selected tint/scale is sufficient.


### Instanced Mesh Hover And Selection

Add instance-level mesh selection as a required v0.4 feature.

Deliverables:

1. query result carries `instance_id`;
2. mesh item-state attribute/storage is per instance when instancing is enabled;
3. selected and hovered instance styling works without duplicating geometry;
4. public example: `examples/c/features/selection_mesh_instances.c`;
5. tests for at least:
   - hit returns the expected instance id,
   - hover state affects only one cube,
   - click selection toggles only one cube,
   - empty click clears,
   - non-instanced mesh remains selectable as one item.

Example scene:

1. one cube geometry;
2. a grid of cube instances;
3. per-instance base colors;
4. hover scale or tint;
5. click selection with retained state;
6. selected-count overlay if overlay/readout APIs are stable.


## Browser Parity Stretch Lane

Do this only if there is time before the release candidate.

Remaining work:

1. add promoted scenario routes for pixel, sphere, and mesh-instance interaction;
2. add or verify WGSL query shaders and decode paths for pixel, sphere, and mesh;
3. update WASM scenario host/readback plumbing beyond point, marker, and image;
4. add browser smoke tests and manifest classification;
5. update `docs/reference/webgpu-subset.md`.

Browser parity should not distort the native v0.4 interaction API. If browser support lags, mark the
affected examples as native-first or WebGPU-planned in metadata.


## Showcase Work

### Embedding Atlas

Add a v0.4 showcase for AI/ML audiences.

Recommended example:

```text
examples/c/showcases/embedding_atlas.c
```

Preprocessed data bundle:

1. `xy.f32`: 2D embedding coordinates;
2. `cluster.u16` or `cluster.u32`;
3. `color.rgba8`;
4. `metadata.jsonl`;
5. optional `neighbors.u32` for nearest-neighbor links.

Interaction:

1. dense point cloud with panzoom;
2. hover metadata readout;
3. click selection;
4. optional nearest-neighbor ring or link segments;
5. optional cluster legend/highlight if the legend/highlight API is stable.

Keep the v0.4 scope focused on interactive embedding exploration. Image-thumbnail LOD, text-label
LOD, semantic search, and dashboard-style side panels are v0.5.


### Lipid Brain Atlas

Include this in v0.4 as a polished visual/video showcase with light interaction.

Source dataset:

```text
/home/cyrille/Downloads/peaks.parquet
```

The raw Parquet file is very large. It may be moved out of `Downloads` into a durable local cache.
The preprocessing script must:

1. look for an existing cached copy first;
2. if `/home/cyrille/Downloads/peaks.parquet` exists and the cache copy does not, move or copy it
   to the cache location according to a command-line flag;
3. download from Zenodo if no local raw file is available;
4. never place the raw Parquet file in the git repository;
5. write compact render-ready artifacts only to an ignored/generated data directory.

Recommended default cache policy:

```text
$DVZ_DATASET_CACHE/lipid_brain_atlas/peaks.parquet
```

with fallback:

```text
$XDG_CACHE_HOME/datoviz/datasets/lipid_brain_atlas/peaks.parquet
```

or:

```text
~/.cache/datoviz/datasets/lipid_brain_atlas/peaks.parquet
```

Recommended preprocessing script:

```text
tools/data/prepare_lipid_brain_atlas.py
```

The script should extract a compact subset:

1. a few representative sections;
2. a small set of lipid or m/z channels;
3. optional categorical lipizone/region metadata if present;
4. min/max or percentile ranges for stable color mapping;
5. metadata needed for labels, colorbars, and scripted video sweeps.

Recommended example:

```text
examples/c/showcases/lipid_brain_atlas.c
```

Showcase behavior:

1. high-quality section/channel rendering;
2. colorbar and labels;
3. deterministic camera/panel framing;
4. scripted channel or section sweep for video;
5. minimal interaction only: channel/section controls and optional hover readout if cheap.

This showcase is primarily for images and videos, not for a heavy exploratory UI.


### Synthetic Animated Mouse

Include this in v0.4.

Source dataset:

```text
https://osf.io/h3ec5/
```

Datoviz should not load `.blend` files or evaluate Blender rigs at runtime. Use preprocessing to
export Datoviz-ready assets.

Recommended preprocessing script:

```text
tools/data/prepare_synthetic_mouse.py
```

Required preprocessed artifacts:

1. static mesh topology;
2. UV coordinates;
3. texture image;
4. normals;
5. baked vertex-position frames or another render-ready animation representation;
6. keypoint positions per frame;
7. skeleton edges;
8. trajectory source points, such as body center, nose, and paws.

Recommended example:

```text
examples/c/showcases/synthetic_mouse.c
```

Showcase behavior:

1. animated textured mesh;
2. reference grid/floor;
3. orientation gizmo;
4. trajectory trail;
5. current keypoint skeleton overlay;
6. fading keypoint skeleton trail;
7. optional ghost mesh poses.

Trail implementation:

1. trajectory trail uses path or segment visuals over recent frames;
2. keypoint skeleton trail uses repeated segment data with fading alpha over a short history window,
   for example 8 to 16 frames;
3. current skeleton remains visually distinct from the history trail;
4. ghost poses are experimental duplicates of older mesh frames with low alpha. Keep them only if
   they look good with depth and transparency.

The example should be designed for short attractive videos as well as interactive viewing.


## Standard Sci-Viz Scene Aids

### Already Covered Or Expected Before This Plan

The active v0.4 stack already has or is expected to have:

1. 2D axes, ticks, and grid lines;
2. colorbars and legends;
3. scale bars;
4. guide lines, spans, and bands;
5. annotations and readouts;
6. panel backgrounds;
7. panzoom, arcball, fly, and turntable controllers;
8. depth, alpha, lighting, EDL, SSAO, MSAA, and depth-cue examples.


### Add In v0.4

Add:

1. orientation gizmo;
2. reference grid/floor grid;
3. trajectory and skeleton trails in the mouse showcase;
4. compact overlay readouts where interaction examples need them;
5. optional bounding-box helper only if it naturally falls out of the reference-grid or annotation
   implementation.


### Defer To v0.5

Defer:

1. 3D rulers and measurement widgets;
2. box selection;
3. lasso selection;
4. mesh face/triangle/region selection;
5. arbitrary selected mesh outlines;
6. camera path authoring UI;
7. dynamic label LOD and collision handling;
8. generalized dashboard/overlay layout beyond the small placement slice.


## Implementation Order

1. Verify the examples/API refactor plan is already implemented and rebase this work onto its final
   names and directories.
2. Implement or promote generic widget placement.
3. Implement orientation gizmo and `features/orientation_gizmo.c`.
4. Implement reference grid and `features/reference_grid.c`.
5. Implement pixel hover/selection and its focused example.
6. Implement sphere hover/selection if the query path is ready.
7. Implement instance-level mesh hover/selection and `features/selection_mesh_instances.c`.
8. Add native tests for pixel, sphere, and mesh-instance query and item-state behavior.
9. Add `embedding_atlas` data preparation and showcase.
10. Add lipid brain atlas preprocessing and showcase.
11. Add synthetic mouse preprocessing and showcase.
12. Add browser parity only if release-candidate time remains.
13. Update example manifest/catalog metadata, docs, screenshots, and WebGPU subset docs as needed.
14. Run validation and `git diff --check`.


## Validation

Use the narrowest relevant loop while iterating, then run broader checks before handoff.

Required checks before finalizing code changes:

```sh
just build
just test
git diff --check
```

For each new public example:

1. build the example;
2. run it through the scenario/example runner if available;
3. capture at least one screenshot or deterministic smoke frame;
4. verify no required asset is missing from the documented cache path;
5. update manifest/catalog metadata;
6. classify browser status as `webgpu-live`, `webgpu-planned`, `webgpu-deferred`, or
   `native-only`.

For interaction features, add focused tests for:

1. hit identity;
2. miss behavior;
3. hover state;
4. click selection;
5. clear selection;
6. item-state upload dirtiness;
7. selection readback identity.


## Non-Goals For v0.4

Do not include:

1. preserving v0.3 APIs for compatibility;
2. lasso or rectangle selection;
3. mesh face/triangle/vertex picking;
4. Blender runtime loading, skinning, or rig evaluation;
5. image-thumbnail LOD for embeddings;
6. full dashboard side-panel UX;
7. glTF import, which is deferred to v0.5 asset-import work;
8. committing raw datasets or generated binary payloads.
