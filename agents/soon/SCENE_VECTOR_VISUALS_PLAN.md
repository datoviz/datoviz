# Scene Vector Visuals Plan

> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED / FOLLOW-UP PLAN`
> - **Updated on:** `2026-05-17`
> - **Purpose:** stage the implementation of segment, stroked path, dashed line, arrow/vector, tube,
>   and SVG-subset visuals on the existing scene -> FramePlan -> DRP2 -> vklite runtime path.


## Current Position

The active scene slice now has:

1. a retained `dvz_segment()` visual for independent endpoint pairs;
2. `position_start` and `position_end` endpoint attributes;
3. per-item `line_width` in screen pixels;
4. RGBA color;
5. analytic GLSL stroked segment lowering based on the v0.3 four-vertex/six-index technique;
6. first-slice segment caps `none`, `round`, `triangle_in`, `triangle_out`, `square`, and `butt`;
7. `dvz_segment_set_caps()`;
8. the existing `dvz_path()` primitive line-strip path when no `line_width` is present;
9. a stroked `dvz_path()` lane when per-point `line_width` is present;
10. open subpath lengths via `dvz_path_set_subpaths()`;
11. stroked path lowering through derived segment-style resources.

It does not yet model dashes, arrowheads, path-specific joins, path-specific miter-limit behavior,
closed subpaths, filled paths, vector fields, curve flattening, or SVG path semantics. Segment/path
picking and WGSL segment/path lowering are also still follow-up work.

## Next Concrete Steps

Immediate vector follow-up should happen in this order:

1. Add segment picking against the visible screen-space stroke. The hit area should use
   `line_width / 2` plus a small tolerance, account for endpoint caps where supported, and return
   the segment item index.
2. Add stroked path picking by reusing the segment stroke hit logic over each derived path edge.
   Return the path/subpath identity, not the derived edge vertex.
3. Add WGSL lowering for segment and stroked path so WebGPU keeps the same public
   `position_start`/`position_end` and `line_width` semantics as GLSL.
4. After picking and WGSL parity, move to closed subpaths, path-native joins, and miter-limit
   behavior before dashes or arrow caps.

The cross-family execution order is recorded in
`spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`.

This is not greenfield. The `v0.3/` tree already contains dedicated segment, path, marker, SVG-SDF,
and 3D arrow work that should be mined and ported selectively:

1. `v0.3/src/scene/visuals/segment.c` creates an indexed triangle-list visual with four vertices
   and six indices per segment, start/end positions, pixel shifts, per-segment color, per-segment
   line width, and start/end cap parameters.
2. `v0.3/src/scene/glsl/graphics_segment.vert` projects segment endpoints to screen space, expands
   a conservative quad in pixels, and passes local `(u, v)` stroke coordinates to the fragment
   shader.
3. `v0.3/src/scene/glsl/graphics_segment.frag` uses the shared antialias helpers to draw body
   coverage and cap coverage analytically.
4. `v0.3/src/scene/visuals/path.c` creates a triangle-strip stroked path visual with
   `(p0, p1, p2, p3)` adjacency, variable per-point color/line width, open/closed path flags,
   miter limit, cap type, and round/square join parameters.
5. `v0.3/src/scene/glsl/graphics_path.vert` and `graphics_path.frag` are Rougier-derived shaders
   that already implement screen-space path expansion, miter handling, miter-limit fallback, round
   joins, caps, and antialiasing.
6. `v0.3/include/datoviz_enums.h` already defines cap values (`none`, `round`, `triangle_in`,
   `triangle_out`, `square`, `butt`), join values (`square`, `round`), path open/closed flags, and
   marker shapes including `DVZ_MARKER_SHAPE_ARROW`.
7. `v0.3/src/scene/visuals/marker.c`, `graphics_marker.*`, and
   `include/datoviz/scene/glsl/markers.glsl` implement point-sprite markers with code-generated SDF
   shapes, bitmap/SDF/MSDF/MTSDF modes, fill/stroke/outline aspects, rotation, edge color, and edge
   width.
8. `v0.3/src/scene/sdf.cpp` exposes `dvz_sdf_from_svg()` and `dvz_msdf_from_svg()` through
   `msdfgen` when available. This is useful prior art for SVG-to-marker textures, but not a complete
   retained SVG scene importer.
9. `v0.3` also has `dvz_shape_arrow()` for 3D arrows as cylinder+cone mesh geometry; this belongs in
   the later mesh/tube lane, not in the 2D marker lane.

The target is not to add a separate 2D renderer. Vector visuals should stay normal retained scene
visuals:

1. `scene` owns retained attributes, stroke style, marker state, path preprocessing, and FramePlan
   metadata.
2. `drp2` receives ordinary buffers, textures, shaders, bind groups, pipelines, and draws.
3. `vklite` executes the same command stream as other scene visuals.
4. `app` and offscreen/live windows should not need a vector-specific presentation path.


## Design Constraints

1. Keep the first implementation portable across Vulkan and WebGPU. Do not rely on geometry shaders.
2. Prefer the v0.3/Rougier shader-based stroke evaluation over CPU tessellating every dash or round
   cap into many triangles.
3. Keep style changes cheap. Line width, dash phase, cap mode, and color changes should not force a
   full geometry rebuild when the point coordinates are unchanged.
4. Keep joins and dashes deterministic in screen space. Scientific plots need stable line quality
   across GPUs and zoom levels more than they need exact browser-SVG corner cases in the first slice.
5. Treat SVG as an import/authoring layer over Datoviz scene primitives, not as the core scene model.
6. Preserve the current v0.4 path visual until the ported stroked path replacement is fully tested,
   then migrate the path family deliberately.
7. Treat v0.3 code as design evidence, not as copy-paste-ready code. The v0.4 port must use retained
   scene objects, FramePlan emission, DRP2 resources, vklite runtime state, `dvz_*` allocation
   wrappers, and current shader registry conventions.


## Reference Technique

Nicolas P. Rougier, "Shader-Based Antialiased, Dashed, Stroked Polylines", JCGT 2(2), 2013:
https://jcgt.org/published/0002/02/08/paper.pdf

The important takeaways for Datoviz are:

1. Use a fragment-shader distance test inside a conservative stroked-line domain rather than relying
   on hardware line rasterization.
2. Compute cap and body coverage analytically in the fragment shader.
3. Represent dashes by path distance, dash period, dash phase, and a reusable dash atlas, avoiding
   per-dash CPU tessellation.
4. Support dash caps separately from path-end caps.
5. Accept a small preprocessing step that gives the shader enough per-segment and cumulative-length
   metadata.

SVG references for the compatibility target:

1. SVG paths define moveto, lineto, cubic/quadratic curves, elliptical arcs, and closepath:
   https://www.w3.org/TR/SVG2/paths.html
2. SVG painting covers fills, strokes, caps, joins, dashes, fill rules, and markers:
   https://www.w3.org/TR/SVG/painting.html
3. The SVG stroke draft adds more advanced dashing controls such as dash-corner and dash-adjust, but
   those should remain out of the first compatibility subset:
   https://w3c.github.io/svgwg/specs/strokes/


## Visual Families

### Segment

`segment` is the narrow foundation. It represents independent 2D or 3D line segments with no joins:

1. per-item attributes: `position_start`, `position_end`, `color`, and `line_width`;
2. visual-level stroke style: start cap, end cap, antialias radius, dash style, coordinate mode;
3. one segment is one independent primitive, so caps and arrowheads are unambiguous;
4. v0.3 already validates the four-vertex/six-index analytic-cap model; the first v0.4 port into
   retained scene/DRP2 has landed for solid strokes and non-arrow caps;
5. useful immediately for plot ticks, grid lines, rulers, annotations, graph edges, and vector-field
   shafts.

This should be implemented before full paths because it isolates stroke width, caps, dashing, and
screen-space extrusion without join complexity.


### Path

`path` is an ordered set of one or more subpaths:

1. per-vertex attributes: `position`, optional `color`, optional `line_width`;
2. structural metadata: subpath lengths first, closed/open bit and cumulative arc length later;
3. visual-level stroke style: cap mode, join mode, miter limit, dash style, antialias radius;
4. derived adjacency data equivalent to the v0.3 `(p0, p1, p2, p3)` vertex payload;
5. optional fill style for closed paths in later stages.

The current line-strip path remains available when `line_width` is absent. When `line_width` is
present, the first stroked path slice derives segment-style resources and uses the segment stroke
pipeline. The v0.3 path shader remains the baseline for later miter/round joins, caps, and
path-native pixel-width strokes; dashing, closed subpaths, and SVG subpaths are still missing.


### Arrow And Marker

Arrows should be modeled as markers attached to a segment or path, not as a separate hard-coded line
variant:

1. marker positions: start, mid, end;
2. marker shapes: reuse the v0.3 code-based SDF set where possible, including arrow, chevron,
   triangle, bar, circle/disc, square, diamond, pin, tag, and rounded rectangle;
3. marker modes: start with code-generated SDF shapes; keep bitmap/SDF/MSDF/MTSDF support as a
   second port because it depends on texture resources and optional `msdfgen`;
4. aspects: filled, stroke, and outline;
5. orientation: tangent-based `auto`, `auto-start-reverse`, or fixed angle;
6. sizing: pixels, stroke-width multiples, or data/world units;
7. fill/stroke style: inherit from parent stroke by default, with override hooks later.

The public "2D arrow" API can be a convenience wrapper that creates a `segment` plus an end marker.


### Vector Field

`vector` should be an instanced glyph visual over the same stroke/marker backend:

1. attributes: anchor position, vector direction, magnitude, optional color, optional scale;
2. modes: raw vector length, normalized direction with scalar length mapping, fixed pixel length;
3. glyphs: shaft-only, arrow, barbed arrow, line-with-dot;
4. color mapping: direct RGBA first, scalar scale/colormap later;
5. picking: item id should refer to the source vector, not the generated shaft/marker geometry.


### Tube, Streamline, And 3D Arrow

`tube` is a 3D path visual, not a 2D stroke. This lane should explicitly cover tractography and
other dense streamline/trajectory use cases:

1. input: many variable-length 3D polylines or curve-flattened paths;
2. output modes: fast camera-facing ribbon/strip, true retained tube mesh, and optional directional
   glyphs/arrowheads;
3. cross-sections: ribbon first for dense tractograms, round tube with configurable side count for
   selected/high-quality bundles, square/other sections later;
4. caps: none and flat first, round/hemisphere later;
5. joins/frames: stable parallel-transport frames for tubes, view-stable orientation for ribbons,
   and explicit handling of sharp turns or broken streamlines;
6. material: use existing mesh/primitive material and depth/SSAO paths for tube meshes; use a lean
   unlit/depth-cued shader for dense ribbons;
7. attributes: per-vertex or per-streamline color, optional scalar, width/radius, bundle id, and
   source streamline id for picking/selection.

Tubes and 3D arrows should wait until the 2D stroke path is stable enough to share input topology and
style vocabulary, but they should not reuse the 2D screen-space stroke shaders. They need different
geometry, depth, lighting, clipping, picking, and performance policy.

Particle systems are related but distinct. A particle system owns dynamic GPU-updated state such as
position, velocity, age, and size; track, streamline, and tube visuals are optional consumers of
particle history or selected trajectories. The scene-level design lives in
`spec/scene/proposals/active/PARTICLE_SYSTEM_DESIGN.md`.

For tractography, the practical rendering ladder should be:

1. **Ribbon/strip mode:** fastest path for many streamlines; supports depth cueing, clipping, alpha,
   bundle coloring, and picking by streamline id.
2. **Tube mesh mode:** true 3D normals and lighting for selected bundles, screenshots, or lower-density
   scenes; generated from polyline data with parallel-transport frames.
3. **3D arrow/glyph mode:** cone/cylinder or cone-only direction cues for selected streamlines,
   vector fields, flow direction, or orientation debugging. Reuse the v0.3 `dvz_shape_arrow()` design
   as geometry prior art, but emit retained v0.4 mesh/instance resources.


## Stroke Model

Add a shared stroke descriptor before adding more visual constructors:

```c
typedef enum DvzStrokeWidthMode
{
    DVZ_STROKE_WIDTH_SCREEN,
    DVZ_STROKE_WIDTH_WORLD,
} DvzStrokeWidthMode;

typedef enum DvzStrokeCap
{
    DVZ_STROKE_CAP_NONE,
    DVZ_STROKE_CAP_BUTT,
    DVZ_STROKE_CAP_SQUARE,
    DVZ_STROKE_CAP_ROUND,
    DVZ_STROKE_CAP_TRIANGLE_IN,
    DVZ_STROKE_CAP_TRIANGLE_OUT,
} DvzStrokeCap;

typedef enum DvzStrokeJoin
{
    DVZ_STROKE_JOIN_MITER,
    DVZ_STROKE_JOIN_BEVEL,
    DVZ_STROKE_JOIN_SQUARE,
    DVZ_STROKE_JOIN_ROUND,
} DvzStrokeJoin;

typedef struct DvzStrokeDesc
{
    float width;
    DvzStrokeWidthMode width_mode;
    DvzStrokeCap cap_start;
    DvzStrokeCap cap_end;
    DvzStrokeJoin join;
    float miter_limit;
    float antialias;
    uint64_t dash_id;
    float dash_phase;
} DvzStrokeDesc;
```

Default policy:

1. width is in pixels;
2. antialias radius is one pixel unless MSAA or target scale suggests otherwise;
3. cap is butt for path/SVG compatibility, round for plot-friendly helper APIs only if explicitly
   requested;
4. join is miter with a conservative miter limit, falling back to bevel on sharp turns;
5. stroke alignment is centered only in the first implementation.
6. map the new enum values deliberately from the v0.3 integer values so shader ports do not depend
   on legacy numeric order.


## Dash Model

Dashing should be a first-class scene resource, not ad-hoc per-visual arrays:

```c
typedef struct DvzDashPatternDesc
{
    const float* lengths;
    uint32_t length_count;
    float period;
} DvzDashPatternDesc;
```

The v0.3 stroke code does not appear to implement dashed segments or dashed paths. Dashing should
therefore be treated as new v0.4 behavior layered on top of the ported analytic stroke coordinate
system and extended with cumulative path distance.

First implementation:

1. store a small dash pattern table in a uniform/storage buffer for a single visual;
2. evaluate `mod(path_distance + dash_phase, period)` in the fragment shader;
3. support solid, dot, dash, dash-dot, and user-provided even-length arrays;
4. apply dash caps independently from path-end caps;
5. keep dash phase mutable without rebuilding vertex buffers.

Second implementation:

1. add a scene-owned dash atlas texture or storage-buffer atlas;
2. pack one dash pattern per row/record with reference point, dash subtype, dash start, and dash end;
3. bind the atlas through the normal scene resource path;
4. allow many visuals to share the same dash pattern resource;
5. keep texture format support explicit in scene capabilities.

The atlas version should follow Rougier's direction because it avoids CPU tessellation for animated
dashes and arbitrary dash patterns. The simple uniform version is still useful as a low-risk bridge.


## GPU Stroke Representation

Use generated triangles, not hardware line primitives. The v0.3 segment and path implementations
already prove this approach in Datoviz; the v0.4 work is to adapt the model to retained visuals,
FramePlan resources, DRP2 command emission, shader variants, and runtime reuse.

For `segment`:

1. start from the v0.3 four-vertex/six-index shape;
2. keep endpoint projection and screen-space quad expansion in the vertex shader;
3. preserve the optional `shift` attribute for pixel offsets at each endpoint because it is useful
   for ticks, error bars, and annotation offsets;
4. fragment shader receives local `(u, v)` coordinates, segment length, width, cap modes, and dash
   metadata;
5. coverage is computed analytically and multiplied into alpha;
6. consider instanced unit-quads only after the direct port is correct and benchmarked.

For `path`:

1. start from the v0.3 adjacency payload: previous point, current point, next point, next-next point,
   per-point color, and per-point line width;
2. extend the derived payload with subpath id, closed/open flags, cumulative distance, and dash flags;
3. emit conservative segment quads for every path edge;
4. port miter-limit and round-join shader logic before adding dash logic;
5. avoid retaining pointers into growable scene arrays during preprocessing; store stable offsets and
   rebuild derived buffers after source data changes.

This representation costs more vertices than a geometry-shader approach but keeps the implementation
portable and debuggable.


## Staged Implementation

### Stage 0. Vocabulary And Internal Data

Scope: v0.3 audit plus public/internal type groundwork, no new rendering behavior.

Expected work:

1. inventory the exact v0.3 segment/path/marker public surface and decide which names remain public
   in v0.4 and which become compatibility notes only;
2. add `DvzStrokeDesc`, cap/join/width enums, and dash descriptors to `include/datoviz/scene/types.h`
   or a `scene/vector.h` subheader if the header is getting too large;
3. include `none`, `round`, `triangle_in`, `triangle_out`, `square`, and `butt` caps because v0.3
   users and shaders already distinguish them;
4. add internal stroke state to `DvzVisual`;
5. add `dvz_visual_set_stroke()` and `dvz_visual_stroke()` only if the public API is ready; otherwise
   keep the setter internal and use examples/tests through narrowly scoped constructors;
6. add validation helpers for width, antialias radius, miter limit, dash period, and dash arrays;
7. add tests that stroke defaults are stable and invalid descriptors are rejected.

Validation:

```text
just build
just test scene
git diff --check
```


### Stage 1. Port Segment Visual Without Dashes

Scope: first true stroked primitive, ported from v0.3.

Status on 2026-05-17: implemented for GLSL/native rendering with solid strokes and non-arrow caps.
WGSL lowering, endpoint `shift`, dashes, arrow caps, scalar/grouped color, and picking remain
follow-up work.

Expected work:

1. add `DVZ_VISUAL_TYPE_SEGMENT`;
2. add `dvz_segment()` with dense attributes matching v0.4 naming: `position_start`,
   `position_end`, `color`, and `line_width`;
3. port `graphics_segment.vert`, `graphics_segment.frag`, and the relevant `antialias.glsl`
   functions into the v0.4 built-in shader registry;
4. lower segment visuals through FramePlan like other retained visuals;
5. add a DRP2 fixture for the four-vertex/six-index resource/pipeline/draw shape;
6. add offscreen smoke coverage for thick horizontal, vertical, diagonal, subpixel, and zero-length
   segments;
7. support `none`, butt, square, round, triangle-in, and triangle-out caps before moving to joins;
8. port the `v0.3/examples/features/segment_cap.py` coverage as a C/offscreen scene example or test.

Validation:

```text
just build
just test test_scene_segment
just test scene
git diff --check
```


### Stage 2. Port Path Geometry And Joins

Scope: replace the current segment-lowered stroked path with a retained stroked polyline,
ported from v0.3. Keep the primitive line-strip path available for paths without `line_width`
until the path-native stroke path is stable and benchmarked.

Status on 2026-05-17: partially implemented. Paths without `line_width` still use the primitive
line-strip path. Paths with per-point `line_width` derive segment-style stroke resources and support
open subpath lengths. Path-native joins, miter limits, closed subpaths, dashes, and picking remain
follow-up work.

Expected work:

1. represent paths as vertices plus subpath metadata rather than one implicit line strip;
2. port the v0.3 derived adjacency generation, but replace direct `calloc`/`FREE` and visual data
   writes with v0.4 allocator wrappers and retained visual attribute storage;
3. add APIs for subpaths and closed paths, for example `dvz_path_set_subpaths()`;
4. build derived segment buffers with cumulative path distance;
5. support open and closed paths with the same endpoint duplication/wrap semantics as v0.3;
6. port the miter-limit and round-join shader behavior first;
7. add explicit bevel/square naming so v0.4 terminology is not ambiguous;
8. preserve fixed-panel/controller modes and z-layer ordering;
9. keep alpha/WBOIT interactions explicit: ordinary alpha blend first, WBOIT only after the opaque
   path is stable.

Focused implementation checklist:

1. Add path-native derived resources in `src/scene/scene_emit.c` instead of reusing segment
   endpoint resources for stroked paths. The first derived payload should mirror the v0.3
   `DvzPathVertex` inputs: previous point `p0`, current point `p1`, next point `p2`, next-next
   point `p3`, per-point color, and per-point `line_width`.
2. Rebuild that payload from retained `position`, `color`, `line_width`, and
   `dvz_path_set_subpaths()` metadata. Open subpaths should duplicate endpoint neighbours exactly
   as v0.3 does; closed subpaths should wrap neighbours once closed-path metadata is added.
3. Preserve the existing public `stroke_width` alias, but keep the internal storage name
   `line_width` unless the whole visual attribute vocabulary is deliberately renamed.
4. Add a path-stroke shader family under `src/scene/glsl/`, adapted from
   `v0.3/src/scene/glsl/graphics_path.vert` and `graphics_path.frag`. The port should use v0.4
   `common.glsl`, viewport, material, clipping, depth, and shader registry conventions rather than
   legacy v0.3 include/binding conventions.
5. Add a distinct built-in shader identity and pipeline path for stroked paths in
   `src/scene/visual_pipeline.c`. A stroked path should no longer be described as
   `DVZ_SCENE_VISUAL_DESC_SEGMENT` once path-native joins are active.
6. Add material or stroke parameters for cap mode, join mode, miter limit, and antialias radius.
   The first port should make round joins and miter-limit fallback match v0.3 before adding dashes.
7. Keep the current segment-lowered path as a temporary fallback while the new shader is being
   tested. Remove or gate the fallback only after offscreen path-join tests and the live path example
   show no cracks at acute joins.
8. Update `include/datoviz/scene.h`, `spec/scene/visuals/PATH.md`, and examples only when the
   supported public surface changes. Do not expose `dvz_path_join()` or closed-path flags until the
   retained state, shader path, and tests all agree on the semantics.

Regression targets:

1. A focused scene emission test should verify that a stroked path binds path-native adjacency
   resources and the path-stroke pipeline rather than the segment pipeline.
2. An offscreen pixel test should render a thick acute polyline on a contrasting background and
   assert that the join region has continuous coverage. This should fail on the current
   segment-lowered path.
3. Add cases for open endpoints, single subpath, multiple subpaths, repeated partial updates, and
   per-point width changes.
4. Add a live-example pressure check using `./build/examples/c/visuals/path` with a thick stroked
   zigzag, because that is the easiest manual way to catch join cracks and miter spikes.

Validation:

```text
just build
just test test_scene_path
just test scene
git diff --check
```


### Stage 3. Dashing

Scope: dash patterns and dash phase for segment and path.

Expected work:

1. add scene-owned dash pattern objects or a visual-level first slice;
2. compute cumulative path distance for every stroked fragment;
3. implement solid/dot/dash/dash-dot built-ins;
4. support arbitrary user dash arrays with even-length normalization;
5. support dash phase animation without rebuilding derived geometry;
6. implement dash caps and ensure path-end caps still apply at subpath boundaries;
7. add tests for dash continuity across joins, closed-path wrapping, zero-length dashes rejection,
   and changing phase over repeated app frames.

Validation:

```text
just build
just test test_scene_dash
just test scene
git diff --check
```


### Stage 4. Marker And Arrow Visuals

Scope: arrows and SVG-like markers, building on the v0.3 marker visual.

Expected work:

1. port the v0.3 code-based marker SDF functions and the filled/stroke/outline fragment logic;
2. add retained marker descriptors for standalone marker visuals and for path marker attachments
   (`start`, `mid`, `end`);
3. support `position`, `size`, `angle`, and `color` attributes plus edge color/line width params;
4. initially use the same point-sprite model as v0.3 for standalone markers if it remains reliable
   in Vulkan/WebGPU; use instanced quads if point-size behavior is too backend-specific;
5. attach marker draws to the same FramePlan render node as the parent path when possible;
6. compute marker tangents from segment endpoints or path join tangents;
7. add convenience API for 2D arrows over `segment`, using the existing v0.3 arrow marker shape as
   the first arrowhead;
8. add bitmap/SDF/MSDF marker texture modes only after code-based markers are stable;
9. add picking metadata so the arrowhead and shaft map back to the same source item;
10. add examples for arrows, graph edges, and vector field glyphs.

Validation:

```text
just build
just test test_scene_arrow
just test scene
git diff --check
```


### Stage 5. Vector Field Visual

Scope: ergonomic scientific vector glyphs.

Expected work:

1. add `dvz_vector()` as an instanced visual using the segment/marker backend;
2. support 2D and 3D anchors;
3. support fixed pixel length, world/data length, and normalized vectors scaled by magnitude;
4. support direct color first, scalar colormap later;
5. support optional arrowheads and shaft width;
6. add panzoom/arcball behavior tests for screen-width and world-width modes.

Validation:

```text
just build
just test test_scene_vector
just test scene
git diff --check
```


### Stage 6. SVG Path Parsing And Curve Flattening

Scope: SVG geometry ingestion without fill rendering yet.

Expected work:

1. add a small parser for SVG path data commands: `M`, `L`, `H`, `V`, `C`, `S`, `Q`, `T`, `A`, `Z`
   and their relative variants;
2. convert quadratic curves to cubic or flatten directly;
3. implement elliptical arc conversion and adaptive flattening;
4. store subpath boundaries and closed flags;
5. evaluate whether the existing optional `msdfgen` SVG path parser used by
   `v0.3/src/scene/sdf.cpp` can be reused for marker texture generation only, or whether retained
   path geometry needs a small native parser to avoid depending on optional C++/MSDF code;
6. expose an API that loads a path string into a Datoviz path object without requiring XML parsing;
7. add golden tests for common path strings, relative commands, arcs, malformed data, and closepath
   behavior.

Validation:

```text
just build
just test test_scene_svg_path_parse
just test scene
git diff --check
```


### Stage 7. SVG Fills

Scope: filled closed paths and polygons.

Expected work:

1. add fill style to path visuals: fill color, fill opacity, fill rule;
2. support `nonzero` and `evenodd` fill rules;
3. choose a triangulation backend. Prefer a small, vendored, well-tested tessellator over a custom
   polygon tessellator;
4. maintain separate fill and stroke draws so stroke-only, fill-only, and fill+stroke paths work;
5. add winding, holes, self-intersection, and degenerate contour tests;
6. make pick ids map filled triangles back to the source path item.

Validation:

```text
just build
just test test_scene_svg_fill
just test scene
git diff --check
```


### Stage 8. Reasonable SVG Subset Import

Scope: import useful scientific/annotation SVG, not full browser SVG.

Supported first subset:

1. geometry: `path`, `line`, `polyline`, `polygon`, `rect`, `circle`, `ellipse`;
2. path commands: `M L H V C S Q T A Z`;
3. presentation: `fill`, `fill-opacity`, `fill-rule`, `stroke`, `stroke-opacity`,
   `stroke-width`, `stroke-linecap`, `stroke-linejoin`, `stroke-miterlimit`,
   `stroke-dasharray`, `stroke-dashoffset`;
4. transforms: `translate`, `scale`, `rotate`, `skewX`, `skewY`, `matrix`;
5. viewport: `viewBox`, `width`, `height`, `preserveAspectRatio` in the common modes;
6. grouping: `g` with inherited style and transforms;
7. markers: `marker-start`, `marker-mid`, `marker-end` for built-in/simple marker shapes;
8. colors: hex, rgb/rgba, named colors if a compact table is acceptable.

Explicitly unsupported first:

1. CSS selector cascade beyond inline `style` and inherited attributes;
2. filters, masks, clip paths, blend modes, patterns, and gradients;
3. text layout and font shaping;
4. animation and scripting;
5. external resources;
6. percentage/unit corner cases beyond viewBox-relative common usage.

What else is needed for this SVG subset:

1. an XML tokenizer/parser or a tiny dependency chosen deliberately;
2. a style resolver with inheritance and presentation attributes;
3. a transform stack and bounding-box utilities;
4. path parser and curve flattening;
5. stroke style mapping to `DvzStrokeDesc`;
6. fill tessellation;
7. marker expansion;
8. optional SDF/MSDF marker texture generation, probably behind the same kind of feature gate as
   v0.3 `HAS_MSDF`;
9. color parsing and opacity composition;
10. import diagnostics that report unsupported SVG features instead of silently dropping them;
11. a fixture corpus with small hand-authored SVGs and exported examples from Inkscape/Matplotlib.

Validation:

```text
just build
just test test_scene_svg_import
just test scene
git diff --check
```


### Stage 9. Streamlines, Tubes, And 3D Arrows

Scope: 3D path geometry for tractography, trajectories, vector fields, and selected directional
glyphs.

Expected work:

1. review the existing v0.3 `dvz_shape_arrow()` cylinder+cone mesh path before choosing public 3D
   arrow/tube APIs;
2. add a fast `dvz_streamline()` or path render mode for dense 3D polyline/ribbon output before
   committing every streamline to tube mesh geometry;
3. add `dvz_tube()` or a tube render mode for true 3D tube output;
4. support variable-length polyline batches with per-streamline offsets/counts and stable source ids;
5. generate indexed mesh buffers from a source polyline for tube mode;
6. compute stable frames with parallel transport, not naive Frenet frames;
7. support ribbon width, tube radius, round tube cross-section with a small configurable side count,
   and flat caps first;
8. add optional 3D arrowheads/glyphs as retained mesh instances for selected streamlines or vectors;
9. support tractography-style coloring: per-vertex RGB, per-bundle color, direction color, scalar
   colormap later;
10. support clipping/slicing hooks so dense brain-tract views can be cut without rebuilding geometry;
11. reuse mesh material state, depth cueing, SSAO/G-buffer eligibility, and normal generation where
    tube mode uses true geometry;
12. add examples for tractography bundles, streamlines, trajectories, vector fields, and 3D graph
    edges.

Validation:

```text
just build
just test test_scene_streamline
just test test_scene_tube
just test scene
git diff --check
```


## API Direction

Likely public API shape:

```c
DvzVisual* dvz_segment(DvzScene* scene, uint32_t flags);
DvzVisual* dvz_path(DvzScene* scene, uint32_t flags);
DvzVisual* dvz_vector(DvzScene* scene, uint32_t flags);
DvzVisual* dvz_streamline(DvzScene* scene, uint32_t flags);
DvzVisual* dvz_tube(DvzScene* scene, uint32_t flags);

int dvz_visual_set_stroke(DvzVisual* visual, const DvzStrokeDesc* desc);
DvzDashPattern* dvz_dash_pattern(DvzScene* scene, const DvzDashPatternDesc* desc);
int dvz_visual_set_dash(DvzVisual* visual, DvzDashPattern* pattern, float phase);
int dvz_path_set_subpaths(DvzVisual* path, const DvzPathSubpath* subpaths, uint32_t count);
int dvz_path_from_svg_d(DvzVisual* path, const char* svg_path_data, const DvzSvgPathDesc* desc);
```

Keep exact names flexible until the first implementation slice touches headers. The important point is
to separate geometry data, stroke style, dash resource, and marker state.


## Testing Strategy

Unit tests:

1. descriptor validation;
2. dash-pattern normalization;
3. cumulative-length generation;
4. subpath closure and zero-length segment handling;
5. SVG path parsing and transform composition;
6. fill tessellation edge cases.
7. v0.3 behavior parity for segment caps, path open/closed handling, path variable line widths,
   marker modes/aspects, and marker rotation.

DRP2/spec tests:

1. segment/path pipeline and resource shape;
2. dash atlas upload shape;
3. marker draw shape;
4. portable DVZR recording for segment/path/vector visuals.

Runtime tests:

1. offscreen screenshots for cap modes, join modes, dashes, and arrows;
2. repeated phase animation through one reused app runtime;
3. resize smoke with dash atlas and marker resources;
4. panzoom tests proving pixel-width strokes stay pixel-width;
5. world-width tests proving 3D/world strokes scale with the camera.
6. tractography smoke tests with many variable-length streamlines, camera rotation, clipping, and
   stable source-id picking.

Manual examples:

1. `hello_segment_glfw`;
2. `hello_path_dash_glfw`;
3. `hello_arrows_glfw`;
4. `hello_vector_field_glfw`;
5. `hello_svg_subset`;
6. `hello_streamline_glfw`;
7. `hello_tube_glfw`;
8. `hello_tractography_glfw`;
9. ports of the useful v0.3 examples: `examples/visuals/segment.py`, `examples/visuals/path.py`,
   `examples/visuals/marker.py`, `examples/features/segment_cap.py`, and
   `examples/features/marker_mode.py`.


## Main Risks

1. **Join correctness:** acute joins, closed paths, and miter fallback can create cracks or overlaps.
   Keep bevel/miter before round joins.
2. **Dash continuity:** dashes must use cumulative path length across segments and subpaths. Avoid
   segment-local dash reset except when explicitly requested.
3. **Capability pressure:** dash atlases and SVG fills add texture/descriptor pressure. Reuse the
   existing descriptor-refresh work and add resize/recreate tests early.
4. **Transparency:** stroked paths with alpha need ordinary blending first. WBOIT/depth peeling can
   be supported later through visual pass capabilities.
5. **SVG scope creep:** browser-compatible SVG is a large project. Keep the first subset focused on
   static scientific overlays, icons, markers, and plot annotations.
6. **Performance:** dynamic paths should update only changed source attributes and derived buffers.
   Dash phase, stroke width, and color should be uniform/material updates when possible.
7. **Porting debt:** v0.3 code uses the old batch/visual abstraction, direct allocation in places,
   point-size marker sprites, and legacy shader includes. Port the behavior, not the architecture.
8. **Tractography density:** true tubes for every streamline can be too expensive. Keep ribbon/strip
   mode as the dense default and reserve tube mesh mode for selected bundles or quality renders.
9. **Tube frame stability:** naive Frenet frames twist or fail on straight segments. Use
   parallel-transport frames and add rotation-stability tests.


## Recommended Order

1. v0.3 audit and shared stroke vocabulary.
2. Port segment visual with analytic caps.
3. Port stroked path adjacency, miter limits, and round/square joins.
4. Add cumulative length and dashing with a simple uniform pattern, then dash atlas.
5. Port code-based markers and attach arrow markers to segment/path.
6. Vector field visual.
7. SVG path parser and curve flattening.
8. Fill tessellation and SVG subset import.
9. Dense 3D streamlines/ribbons for tractography.
10. Tube meshes and 3D arrow/glyph instances for selected/high-quality 3D paths.

This order gives useful scientific visuals early while keeping the hard SVG and 3D geometry problems
behind a stable stroke backend.
