# Scene Visual Family Implementation Decisions

> **Execution Status**
> - **Status:** `ACTIVE FOLLOW-UP CONTRACT`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve the landed first-slice visual-family decisions and define the remaining
>   follow-up lanes for pixel, point, marker, segment, path, image, sphere, and mesh consistency.


## Scope

This note refines:

1. `spec/scene/visuals/PIXEL.md`
2. `spec/scene/visuals/POINT.md`
3. `spec/scene/visuals/MARKER.md`
4. `spec/scene/visuals/SEGMENT.md`
5. `spec/scene/visuals/PATH.md`
6. `spec/scene/visuals/IMAGE.md`
7. `spec/scene/visuals/SPHERE.md`
8. `spec/scene/visuals/MESH.md`
9. `spec/scene/visuals/VOLUME.md`
10. `agents/soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md`
11. `agents/soon/scene/SCENE_VECTOR_VISUALS_PLAN.md`

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
7. Do not add CPU fallback picking. Unsupported GPU pick precision should return an explicit
   request/result failure status rather than pretending to be a miss.
8. Avoid vague `size` attributes in new or refactored API. Use `pixel_size`, `diameter`, `radius`,
   `stroke_width`, and `extent` according to the family contract.
9. Data upload should use generic `dvz_visual_set_data()` attributes. Typed public setters should
   configure behavior, style, material, or render modes rather than duplicate per-attribute data
   upload.
10. Workers may commit after coherent slices once `git diff --check`, `just build`, and the narrow
   relevant tests pass.


## Consistency Pass Decisions

1. `pixel` uses `pixel_size`, not `size`.
2. `point` and `marker` use `diameter`, not `size`.
3. `sphere` uses `radius`, not `size`, and should stop using typed data setters as the primary API.
4. `segment` and stroked `path` use `stroke_width`, not `line_width`.
5. `image` is a multi-item visual with per-item `position`, `extent`, optional `anchor`, and
   optional `tex_rect`, sampling one shared field/texture/atlas in the first coherent slice.
   `angle` and tint remain follow-up attributes.
6. `mesh` supports instancing through per-instance attributes such as `instance_transform`,
   `instance_color`, and optional authored `instance_id`; instance identity must be preserved in
   picking.
7. Point/marker share a fill/stroke style descriptor; segment/path share a stroke descriptor.
8. Picking results should distinguish real misses from unsupported target precision and GPU
   execution/readback failures through `DvzPickStatus`.


## Current Landed Status

Status on 2026-05-17: the first implementation batch for the five families has landed.

Additional visual-consistency updates landed on 2026-05-17:

1. Public visual attribute names now use `diameter`, `pixel_size`, `radius`, and `stroke_width`
   while preserving the current internal storage names.
2. `sphere` now uses the generic `dvz_visual_set_data()` path and no longer exposes duplicate
   typed data setters.
3. `DvzPickResult` has an explicit `DvzPickStatus` so misses, unsupported visuals, outside-panel
   requests, GPU execution failures, and readback failures are distinguishable.
4. `mesh` accepts per-instance `instance_transform` data, emits instanced vertex input layouts, and
   lowers draws with the retained instance count.
5. `image` supports multi-item sampled rectangles through per-item `position` + `extent` and
   optional `tex_rect` atlas coordinates, while keeping the legacy four-corner `texcoords` path.

Implemented:

1. `pixel`: public `position`/`color`/`pixel_size`, GLSL native square points, WGSL instanced
   quads, offscreen nonblank smoke, and GPU square picking.
2. `point`: public `position`/`color`/`diameter`, antialiased circular coverage, edge/stroke
   styling via `dvz_point_set_style()`, GLSL native point-coordinate rendering, WGSL instanced
   quads, and GPU circular picking.
3. `marker`: public `dvz_marker()`, code-SDF shapes `disc`, `square`, `triangle`, `diamond`,
   `cross`, and `ring`, public `position`/`color`/`diameter`/`angle`/`shape`, marker style API, GLSL
   marker rendering, WGSL point-like lowering, and GPU bounding-box picking.
4. `segment`: public `dvz_segment()`, `position_start`/`position_end` endpoint attributes,
   per-item `stroke_width`, RGBA color, analytic GLSL stroke quads, non-arrow caps, and cap
   validation/API.
5. `path`: existing primitive line-strip path plus a stroked path lane when per-point
   `stroke_width` is present, open subpath lengths via `dvz_path_set_subpaths()`, and GLSL lowering
   through the segment stroke pipeline.

Validation recorded after the batch:

1. `just test app` passed `58/58`.
2. `just spec-check` passed all DRP2 fixture and pytest phases.
3. Full `just test` passed `563/563`.

Remaining work should start from the deferred lists below rather than re-opening these first-slice
decisions.


## Next Implementation Batch

The next batch should focus on picking and backend parity, in this order:

1. Segment/path picking. Add GPU-backed hit requests for screen-space stroked segments first, then
   reuse the same distance-to-stroke logic for stroked paths. A hit should use the visible
   `stroke_width / 2` region plus a small tolerance; return the segment index for `segment` and the
   subpath/path identity for `path`.
2. Exact marker SDF-mask picking. Keep the current marker bounding-box picker as the broad first
   pass, then reject hits outside the active code-SDF shape so triangles, crosses, rings, and
   diamonds do not pick in transparent corners.
3. Segment/path WGSL lowering. Preserve the same public visual contract as GLSL; WebGPU should lower
   the analytic stroke representation transparently rather than exposing a backend-specific API.
4. Visual-family showcase. Add one compact example or smoke scene containing `pixel`, `point`,
   `marker`, `segment`, and stroked `path` together so future regressions are easier to spot.

Marker-specific execution phases live in `agents/soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md`.
Segment, path, dash, arrow, and SVG execution phases live in
`agents/soon/scene/SCENE_VECTOR_VISUALS_PLAN.md`. Volume implementation and napari-style example
sequencing lives in `agents/soon/scene/SCENE_VOLUME_RENDERING_FOLLOWUP.md`; the durable contract
lives in `VOLUME.md`.


## Pixel

Implemented first slice:

1. Keep the current retained API shape: `position`, `color`, and `pixel_size` are dense public
   attributes.
2. Internal storage may still reuse the historical `size` slot, but public code should use
   `pixel_size`.
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

1. Keep current dense `position`, `color`, and `diameter` public attributes.
2. `diameter` is the point diameter in screen pixels. Internal storage may still reuse the
   historical `size` slot.
3. Colors are RGBA only.
4. Antialiased circular point rendering is in place.
5. Point edge/stroke styling is in place.
6. GPU-backed circular picking is in place.
7. EDL, depth-cue, alpha-mode, WBOIT, and depth-peel eligibility remain routed through the existing
   point visual pass-capability path.

Naming:

1. `color` remains the face/fill color attribute.
2. `edge_color` is the stroke/edge color.
3. `stroke_width` is the stroke width in screen pixels.
4. Use one marker-compatible style aspect enum where needed: `filled`, `stroke`, or `outline`.

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
6. Attributes: `position`, `color`, `diameter`, and `angle`.
7. Optional per-item shape attribute name: `shape`.
8. The `shape` attribute uses `uint32_t`, not `uint8_t`, for alignment and future symbol/atlas
   consistency.
9. GPU-backed marker picking is in place.
10. Picking currently uses the marker sprite bounding box. Exact code-SDF shape-mask picking remains
    deferred.

Style naming:

1. `color` is fill/tint color.
2. `edge_color` is edge/stroke color.
3. `stroke_width` is stroke width in screen pixels.
4. Style aspect is one exclusive value: `filled`, `stroke`, or `outline`.

Deferred:

1. Bitmap marker mode.
2. Atlas-backed markers.
3. SDF/MSDF marker modes.
4. Shared marker/glyph atlas infrastructure.
5. Scalar color and grouped attributes.
6. Data-space marker sizing.

Marker render-mode rules:

1. Bitmap, SDF, and MSDF are marker render modes, not separate public visual families.
2. One marker item remains one screen-facing symbol anchored at a data/world position.
3. Bitmap mode samples an RGBA or alpha texture and applies marker color as tint/alpha according to
   the selected policy.
4. Atlas-backed marker modes use per-symbol UV rectangles and nominal bounds; the per-item `shape`
   or future `symbol` attribute selects the entry.
5. SDF/MSDF marker modes consume shared atlas entries and shared decode helpers; marker must not
   own a custom font or glyph pipeline.

Marker/glyph sharing boundary:

1. Shared internals may include atlas texture creation/upload, atlas entry metadata, UV rectangle
   lookup, SDF/MSDF decode helpers, sampler setup, DRP2 texture/bind-group emission, dirty-state
   handling, and descriptor-refresh behavior.
2. Marker semantics remain scatter-symbol semantics: position is the item anchor, diameter controls
   marker extent, angle rotates around the center, and no baseline, advance, shaping, or fallback is
   involved.
3. Glyph/text semantics remain text-layout semantics: atlas entry identity is tied to font face and
   glyph id, placement comes from layout, and metrics such as advance, bearing, baseline, line
   height, and bounds matter.
4. Keep `marker` and glyph/text as separate public surfaces even when they share atlas/MSDF
   infrastructure.


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
6. Data-space stroke width.
7. Segment picking.
8. WGSL support if not completed in the first renderer pass.


## Path

Ordering:

1. The segment first slice has landed.
2. The current path implementation keeps primitive line-strip rendering when `stroke_width` is
   absent and lowers to the segment stroke pipeline when `stroke_width` is present.

Implemented stroked path slice:

1. The current line-strip convenience path is preserved for paths without `stroke_width`.
2. Explicit open subpath metadata is provided by `dvz_path_set_subpaths()`.
3. `dvz_visual_set_data("span_sizes", ...)` is not used as the primary public API.
4. Path caps are configured by `dvz_path_set_caps()`.
5. Path-specific joins are configured by `dvz_path_set_join()`.
6. Path-specific miter-limit fallback is implemented for the GLSL/Vulkan path pipeline.
7. Per-point/per-vertex path width is implemented.
8. Open subpath lengths are preserved explicitly; closed path metadata remains deferred.

Deferred:

1. Dashes.
2. SVG parsing.
3. SVG import and filled paths beyond the semantic polygon/polygon-set composite path.
4. Path/subpath identity picking beyond the active stroke item-pick slice.
5. Data-space stroke width unless a separate implementation note defines the 2D/3D projection rules.
6. Closed subpaths.
7. WGSL lowering for path-native strokes.


## Stroke, Dash, Arrow, and Vector Rules

Segment and path share a stroke vocabulary.

Stroke rules:

1. Width is screen-space by default.
2. Stroke alignment is centered for the first implementation.
3. Antialias radius defaults to one pixel unless MSAA or target scale requires otherwise.
4. Path joins should support miter, bevel, and round; miter falls back to bevel when the configured
   limit is exceeded.
5. Segment/path caps include `none`, `butt`, `square`, `round`, `triangle_in`, and `triangle_out`.
6. Arrow-style caps and marker attachments are follow-up behavior layered on the same stroke
   vocabulary, not separate line families.

Dash rules:

1. Dashes are scene resources or visual-level stroke resources, not ad-hoc per-visual arrays.
2. Dashing uses cumulative path distance and dash phase; dash phase should be mutable without
   rebuilding source geometry.
3. Dash caps are separate from path-end caps.
4. A simple uniform/storage-buffer dash pattern is the first bridge; a shared dash atlas can follow
   when multiple visuals need pattern sharing.

GPU stroke representation:

1. Use generated triangles and fragment-shader analytic coverage, not hardware line primitives or
   geometry shaders.
2. Segment starts from the v0.3 four-vertex/six-index analytic-cap model.
3. Path-native strokes should use adjacency-style derived payloads with previous/current/next
   positions, stroke width, color, subpath metadata, closed/open flags, and cumulative distance.
4. The 2026-05-21 GLSL/Vulkan path slice replaces the temporary segment-lowered stroked path with
   a path-native descriptor and adjacency-style derived payload. The segment visual remains the
   independent endpoint-pair stroke family.

Vector and 3D line-family direction:

1. A 2D vector-field visual should lower to the segment/marker backend with source item identity
   preserved.
2. Tubes, streamlines, and 3D arrows are 3D geometry lanes, not reuse of 2D screen-space stroke
   shaders.
3. Dense streamlines should get a fast ribbon/strip path before every streamline is committed to a
   true tube mesh.
4. Tube mesh mode should use stable frames, preferably parallel-transport frames, and reuse mesh
   material, depth, SSAO/G-buffer, clipping, picking, and normal-generation policy where possible.


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
