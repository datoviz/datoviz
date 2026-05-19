# Scene Pixel, Point, Marker Implementation Plan

> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED / FOLLOW-UP PLAN`
> - **Updated on:** `2026-05-17`
> - **Purpose:** define the staged v0.4 path for full pixel, point, marker, bitmap-marker,
>   MSDF-marker, and glyph/MSDF support through the active scene -> DRP2 -> runtime stack.


## Summary

The v0.4 scene layer should keep the three point-like visual families separate by intent:

1. `pixel`: the cheapest square sprite for dense data.
2. `point`: a styled circular sprite for ordinary scatter and point clouds.
3. `marker`: a symbolic sprite visual for categorical shapes, icons, bitmap markers, and
   atlas-backed SDF/MSDF symbols.

Bitmap markers belong in the marker visual as a marker mode, not as a separate visual family. Glyphs
and MSDF markers should share atlas/MSDF infrastructure, but glyph/text should remain a separate
visual because text layout has different semantics from scatter-symbol rendering.

Implementation is happening in small slices. The first pixel, point, and code-SDF marker slices have
landed. Remaining marker work should move to exact picking, bitmap mode, atlas mode, then shared
MSDF infrastructure used by both marker and glyph/text.


## Current v0.4 Position

Already present:

1. `dvz_point()` and `dvz_pixel()` are public scene constructors.
2. Point-like visuals already use `position`, `color`, and `size` attributes.
3. `dvz_marker()` is a public scene constructor for code-SDF markers.
4. Scene frame-plan metadata recognizes point-like visual types, including
   `DVZ_VISUAL_TYPE_MARKER`.
5. GLSL and WGSL shader files exist for `point`, `pixel`, and depth-cue variants.
6. GLSL marker shader files exist for the code-SDF marker slice.
7. WGSL point/pixel lowering uses instanced quads because WebGPU has no native point-list size
   equivalent.
8. GLSL point/pixel/marker lowering currently uses native point-list semantics.
9. Point picking exists for `DVZ_VISUAL_TYPE_POINT` through the scene request path.
10. Pixel picking exists with square hit areas.
11. Marker picking exists with marker sprite bounding-box hit areas.
12. Depth cueing, EDL, alpha-mode planning, WBOIT/depth-peel graph routing, and runtime reuse
    already know about point-like visuals in some form.

First-slice marker capabilities:

1. dense `position`, `color`, `size`, `angle`, and `shape`;
2. `DvzMarkerShape` values stored as `uint32_t`;
3. built-in shapes `disc`, `square`, `triangle`, `diamond`, `cross`, and `ring`;
4. `DvzMarkerStyle` with `edge_color`, `line_width`, `filled`, `stroke`, and `outline`;
5. code-SDF mode only.

Gaps:

1. Exact marker shape-mask picking is not implemented yet; the active pick area is the marker sprite
   bounding box.
2. Bitmap/SDF/MSDF marker support does not yet have an atlas/resource substrate in the active v0.4
   scene path.
3. Glyph/text bookkeeping exists, but GPU-backed glyph rendering and shared MSDF atlas support are
   not active yet.
4. Constant/grouped/scalar attribute-source variants are still target-contract work for these
   families.
5. Data-space sizing and `shift` are still target-contract work.


## Next Concrete Steps

Immediate point-like follow-up should prioritize marker picking quality, then marker resource modes:

1. Add exact code-SDF marker picking. Reuse the current GPU-backed marker bounding-box path as the
   broad hit area, then apply the same built-in shape SDF used by rendering before returning a hit.
   Keep the request result identity as the marker item index.
2. Add focused tests for false positives at transparent marker corners and holes, especially
   triangle, diamond, cross, and ring.
3. Keep pixel square picking and point circular picking as the reference behavior for point-like
   family-specific hit masks.
4. Defer bitmap/SDF/MSDF marker picking until those render modes have real texture/atlas resource
   paths.

The cross-family execution order is recorded in
`spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`.


## Design Principles

1. Keep `pixel` fast and visually plain.
2. Keep `point` focused on circular scatter points, not arbitrary symbols.
3. Keep `marker` as the symbolic sprite visual, including bitmap and SDF/MSDF modes.
4. Do not create a parallel renderer, presentation path, or Vulkan wrapper.
5. Lower all visuals through scene frame plans, DRP2 streams, and the existing vklite/canvas runtime.
6. Use shared atlas/MSDF infrastructure for marker and glyph, while keeping their public visuals
   separate.
7. Preserve WebGPU portability: every point-like semantic should have a clean instanced-quad lowering.
8. Prefer shader-coded marker shapes before bitmap/MSDF, because code-SDF needs no asset pipeline.
9. Treat picking/hit testing as part of the visual contract, not a later add-on.
10. Keep the first marker API useful but narrow; do not port every v0.3 mode at once.


## Visual Semantics

### Pixel Visual

Purpose: dense point clouds and raster-like unstructured data where throughput matters more than
styling.

Contract:

1. required `position`: per-item `vec3f`.
2. required `color`: per-item `RGBA8`, with constant and grouped color allowed when the shared
   attribute-source machinery supports it.
3. `size`: side length in framebuffer pixels; allow per-item and constant forms.
4. shape: filled axis-aligned square.
5. antialiasing: none in the first contract.
6. stroke/edge: none.
7. rotation: none.
8. texture: none.
9. depth: can depth-test and depth-write when attached through a normal controller.
10. alpha: follows existing visual alpha mode routing.

Important default:

1. Missing `size` should eventually default to `1.0f` so callers can upload only position and color.
2. Until default attributes are implemented, tests should keep using explicit size data.


### Point Visual

Purpose: ordinary scatter points and point clouds where visual quality matters but symbolic shapes do
not.

Contract:

1. required `position`: per-item `vec3f`.
2. required `color`: face color, per-item `RGBA8`.
3. `size`: diameter in framebuffer pixels; allow per-item and constant forms.
4. shape: circular disc.
5. antialiasing: enabled by default in the fragment shader.
6. optional uniform `edge_color`: `RGBA8` or normalized `vec4`.
7. optional uniform `line_width`: stroke width in framebuffer pixels.
8. no arbitrary shape selection.
9. no bitmap texture.
10. no marker rotation.
11. depth: can depth-test and depth-write when attached through a normal controller.
12. alpha: follows existing visual alpha mode routing.

The point visual should become a true antialiased disc on every backend. GLSL can initially use
`gl_PointCoord`, but instanced-quad lowering should be considered if exact GLSL/WGSL parity or
future WebGPU-native alignment becomes more important than native point-list throughput.


### Marker Visual

Purpose: categorical/symbolic scatter points, icons, and custom marker glyphs.

Contract:

1. required `position`: per-item `vec3f`.
2. required `color`: fill/tint color, per-item `RGBA8`.
3. `size`: screen-space marker diameter in framebuffer pixels; allow per-item and constant forms.
4. `angle`: marker rotation in radians; allow per-item and constant forms, default `0`.
5. `shape`: built-in symbolic shape or atlas symbol id; support per-item shape eventually, with a
   per-visual default as convenience.
6. style: `filled`, `stroke`, and `outline`.
7. optional uniform `edge_color`.
8. optional uniform `line_width`.
9. mode: `code`, `bitmap`, `sdf`, `msdf`.
10. depth: can depth-test and depth-write using the marker center depth for the first implementation.
11. alpha: follows existing visual alpha mode routing.

The first marker slice should support only `code` mode. Bitmap and atlas modes should come next.
SDF/MSDF should wait until the shared atlas substrate is in place.


## Marker Modes

### Code Mode

Code mode evaluates built-in signed-distance marker functions directly in the fragment shader. This
is the best first implementation because it needs no texture binding, no atlas, no external asset
pipeline, and no generator dependency.

Initial shape subset:

1. disc,
2. square,
3. triangle,
4. diamond,
5. cross,
6. ring.

Later shape expansion can reuse v0.3's GLSL SDF functions for asterisk, chevron, clover, club,
arrow, ellipse, hbar, heart, infinity, pin, spade, tag, vbar, and rounded rectangle.


### Bitmap Mode

Bitmap mode samples a normal RGBA or alpha texture and applies marker color as tint/alpha according
to the selected policy.

Bitmap mode should be part of `marker`, not a separate visual, because it shares marker's core
semantics:

1. one symbol per scene item,
2. screen-space size,
3. optional rotation,
4. per-item color/tint,
5. optional atlas symbol id,
6. point-like depth and alpha behavior.

First bitmap slice:

1. one bitmap texture per marker visual,
2. one UV rectangle covering the full texture,
3. all items in the visual use the same bitmap,
4. per-item color multiplies sampled color,
5. alpha comes from sampled alpha times per-item color alpha.

Second bitmap slice:

1. atlas-backed bitmap markers,
2. per-item symbol id,
3. per-symbol UV rectangle and nominal bounds,
4. support categorical icon scatter without creating one visual per icon.

A separate `icon` visual should be deferred. It is only justified if bitmap/MSDF markers later grow
layout semantics that do not fit scatter-symbol rendering.


### SDF And MSDF Modes

SDF/MSDF modes should consume shared atlas entries. The marker visual should not own a custom
font/icon pipeline.

SDF mode:

1. single-channel distance field,
2. simpler and cheaper than MSDF,
3. suitable for simple filled symbols and silhouettes.

MSDF mode:

1. RGB multichannel distance field,
2. better corners and complex icon outlines,
3. useful for custom SVG/path markers and font glyphs.

MTSDF or additional channels should remain deferred until there is a real quality need.


## Glyph And MSDF Interaction

Glyph/text and marker should share infrastructure, not public semantics.

Shared internal substrate:

1. atlas texture creation and upload,
2. atlas resizing or immutable-atlas creation policy,
3. atlas entry metadata,
4. UV rectangle lookup,
5. nominal symbol bounds,
6. SDF/MSDF decode helpers,
7. sampler setup,
8. scene resource labels and DRP2 texture/bind-group emission,
9. dirty-state and descriptor-refresh behavior,
10. optional offline or import-time SVG/path/font-to-MSDF generation.

Marker-specific interpretation:

1. entry id means symbol/icon/marker shape,
2. position is data point position,
3. size is marker diameter in framebuffer pixels,
4. angle rotates the symbol around its center,
5. no baseline,
6. no advance,
7. no shaping,
8. color is fill/tint.

Glyph/text-specific interpretation:

1. entry id means glyph id/codepoint within a font face,
2. placement follows text layout,
3. advances, bearings, baseline, line height, and font metrics matter,
4. shaping and fallback may eventually be required,
5. atlas bounds are used for glyph quads, not marker diameters.

Therefore:

1. Keep `marker` and `glyph/text` as separate visuals.
2. Put atlas/MSDF resource ownership in shared scene internals.
3. Put MSDF shader decode helpers in shared GLSL/WGSL includes.
4. Let marker and glyph have separate vertex inputs and draw semantics.


## Proposed Public API Shape

Exact naming can change during implementation, but the first API should look roughly like this.

```c
typedef enum
{
    DVZ_MARKER_MODE_CODE = 0,
    DVZ_MARKER_MODE_BITMAP,
    DVZ_MARKER_MODE_SDF,
    DVZ_MARKER_MODE_MSDF,
} DvzMarkerMode;

typedef enum
{
    DVZ_MARKER_ASPECT_FILLED = 0,
    DVZ_MARKER_ASPECT_STROKE,
    DVZ_MARKER_ASPECT_OUTLINE,
} DvzMarkerAspect;

typedef enum
{
    DVZ_MARKER_SHAPE_DISC = 0,
    DVZ_MARKER_SHAPE_SQUARE,
    DVZ_MARKER_SHAPE_TRIANGLE,
    DVZ_MARKER_SHAPE_DIAMOND,
    DVZ_MARKER_SHAPE_CROSS,
    DVZ_MARKER_SHAPE_RING,
} DvzMarkerShape;

typedef struct DvzMarkerStyle
{
    DvzMarkerMode mode;
    DvzMarkerAspect aspect;
    DvzMarkerShape default_shape;
    float edge_color[4];
    float line_width;
} DvzMarkerStyle;

DVZ_EXPORT DvzVisual* dvz_marker(DvzScene* scene, uint32_t flags);
DVZ_EXPORT int dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style);
DVZ_EXPORT int dvz_marker_set_bitmap(DvzVisual* visual, const DvzMarkerBitmapDesc* bitmap);
DVZ_EXPORT int dvz_marker_set_atlas(DvzVisual* visual, DvzSceneAtlas* atlas);
```

Attribute names:

1. `position`: `vec3f`, required.
2. `color`: `RGBA8`, required.
3. `size`: `float`, optional/defaulted eventually.
4. `angle`: `float`, optional/defaulted eventually.
5. `shape`: `uint32_t`, optional for code mode.
6. `symbol`: `uint32_t`, optional for atlas-backed bitmap/SDF/MSDF modes.

For points, style can be narrower:

```c
typedef struct DvzPointStyle
{
    float edge_color[4];
    float line_width;
} DvzPointStyle;

DVZ_EXPORT int dvz_point_set_style(DvzVisual* visual, const DvzPointStyle* style);
```

Do not overload `dvz_visual_set_primitive_shading()` for point or marker styling. Stroke and marker
style are not lighting/material concerns.


## Internal Data Model

Extend `DvzVisual` with point-like style state rather than reusing primitive material parameters for
non-material data.

Suggested internal state:

1. `DvzPointStyleState point_style`.
2. `DvzMarkerState marker`.
3. dirty/version counters for style buffers.
4. optional texture/atlas binding state for marker bitmap/SDF/MSDF.
5. per-visual default shape/symbol.

Do not put marker styling into file-scope mutable state. Keep all state owned by scene objects and
visuals so tests and multiple scenes remain isolated.


## Shader And Pipeline Direction

### Pixel

GLSL:

1. native point-list path is acceptable for the first pass.
2. fragment shader outputs color directly.
3. no antialiasing.

WGSL:

1. keep instanced-quad lowering.
2. fragment shader outputs color directly.


### Point

Preferred long-term canonical lowering:

1. instanced quad on all backends,
2. fragment receives local `corner` coordinates,
3. SDF circle computes alpha coverage,
4. line width and edge color are applied in one helper.

Acceptable transitional GLSL path:

1. native point-list,
2. use `gl_PointCoord` for local coordinates,
3. keep the same SDF/stroke helper as WGSL.


### Marker

Code mode:

1. vertex shader expands a point-like item to a sprite quad, or native point plus `gl_PointCoord`
   during an early GLSL-only step.
2. fragment shader evaluates a selected signed-distance function.
3. style helper applies filled/stroke/outline.
4. per-item shape should be supported by an integer attribute after the first per-visual shape slice.

Bitmap mode:

1. vertex shader computes sprite quad UVs.
2. fragment shader samples `sampler2D`.
3. per-item color tints sampled color.
4. atlas mode replaces full-texture UVs with per-symbol UV rects.

SDF/MSDF modes:

1. share decode helpers with glyph.
2. consume atlas metadata.
3. use screen-space derivative or explicit pixel range parameters for antialiasing.


## Picking And Hit Testing

Point-like picking should be generalized from point-only to:

1. pixel square hit testing,
2. point circular hit testing,
3. marker code-SDF hit testing for supported shapes,
4. bitmap alpha-threshold hit testing for bitmap markers,
5. SDF/MSDF distance-threshold hit testing for atlas markers.

Staged approach:

1. CPU hit testing for pixel/point using position, size, panel transform, and simple masks.
2. GPU picking for point-like visuals using a dedicated picking pass.
3. For marker code mode, use the same SDF mask in pick shader as color shader.
4. For bitmap/SDF/MSDF markers, sample the marker texture/atlas in the pick shader and reject
   transparent/outside fragments.

For the first marker slice, center-depth picking is acceptable. Per-fragment analytic marker depth is
not needed because markers are screen-space symbols, not 3D surfaces.


## Staged Implementation

### Stage 1 - Pixel Contract Hardening

Expected work:

1. Make the pixel public docs match the actual v0.4 retained API.
2. Keep pixel attributes to `position`, `color`, and `size`.
3. Add or tighten GLSL and WGSL emission tests.
4. Add one runtime/offscreen smoke that confirms pixel draws nonblank squares.
5. Add a small `hello_pixel.c` or adapt an existing dense-pixel example into the active examples list.
6. Add pixel point-like picking with square hit area if interaction work is in scope for the slice.

Validation:

```text
git diff --check
just build
just test scene
```


### Stage 2 - Antialiased Point Disc

Expected work:

1. Add point style state for edge color and line width.
2. Add a public point style setter.
3. Update point GLSL/WGSL shaders to render an antialiased disc.
4. Add stroke/outline handling or keep stroke initially disabled behind style defaults.
5. Ensure depth cue variants share the same shape mask and antialias coverage.
6. Keep EDL/depth post-process eligibility unchanged unless tests prove otherwise.
7. Add offscreen pixel checks around center, edge, and outside fragments.
8. Add point picking that respects circular size.

Validation:

```text
git diff --check
just build
just test scene
just test app
```


### Stage 3 - Marker Code-SDF First Slice

Expected work:

1. Add public marker enums and `dvz_marker()`.
2. Add marker style state and setter.
3. Add marker attributes: `position`, `color`, `size`, `angle`.
4. Add optional per-visual default shape.
5. Add marker shader registry entries.
6. Add GLSL and WGSL code-SDF shaders for the initial shape subset.
7. Route marker through point-like frame-plan metadata.
8. Add marker pipeline, bind, pass-capability, alpha-mode, and depth tests.
9. Add nonblank offscreen runtime smoke for several shapes.
10. Add a `hello_marker_glfw.c` example with GUI controls only after the fixed smoke works.

Validation:

```text
git diff --check
just build
just test scene
just test app
```


### Stage 4 - Marker Bitmap Mode

Expected work:

1. Add marker bitmap state and one-texture-per-visual binding.
2. Reuse scene texture/field upload paths where possible.
3. Add marker bitmap GLSL/WGSL shader variants.
4. Add descriptor refresh coverage for texture replacement and resize.
5. Add offscreen runtime smoke with transparent and opaque bitmap regions.
6. Add bitmap-aware picking by alpha threshold.

Validation:

```text
git diff --check
just build
just test scene
just test app
```


### Stage 5 - Shared Atlas Substrate

Expected work:

1. Add a scene-owned atlas object with texture payload, entries, versioning, and dirty ranges.
2. Add atlas entry metadata: UV rect, bounds, nominal size, type, and optional user symbol id.
3. Add marker atlas binding and per-item `symbol` attribute.
4. Emit atlas texture creation/upload through DRP2.
5. Add descriptor refresh tests for atlas recreation and update.
6. Add bitmap-atlas marker smoke with multiple symbols in one visual.
7. Keep glyph-specific metrics out of marker-facing APIs.

Validation:

```text
git diff --check
just build
just test scene
just test app
just spec-check
```


### Stage 6 - Shared SDF/MSDF Decode

Expected work:

1. Add shared GLSL/WGSL SDF/MSDF decode helpers.
2. Add atlas entry fields needed for distance-field pixel range.
3. Add marker `SDF` and `MSDF` modes using the shared atlas.
4. Add quality tests for scale changes and sharp corners.
5. Add alpha/stroke tests for MSDF markers.
6. Add portable recording/replay coverage if new resource commands are introduced.

Validation:

```text
git diff --check
just build
just test scene
just test drp2
just spec-check
```


### Stage 7 - Glyph/Text GPU Rendering

Expected work:

1. Keep text/glyph as a separate visual or retained text object.
2. Reuse the shared atlas and MSDF decode helpers.
3. Add glyph metrics: advance, bearing, baseline, line height, and bounds.
4. Add first simple text layout without shaping.
5. Add glyph/text shaders that consume atlas entries but use text-specific placement.
6. Add offscreen text smoke and atlas reuse tests.
7. Defer shaping, fallback, bidi, rich text, and dynamic font loading until basic GPU text is stable.

Validation:

```text
git diff --check
just build
just test scene
just test app
```


## Test Matrix

Pixel:

1. GLSL emit command shape.
2. WGSL emit command shape.
3. offscreen nonblank square smoke.
4. depth cue toggle does not accidentally switch to point shader semantics.
5. optional square picking.

Point:

1. GLSL antialiased disc center/edge/outside pixel smoke.
2. WGSL antialiased disc command shape.
3. edge color and line width style update causes only expected resource updates.
4. depth cue and antialiasing combine correctly.
5. EDL eligibility remains valid.
6. circular picking.

Marker code:

1. constructor and attribute validation.
2. shape enum values match shader expectations.
3. style setter updates retained state.
4. filled/stroke/outline output.
5. several shape masks render nonblank.
6. per-item angle rotates visibly in a smoke test or fixture.
7. marker picking respects shape masks.

Marker bitmap:

1. texture upload and binding.
2. alpha-masked transparent pixels discard.
3. color tint.
4. texture replacement refreshes descriptors.
5. bitmap picking rejects transparent fragments.

Atlas/MSDF:

1. atlas creation/upload/update.
2. multiple marker symbols in one visual.
3. descriptor refresh after atlas resize/recreate.
4. SDF/MSDF decode helper parity in GLSL/WGSL.
5. glyph and marker can share one atlas without crossing public semantics.


## Risks And Decisions

### Native Points Versus Instanced Quads

Native points are attractive for throughput on Vulkan, but portability and exact shape semantics favor
instanced quads. The first point cleanup can keep native GLSL points if it uses `gl_PointCoord` and
matches WGSL visually. If backend divergence becomes a recurring source of bugs, move all point-like
families to instanced quads and treat native points as an optional optimization.


### Per-Item Shape

Per-item marker shape is useful for categorical scatter. It may increase shader branching, but the
API should allow it. Performance-sensitive users can group by shape into separate visuals if needed.


### Bitmap As Marker Mode

Bitmap should be a marker mode because it shares marker's item contract. A separate bitmap/icon visual
would create a duplicate API for position, size, angle, color, depth, alpha, and picking. Reconsider
only if bitmap icons later need layout semantics that markers do not have.


### Glyph And Marker Sharing

Sharing atlas/MSDF code is desirable. Sharing one visual is not. Glyph/text needs shaping, metrics,
advance, baseline, and layout. Marker needs scatter-symbol behavior. Keep those public models
separate and share only the atlas/texture/decode substrate.


### SVG To MSDF

SVG-to-MSDF generation is an asset-pipeline feature, not the first rendering feature. The first MSDF
slice should consume prebuilt atlas entries. SVG/path import can come later through tools or optional
helpers once the runtime atlas path is stable.


## Recommended Commit Order

1. Pixel contract and tests.
2. Point antialiased disc and style state.
3. Point-like picking generalization for pixel and point.
4. Marker public enums and code-SDF renderer.
5. Marker code-mode picking.
6. Marker bitmap mode with one texture per visual.
7. Scene atlas resource and atlas-backed bitmap markers.
8. Shared SDF/MSDF decode and marker MSDF mode.
9. Glyph/text GPU rendering on the shared atlas.
