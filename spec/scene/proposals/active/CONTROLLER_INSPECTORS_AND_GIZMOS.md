> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the intended boundary for controller inspector widgets, 3D gizmos,
>   and surface-plot convenience APIs before implementation work spreads across UI,
>   controller, geometry, and mesh modules.

# Controller Inspectors, Gizmos, And Surface Plots

## Objective

This proposal records three related but distinct user-facing conveniences:

1. small controller widgets for panzoom, arcball, turntable, fly, and camera state;
2. 3D orientation and transform gizmos;
3. surface-plot helpers built from structured height fields.

All three are useful, but none should punch through the v0.4 scene architecture. They should be
specified as thin layers over scene-owned state, controller state, geometry utilities, and mesh
visuals.


## Controller Widgets

Controller widgets are external UI inspectors, not scene primitives.

Examples:

1. a panzoom panel showing pan and zoom sliders;
2. an arcball panel showing Euler angles, quaternion, zoom, and pan-center controls;
3. a turntable panel showing yaw, pitch, distance, and pivot;
4. a fly-camera panel showing position, yaw, pitch, roll, movement speed, and pivot state;
5. a camera panel showing eye, target, up vector, FOV, near, and far planes.

The widget may be implemented with ImGui, a native toolkit, a web UI, Python, or another host
framework. The scene should not care which toolkit renders the controls.


### Ownership Rule

The controller remains the source of truth.

The widget may:

1. read controller state;
2. display derived values;
3. edit values through public controller setters;
4. request redraw after a state change.

The widget must not:

1. own camera math;
2. write panel matrices directly;
3. bypass controller invalidation;
4. become a visual family;
5. become a hidden DRP2 overlay path.


### State Snapshots

The recommended API shape is family-specific state snapshots:

```text
dvz_panzoom_get_state(panzoom, &state)
dvz_panzoom_set_state(panzoom, &state)

dvz_arcball_get_state(arcball, &state)
dvz_arcball_set_state(arcball, &state)

dvz_turntable_get_state(turntable, &state)
dvz_turntable_set_state(turntable, &state)

dvz_fly_get_state(fly, &state)
dvz_fly_set_state(fly, &state)
```

Each state struct should contain the semantic controller values, not cached matrices. Matrices are
derived outputs and should continue to be produced by the controller/camera update path.

The first implementation can expose direct getters and setters before the full state structs exist,
but the long-term public surface should converge on explicit state structs.


### Invalidations

Setting controller state through an inspector is equivalent to a user input gesture.

It should:

1. update the controller;
2. apply attached camera or panel state when relevant;
3. mark `PanelTransformDirty`;
4. mark axis/layout dirty only when the visible domain changes;
5. request redraw.

It should not rebuild unrelated visual resources.


## 3D Gizmos

The v0.3 gizmo implementation is a good geometry baseline: three colored arrows are generated as
ordinary mesh geometry and merged into one shape.

The v0.4 design should keep the useful geometry idea while separating two products:

1. orientation gizmo or axis triad;
2. interactive transform gizmo.

Because the v0.4 branch accepts API-breaking cleanup, the first public C surface should not preserve
the ambiguous old `panel.gizmo()` naming as a contract. Use `orientation gizmo` for the non-editing
view-orientation widget and reserve `transform gizmo` for future editing handles. Bindings may later
choose a shorter convenience name, but the C API and specs should keep the two products explicit.


### Orientation Gizmo

An orientation gizmo is a small non-editing axis triad that shows view orientation.

Recommended properties:

1. generated from `geom` arrows or a dedicated `dvz_geom_gizmo_axes()` helper;
2. rendered in a small fixed viewport or overlay panel;
3. follows the main 3D view rotation through a rotation-only controller state link;
4. ignores main view translation and zoom;
5. uses unlit or lightly lit colored mesh geometry;
6. stays outside the main panel depth range;
7. is optional scene UI, not required for mesh rendering.

This belongs above raw geometry and below generic application UI: geometry generation happens in
`geom`, while placement and synchronization happen in the scene/app layer.

Recommended first implementation slice:

1. add a retained `DvzOrientationGizmo` scene object, attached to one panel;
2. generate the triad with `dvz_geom_gizmo_axes()` once `geom` is available, or with the nearest
   active geometry helper as a temporary internal bridge;
3. render into a small panel-local inset or overlay viewport, not into the main world depth range;
4. synchronize only orientation from the panel's 3D controller or camera state;
5. ignore main view pan, target translation, zoom, and distance for placement;
6. use unlit colored mesh geometry and deterministic X/Y/Z colors;
7. request redraw when the source controller/camera orientation changes;
8. expose visibility, anchor, size, and source-controller/camera binding controls before adding
   styling knobs.

Candidate C entry points should keep orientation and editing semantics distinct:

```c
DvzOrientationGizmo* dvz_panel_orientation_gizmo(DvzPanel* panel);
void dvz_orientation_gizmo_visible(DvzOrientationGizmo* gizmo, bool visible);
```

Reserve `DvzTransformGizmo` for future pickable editing handles.

Focused first-slice tests should cover lifetime with the owning panel, source controller binding,
visibility toggles, deterministic inset sizing, and rotation-only transform derivation from
arcball and turntable sources.

The first slice should support arcball and turntable orientation sources. Camera/fly support may
follow once camera state snapshots are explicit enough to read orientation without duplicating
controller math.

Implementation note: this should use the general controller state-link model in
[`../../interaction/CONTROLLERS.md`](../../interaction/CONTROLLERS.md), not an
orientation-gizmo-specific or arcball-specific synchronization API. The source and gizmo should be
distinct controllers linked by orientation only so the gizmo keeps its own viewport/camera
evaluation and remains centered in its inset.


### Interactive Transform Gizmo

An interactive transform gizmo is a different feature.

It has pickable handles and mutates object transform, pivot, or controller target state. It should
therefore be modeled as controller or annotation behavior using pickable helper geometry, not as a
plain mesh convenience.

The transform gizmo should wait until object transforms, picking identities, and selection state
are stable enough to avoid inventing a parallel editing path.

When it is implemented, it should use a distinct public name such as `DvzTransformGizmo`, not
`DvzGizmo`, so code cannot accidentally treat a passive orientation widget as an editing tool.


## Surface Plots

Surface plots are structured-grid mesh conveniences.

The v0.3 path generated mesh positions, indices, colors, texture coordinates, and normals from:

1. row count;
2. column count;
3. height array;
4. optional color array;
5. origin vector;
6. two grid basis vectors;
7. optional contour/indexing preprocessing.

That remains the right conceptual model, but the ownership should move.


### Recommended v0.4 Ownership

Surface-grid generation belongs in `geom`.

The mesh visual should render the resulting geometry. The scene may expose a convenience helper,
but internally it should still create or update a mesh geometry resource.

Recommended layers:

1. `geom`: generate structured-grid geometry from height/scalar arrays;
2. scene resource model: retain the uploaded mesh resource and any structured-grid provenance;
3. `mesh` visual: render lighting, colormap, texture, edge overlay, material, and isolines;
4. app/UI: provide sliders and presets for colormap, height scale, contour mode, and camera.


### Structured Provenance

A surface plot can be rendered as an ordinary triangle mesh, but the scene should keep optional
structured-grid provenance when possible.

This provenance is useful for:

1. efficient height-only updates;
2. normal recomputation after height changes;
3. row/column edge overlays;
4. surface-specific contours and isolines;
5. future level-of-detail or tiled updates.

The public API can initially expose a convenience constructor, but the internal representation
should not permanently collapse the surface into anonymous triangles if the source is a structured
grid.


### API Direction

The low-level path should look like:

```text
DvzGeometry* geom = dvz_geometry_surface_grid(&desc);
DvzMeshResource* mesh = dvz_scene_mesh_resource(scene, geom);
DvzVisual* visual = dvz_mesh(scene, mesh, &mesh_desc);
```

A higher-level convenience may wrap that:

```text
DvzVisual* visual = dvz_surface(scene, &surface_desc);
```

The convenience helper should not define a separate surface rendering backend. It is a mesh visual
with structured-grid construction semantics.


## Promotion Targets

This proposal should be promoted into:

1. `../../integration/EXTERNAL_UI.md` for controller widget ownership;
2. `../../interaction/CONTROLLERS.md` for controller state get/set contracts;
3. `../../semantics/GEOMETRY_UTILITIES.md` for gizmo and surface-grid generation;
4. `../../proposals/promoted/GEOM_DESIGN.md` for `DvzGeometry` generator scope;
5. `../../visuals/MESH.md` for surface-plot rendering semantics.
