# Scene Point, Pixel, And Marker Visuals

> **Execution Status**
> - **Status:** `DONE / BASELINE LANDED`
> - **Updated on:** `2026-05-21`
> - **Purpose:** record the first retained point-like visual slice and point remaining execution
>   work to the active follow-up note.


## Landed Baseline

Durable point-like visual contracts live in:

1. [`../../spec/scene/visuals/PIXEL.md`](../../spec/scene/visuals/PIXEL.md)
2. [`../../spec/scene/visuals/POINT.md`](../../spec/scene/visuals/POINT.md)
3. [`../../spec/scene/visuals/MARKER.md`](../../spec/scene/visuals/MARKER.md)
4. [`../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md`](../../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)

The active codebase has:

1. public `dvz_pixel()`, `dvz_point()`, and `dvz_marker()` constructors;
2. dense `position`/`color`/size-style attributes using the v0.4 names;
3. styled antialiased points;
4. code-SDF markers with the first built-in shape subset;
5. GLSL/Vulkan and current WGSL/WebGPU point-like lowering where supported;
6. GPU-backed picking for pixel, point, and marker broad hit regions.


## Active Follow-Up

Remaining near-term point-like work is tracked in
[`../soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md`](../soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md).
Do not duplicate stable visual contracts there.


## Validation Record

The first-slice visual-family batch recorded:

```text
just test app
just spec-check
just test
git diff --check
```
