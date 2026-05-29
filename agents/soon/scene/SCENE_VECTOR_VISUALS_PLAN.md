# Scene Vector Visuals Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining segment, path, vector, SVG, and 3D line-family work after the
>   first retained segment and stroked-path slices landed.


## Current State

Durable vector-family contracts live in:

1. [`../../../spec/scene/visuals/SEGMENT.md`](../../../spec/scene/visuals/SEGMENT.md)
2. [`../../../spec/scene/visuals/PATH.md`](../../../spec/scene/visuals/PATH.md)
3. [`../../../spec/scene/visuals/MARKER.md`](../../../spec/scene/visuals/MARKER.md)
4. [`../../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`](../../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
5. [`../../../spec/scene/visuals/VECTOR.md`](../../../spec/scene/visuals/VECTOR.md)

The active v0.4 code already has retained `dvz_segment()` endpoint-pair visuals, analytic GLSL
screen-space stroked segments, the first segment cap set, `dvz_segment_set_caps()`, primitive
line-strip `dvz_path()` rendering when no `line_width` is present, and stroked `dvz_path()`
lowering through derived segment-style resources for open subpaths.

Use this file only for remaining execution work. Do not duplicate stable visual contracts here.


## Remaining Vector Work

Recommended follow-up commits:

1. Add segment picking against the visible screen-space stroke, accounting for stroke width,
   tolerance, and endpoint caps. Return the source segment item index.
2. Add stroked path picking by reusing segment stroke hit logic over derived path edges while
   returning path/subpath identity rather than derived edge identity.
3. Add WGSL lowering for segment and stroked path so WebGPU preserves the public
   `position_start`/`position_end` and `line_width` semantics.
4. Add first-class closed-path API and path/subpath identity preservation on top of the landed
   path-native joins, miter-limit behavior, subpath metadata, and path-specific cap handling.
5. Add dashing after cumulative path-distance metadata and dash phase updates can be tested without
   rebuilding source geometry.
6. Settle the remaining open questions in the vector visual spec before implementation.
7. Add vector-field helpers and independent vector-head styling once picking metadata can map
   shafts and heads back to the same source item.
8. Keep SVG parsing, fills, markers, transforms, and static import as a later subset. Treat SVG as
   an authoring/import layer over Datoviz visuals, not as a separate renderer.
9. Keep dense 3D streamlines/ribbons and tube meshes behind the 2D stroke backend. Tube work should
   follow [`../../../spec/scene/visuals/TUBE.md`](../../../spec/scene/visuals/TUBE.md) and reuse
   mesh material, depth cueing, SSAO/G-buffer, and stable frame-generation rules.


## v0.3 Reference

Use v0.3 as behavior reference, not architecture:

1. `v0.3/src/scene/visuals/segment.c`
2. `v0.3/src/scene/glsl/graphics_segment.vert`
3. `v0.3/src/scene/glsl/graphics_segment.frag`
4. `v0.3/src/scene/visuals/path.c`
5. `v0.3/src/scene/glsl/graphics_path.vert`
6. `v0.3/src/scene/glsl/graphics_path.frag`
7. `v0.3/src/scene/visuals/marker.c`
8. `v0.3/src/scene/sdf.cpp`
9. `v0.3` arrow shape helpers for later 3D vector-head meshes.

Useful ideas to retain:

1. four-vertex/six-index analytic segment expansion;
2. Rougier-style shader-based antialiased stroked polylines;
3. cap, join, miter-limit, dash, and cumulative-length metadata;
4. marker attachments for vector heads and SVG-style path markers;
5. SVG-to-MSDF tooling as an asset path, not a required runtime import path.

Avoid reviving v0.3 batch ownership, direct allocation patterns, geometry-shader assumptions, or a
parallel 2D renderer.


## Validation

For vector visual work:

```text
just build
just test scene
git diff --check
```

For shader/runtime changes, add focused segment/path tests and an offscreen or bounded GLFW smoke.
For SVG parsing, fills, atlas resources, or portable recording changes, also run the relevant DRP2
and spec checks.
