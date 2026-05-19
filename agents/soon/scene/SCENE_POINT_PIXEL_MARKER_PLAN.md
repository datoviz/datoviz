# Scene Point, Pixel, And Marker Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining point-like visual work after the first pixel, point, and code-SDF
>   marker slices landed.


## Current State

Durable point-like visual contracts live in:

1. [`../../../spec/scene/visuals/PIXEL.md`](../../../spec/scene/visuals/PIXEL.md)
2. [`../../../spec/scene/visuals/POINT.md`](../../../spec/scene/visuals/POINT.md)
3. [`../../../spec/scene/visuals/MARKER.md`](../../../spec/scene/visuals/MARKER.md)
4. [`../../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`](../../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)

The active v0.4 code already has public `dvz_pixel()`, `dvz_point()`, and `dvz_marker()`
constructors; dense position/color/size-style attributes; styled antialiased points; code-SDF
markers with the first built-in shape subset; GLSL/Vulkan and WGSL/WebGPU point-like lowering
where currently supported; and GPU-backed picking for pixel, point, and marker bounding boxes.

Use this file only for remaining execution work. Do not duplicate stable visual contracts here.


## Remaining Point-Like Work

Recommended follow-up commits:

1. Improve marker picking from sprite bounding-box hits to exact code-SDF mask hits for the active
   built-in shapes. Include transparent corners and ring/cross holes in tests.
2. Add bitmap marker mode only after the one-texture-per-visual resource path, alpha discard, color
   tinting, descriptor refresh, and bitmap-aware picking are testable.
3. Add a scene-owned atlas substrate before atlas-backed bitmap/SDF/MSDF markers. Keep glyph metrics
   and text layout out of marker-facing APIs.
4. Add shared SDF/MSDF decode helpers for marker and text only after atlas entries carry the
   distance-field metadata needed for scale-correct antialiasing.
5. Fill remaining target-contract gaps deliberately: constant/grouped/scalar attributes, `shift`,
   data-space sizes, and default size convenience.
6. Preserve backend-aware lowering: Vulkan may keep native point lists where valuable, while WebGPU
   requires instanced quads for programmable point-like size.
7. Keep glyph/text GPU rendering in the text-layout workstream, reusing atlas/MSDF infrastructure
   but not marker visual semantics.


## v0.3 Reference

Use v0.3 as behavior reference, not architecture:

1. `v0.3/src/scene/visuals/marker.c`
2. `v0.3/src/scene/glsl/graphics_marker.vert`
3. `v0.3/src/scene/glsl/graphics_marker.frag`
4. `v0.3/include/datoviz/scene/glsl/markers.glsl`
5. `v0.3/src/scene/sdf.cpp`
6. `v0.3/examples/features/marker_mode.py`

Useful ideas to retain:

1. code-generated SDF marker shapes;
2. bitmap, SDF, MSDF, and MTSDF modes as marker render modes;
3. fill/stroke/outline aspects;
4. rotation, edge color, and edge width;
5. SVG-to-MSDF asset generation as a tool/helper path.

Avoid reviving v0.3 batch ownership, legacy shader includes, or marker/text coupling at the public
visual level.


## Validation

For point-like visual work:

```text
just build
just test scene
just test app
git diff --check
```

For atlas or new resource commands, also run the relevant DRP2/spec checks.
