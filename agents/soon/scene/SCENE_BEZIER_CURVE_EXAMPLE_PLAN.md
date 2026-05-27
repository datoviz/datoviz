# Scene Bezier Curve Example Plan

> **Execution Status**
> - **Status:** `ACTIVE / EXAMPLE PLAN`
> - **Updated on:** `2026-05-27`
> - **Purpose:** track the first curve-tessellation example slice for the existing path visual.


## Current State

Datoviz does not currently have a dedicated Bezier or curve-tessellation C example. The closest
examples are:

1. `examples/c/visuals/path.c`: retained `dvz_path()` stress coverage for sampled polyline data,
   subpaths, stroke width, caps, joins, and live updates.
2. `examples/c/visuals/polygon.c`: polygon triangulation and triangulation-overlay coverage through
   `dvz_triangulate_polygon()`, but not curve tessellation.

Durable contracts and intended semantics live in:

1. [`../../../spec/scene/semantics/GEOMETRY_UTILITIES.md`](../../../spec/scene/semantics/GEOMETRY_UTILITIES.md)
2. [`../../../spec/scene/visuals/PATH.md`](../../../spec/scene/visuals/PATH.md)
3. [`SCENE_VECTOR_VISUALS_PLAN.md`](SCENE_VECTOR_VISUALS_PLAN.md)

Keep analytic curves as CPU-side geometry preparation. Do not introduce a separate scene `curve`
visual for the v0.4 baseline; tessellated curve output should lower to ordinary `dvz_path()` data.


## Recommended First Example

Add `examples/c/visuals/bezier.c` as the first slice.

The example should make the intended model explicit:

1. Bezier controls are CPU-side input data.
2. Tessellation produces ordinary polyline points.
3. Rendering uses the existing `dvz_path()` visual.
4. Control polygons use `dvz_segment()`.
5. Control points use `dvz_marker()` or `dvz_point()`.
6. One path visual can hold multiple tessellated curves through `dvz_path_set_subpaths()`.


## Scope

The first version should demonstrate:

1. one cubic Bezier curve with four control points;
2. one quadratic Bezier curve with three control points;
3. multiple curves uploaded as one `dvz_path()` payload with explicit subpath lengths;
4. path stroke width, caps, and joins using the existing path styling API;
5. optional overlays for control points and control polygon segments;
6. a small GUI for tessellation quality, stroke width, join mode, and overlay visibility.

If public `dvz_tessellate_bezier_*()` helpers exist before this example lands, use those helpers.
If they do not exist yet, keep any tessellation code example-local and clearly static, and name the
example around the rendering path rather than presenting a stable public tessellation API.


## Implementation Order

Recommended commits:

1. Add focused `geom` helpers first if the public API is ready:
   `dvz_tessellate_bezier_quadratic()` and `dvz_tessellate_bezier_cubic()`, with allocator-safe
   output and narrow tests.
2. Add `examples/c/visuals/bezier.c`, using either the new helpers or example-local static
   tessellation.
3. Register the example in `examples/c/CMakeLists.txt` and example coverage docs.
4. Add an offscreen or bounded live smoke path if the example has frame-count support.

Do not block the first example on Catmull-Rom, B-spline, dashes, closed-path API, curve picking, or
WGSL parity. Those belong to later vector/path follow-up work.


## Later Example

After Bezier helpers and the Bezier example are stable, add a broader
`examples/c/visuals/curve_tessellation.c` only if it can compare multiple analytic curve families:

1. quadratic and cubic Bezier;
2. Catmull-Rom;
3. uniform cubic B-spline;
4. fixed-step versus tolerance/adaptive output where available.

That broader example should still render through `dvz_path()` rather than introducing a new visual
family.


## Validation

For a plan-only edit:

```text
git diff --check
```

For the eventual example or helper implementation:

```text
just build
just example-c visuals/bezier
just test geom
just test scene
git diff --check
```

For shader/runtime changes beyond path data upload, also run the relevant path-focused scene tests
and a bounded GLFW smoke on a Vulkan-capable runtime.
