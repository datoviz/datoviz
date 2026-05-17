# Scene Visual Family Implementation Decisions

> **Execution Status**
> - **Status:** `ACTIVE IMPLEMENTATION CONTRACT`
> - **Updated on:** `2026-05-17`
> - **Purpose:** lock the first-slice implementation decisions for the pixel, point, marker,
>   segment, and path visual families so independent workers can proceed without making API or
>   semantic decisions mid-implementation.


## Scope

This note refines:

1. `spec/scene/visuals/PIXEL.md`
2. `spec/scene/visuals/POINT.md`
3. `spec/scene/visuals/MARKER.md`
4. `spec/scene/visuals/SEGMENT.md`
5. `spec/scene/visuals/PATH.md`
6. `agents/soon/SCENE_POINT_PIXEL_MARKER_PLAN.md`
7. `agents/soon/SCENE_VECTOR_VISUALS_PLAN.md`

It is authoritative for the first implementation pass when those documents disagree.


## Shared Rules

1. Keep all visuals on the active scene -> FramePlan -> DRP2 -> vklite/canvas path.
2. Do not create a parallel renderer, presentation path, Vulkan wrapper, or special app path.
3. Preserve the public `DvzVisual*` model; do not introduce public visual subtypes.
4. Keep first-slice attributes dense and explicit unless this note says otherwise.
5. Defer scalar color/size modes, `PER_GROUP`, `CONSTANT` source optimization, and data-space
   sizing unless explicitly included below.
6. Prefer GPU-backed picking because the request path already executes point/image readbacks through
   DRP2/runtime auxiliary streams.
7. Workers may commit after coherent slices once `git diff --check`, `just build`, and the narrow
   relevant tests pass.


## Current Landed Status

Status on 2026-05-17: the first implementation batch for the five families has landed.

Implemented:

1. `pixel`: dense `position`/`color`/`size`, GLSL native square points, WGSL instanced quads,
   offscreen nonblank smoke, and GPU square picking.
2. `point`: dense `position`/`color`/`size`, antialiased circular coverage, edge/stroke styling via
   `dvz_point_set_style()`, GLSL native point-coordinate rendering, WGSL instanced quads, and GPU
   circular picking.
3. `marker`: public `dvz_marker()`, code-SDF shapes `disc`, `square`, `triangle`, `diamond`,
   `cross`, and `ring`, dense `position`/`color`/`size`/`angle`/`shape`, marker style API, GLSL
   marker rendering, WGSL point-like lowering, and GPU bounding-box picking.
4. `segment`: public `dvz_segment()`, `position_start`/`position_end` endpoint attributes,
   per-item `line_width`, RGBA color, analytic GLSL stroke quads, non-arrow caps, and cap
   validation/API.
5. `path`: existing primitive line-strip path plus a stroked path lane when per-point `line_width`
   is present, open subpath lengths via `dvz_path_set_subpaths()`, and GLSL lowering through the
   segment stroke pipeline.

Validation recorded after the batch:

1. `just test app` passed `58/58`.
2. `just spec-check` passed all DRP2 fixture and pytest phases.
3. Full `just test` passed `563/563`.

Remaining work should start from the deferred lists below rather than re-opening these first-slice
decisions.


## Pixel

Implemented first slice:

1. Keep the current retained API shape: `position`, `color`, and `size` are dense attributes.
2. `size` remains a per-item attribute in the first slice, matching current implementation and tests.
3. Colors are RGBA only.
4. Sizes are screen-space pixels only.
5. No `shift`, scalar color, `PER_GROUP`, default-size optimization, or `size_space = data` yet.
6. GLSL and WGSL emission coverage is in place.
7. Runtime/offscreen smoke proves pixel draws nonblank square marks.
8. GPU-backed square picking is in place.

Deferred:

1. Constant/default size storage.
2. `shift`.
3. Scalar color and scale binding.
4. Grouped color.
5. Data-space pixel size.


## Point

Implemented first slice:

1. Keep current dense `position`, `color`, and `size` attributes.
2. `size` is the point diameter in screen pixels.
3. Colors are RGBA only.
4. Antialiased circular point rendering is in place.
5. Point edge/stroke styling is in place.
6. GPU-backed circular picking is in place.
7. EDL, depth-cue, alpha-mode, WBOIT, and depth-peel eligibility remain routed through the existing
   point visual pass-capability path.

Naming:

1. `color` remains the face/fill color attribute.
2. `edge_color` is the stroke/edge color.
3. `line_width` is the stroke width in screen pixels.
4. Use marker-compatible style aspects where needed: `filled`, `stroke`, and `outline`.

Backend lowering:

1. Vulkan/GLSL may keep native point-list lowering transparently.
2. GLSL point shaders must use `gl_PointCoord` for circular coverage and edge/stroke styling.
3. WebGPU/WGSL continues to lower point items to instanced quads.
4. If GLSL/WGSL visual parity becomes fragile, a later slice may move GLSL to instanced quads too.

Deferred:

1. `shift`.
2. Scalar color/size.
3. `PER_GROUP`.
4. Constant source lowering.
5. `size_space = data`.


## Marker

Implemented first slice:

1. `marker` is implemented as code-SDF only.
2. Public marker enums and `dvz_marker()` are in place.
3. `DvzMarkerStyle` and `dvz_marker_set_style()` are in place.
4. Initial built-in shapes are `disc`, `square`, `triangle`, `diamond`, `cross`, and `ring`.
5. Use v0.3 marker GLSL SDF code as design prior art and port selectively into the v0.4 shader
   registry and runtime path.
6. Attributes: `position`, `color`, `size`, and `angle`.
7. Optional per-item shape attribute name: `shape`.
8. The `shape` attribute uses `uint32_t`, not `uint8_t`, for alignment and future symbol/atlas
   consistency.
9. GPU-backed marker picking is in place.
10. Picking currently uses the marker sprite bounding box. Exact code-SDF shape-mask picking remains
    deferred.

Style naming:

1. `color` is fill/tint color.
2. `edge_color` is edge/stroke color.
3. `line_width` is stroke width in screen pixels.
4. Style aspects are `filled`, `stroke`, and `outline`.

Deferred:

1. Bitmap marker mode.
2. Atlas-backed markers.
3. SDF/MSDF marker modes.
4. Shared marker/glyph atlas infrastructure.
5. Scalar color and grouped attributes.
6. Data-space marker sizing.


## Segment

Implemented first slice:

1. A retained `segment` visual for independent endpoint pairs is in place.
2. Endpoint attribute names are `position_start` and `position_end`.
3. Use screen-space stroked segments based on the v0.3 analytic four-vertex/six-index technique.
4. Support non-arrow caps needed by the v0.3 segment cap model.
5. Default caps are `butt` at both ends.
6. Width is screen-space only in the first slice.
7. Do not implement dashes in the first slice.
8. Do not implement arrow caps in the first slice; arrows belong to the later marker/attachment lane.
9. Scalar color, `color_end`, grouped attributes, and segment picking remain deferred.
10. GLSL/native runtime support is in place; WGSL segment lowering remains deferred.

Deferred:

1. Dashes.
2. Arrow caps and marker attachments.
3. `color_end` gradients.
4. Scalar color.
5. Grouped width/color.
6. Data-space line width.
7. Segment picking.
8. WGSL support if not completed in the first renderer pass.


## Path

Ordering:

1. The segment first slice has landed.
2. The current path implementation keeps primitive line-strip rendering when `line_width` is absent
   and lowers to the segment stroke pipeline when `line_width` is present.

Implemented first stroked path slice:

1. The current line-strip convenience path is preserved for paths without `line_width`.
2. Explicit open subpath metadata is provided by `dvz_path_set_subpaths()`.
3. `dvz_visual_set_data("span_sizes", ...)` is not used as the primary public API.
4. Default caps are `butt` through the segment stroke pipeline.
5. Path-specific joins remain deferred.
6. Path-specific miter limit remains deferred.
7. Per-point/per-vertex path width is implemented.
8. Open subpath lengths are preserved explicitly; closed path metadata remains deferred.

Deferred:

1. Dashes.
2. SVG parsing.
3. Filled paths/polygons.
4. Path picking unless explicitly scoped after rendering is stable.
5. Data-space line width unless a separate implementation note defines the 2D/3D projection rules.
6. Closed subpaths.
7. Path-specific joins and miter-limit handling.


## Remaining Worker Lanes

1. Exact marker SDF-mask picking, if the bounding-box pick area is too broad for examples.
2. Segment and path WGSL lowering.
3. Segment/path picking based on screen-space stroke width.
4. Path closed-subpath, join, and miter-limit support.
5. Scalar/grouped/constant-source attribute modes shared across these families.
6. Bitmap/SDF/MSDF marker modes and shared marker/glyph atlas infrastructure.
7. Dashes, arrow caps, and marker attachments for segment/path.

Workers are not alone in the codebase. Each lane must preserve unrelated user edits, avoid reverting
other workers' changes, and coordinate around shared files such as `include/datoviz/scene.h`,
`src/scene/_scene.h`, `src/scene/visual.c`, `src/scene/visual_pipeline.c`, shader registry files, and
scene tests.


## Validation Baseline

Every committed slice:

```text
git diff --check
just build
```

Scene visual API, retained-state, or emission changes:

```text
just test scene
```

Runtime/offscreen/picking changes:

```text
just test app
```

DRP2 fixture, recording, or portable stream changes:

```text
just test drp2
just spec-check
```

When Vulkan/GLSL runtime behavior changes, add focused bounded examples or offscreen smokes in
addition to the narrow tests.
