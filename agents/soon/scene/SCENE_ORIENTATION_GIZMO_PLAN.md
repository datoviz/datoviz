# Scene Orientation Gizmo Plan

> **Execution Status**
> - **Status:** `SOON`
> - **Updated on:** `2026-05-21`
> - **Purpose:** define the first implementation slice for a v0.4 orientation gizmo without
>   pulling in transform-editing handles or legacy v0.3 API compatibility.


## Scope

Implement a passive 3D orientation gizmo: a small colored axis triad that reflects the current
orientation of a 3D panel.

This is not the interactive transform gizmo. It must not mutate object transforms, controller
targets, selection state, or scene data.


## Source Specs

1. [../../../spec/scene/proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md](../../../spec/scene/proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md)
2. [../../../spec/scene/semantics/GEOMETRY_UTILITIES.md](../../../spec/scene/semantics/GEOMETRY_UTILITIES.md)
3. [../../../spec/scene/api/API_SURFACE.md](../../../spec/scene/api/API_SURFACE.md)


## API Direction

Use explicit naming:

```c
DvzOrientationGizmo
dvz_panel_orientation_gizmo(...)
```

Do not use `DvzGizmo` as the first public handle. Reserve future naming room for
`DvzTransformGizmo` if editing handles are added later.

The old v0.3-style `panel.gizmo()` is only a legacy ergonomics reference. v0.4 C does not need to
preserve it.


## First Slice

1. Add the retained scene object and panel attachment state.
2. Build X/Y/Z arrow geometry as ordinary mesh data.
3. Render the mesh in a small panel-local inset or overlay viewport.
4. Synchronize rotation from an arcball or turntable controller.
5. Ignore pan, zoom, distance, and translation.
6. Keep the gizmo outside the main scene depth range.
7. Request redraw when the source orientation changes.
8. Add focused tests for object lifetime, source binding, visibility toggles, and transform
   derivation.


## Geometry

Prefer `dvz_geom_gizmo_axes()` once that generator lands in the active `geom` module. Until then,
keep any temporary triad generator internal and shaped so it can move to `geom` without changing
scene semantics.

The generated geometry should be deterministic:

1. X axis: red;
2. Y axis: green;
3. Z axis: blue;
4. unlit material by default;
5. configurable length and shaft/head proportions.


## Validation

For documentation-only edits, run `git diff --check`.

For implementation:

1. run `just build`;
2. run focused scene tests for panel attachment and controller binding;
3. run an offscreen or GLFW smoke that verifies the gizmo renders above/alongside a depth-enabled
   mesh without being hidden by world geometry;
4. use Vulkan validation for any render-pass, viewport/scissor, depth-state, or overlay path
   changes.


## Deferrals

Defer these until the passive orientation gizmo is stable:

1. interactive transform handles;
2. object selection integration;
3. GPU picking for handles;
4. snapping and constraints;
5. multi-object editing;
6. fly-camera support if it requires duplicating camera/controller math.
