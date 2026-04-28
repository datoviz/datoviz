# Scene Controllers And Interaction

This document defines how controllers and interaction should work in the future scene layer.

Controllers are scene-side state machines.

They translate panel-local or scene-level events into scene-state mutations, invalidation, and frame
scheduling.


## Purpose

The controller model should:

1. keep input and interaction policy above DRP2 and backend internals,
2. make panel-local navigation explicit,
3. give picking, hover, and selection a clean scene-level home,
4. separate event interpretation from visual rendering,
5. fit naturally with invalidation, redraw, and `FramePlan` rebuild rules.


## Position

Controllers sit between:

1. runtime-delivered input events,
2. scene-owned objects such as panels, visuals, axes, and selection state,
3. the invalidation and redraw system,
4. `FramePlan` construction for the next frame.

The intended order is:

1. events are translated into scene-level events,
2. controllers consume those events,
3. controllers mutate scene state,
4. invalidation and redraw are recorded,
5. the next frame build resolves the consequences.


## Core Rule

A controller mutates scene state.

A controller does not:

1. emit DRP2 directly,
2. record backend commands,
3. own backend synchronization or submission,
4. bypass scene invalidation rules.


## Non-Goals

This document does not define:

1. the exact runtime event adapter,
2. the exact callback signatures,
3. the exact final C API,
4. the exact threading model for event delivery,
5. low-level window-system behavior.


## Controller Scope

The scene layer should recognize at least two controller scopes:

1. panel-local controllers,
2. scene-global controllers.


### Panel-Local Controllers

These operate on one panel’s view, interaction state, or attached scene objects.

Examples:

1. 2D panzoom,
2. 3D orbit or fly camera,
3. box selection inside one panel,
4. axis interaction or panel-local probes.


### Scene-Global Controllers

These operate across the scene or coordinate multiple panels.

Examples:

1. synchronized linked panning,
2. global animation scrubbing,
3. shared selection state,
4. coordinated cursor or crosshair behaviors.


## Main Responsibilities

Controllers should be responsible for:

1. interpreting scene-level events,
2. mutating panel or scene state,
3. scheduling picking requests when needed,
4. marking the correct invalidation scopes,
5. requesting redraw when the user-visible result changed.


## Event Model

The scene spec should use scene-level events rather than raw backend events.

Useful conceptual event families include:

1. pointer motion,
2. pointer press,
3. pointer release,
4. wheel or scroll,
5. keyboard press or release,
6. enter or leave panel,
7. resize,
8. timer or animation tick,
9. picking result arrival.

These are semantic events.
They do not need to preserve every backend-specific field.


## Event Routing

Event routing should be explicit and panel-aware.

The normal route is:

1. runtime event arrives,
2. scene determines which panel or scene object it targets,
3. the event is translated into a scene-level event,
4. relevant controllers receive it in a deterministic order.

The spec should allow:

1. one focused panel,
2. one hovered panel,
3. optional controller capture during drag interaction.


## Capture And Focus

The scene should support a notion of temporary interaction capture.

This matters when:

1. a drag starts in one panel,
2. the pointer temporarily leaves the panel,
3. interaction should still continue with the same controller.

So the model should allow:

1. panel focus,
2. pointer capture during an active gesture,
3. release of capture when the gesture ends or is canceled.


## Panel-Owned Navigation

Navigation should be owned by the panel, not by the visual.

That means:

1. 2D panzoom belongs to panel state,
2. 3D camera belongs to panel state,
3. controllers mutate panel-local view state,
4. visuals consume the resulting panel-local transform state.

This keeps the transform model aligned with `TRANSFORM_PIPELINE.md`.


## Default Controller Families

The first useful set is:

1. `PanZoom2DController`
2. `Camera3DController`
3. `GlobeController`
4. `HoverController`
5. `SelectionController`
6. `LinkedPanelsController`

These names are descriptive only.


## `PanZoom2DController`

This controller should:

1. interpret drag and scroll input in panel coordinates,
2. mutate the panel’s visible domain or equivalent panzoom state,
3. mark `PanelTransformDirty`,
4. reevaluate whether attached axes need `AxisLayoutDirty`,
5. request redraw.

It should usually not:

1. invalidate normalized scene resources,
2. force bulk reupload of ordinary visual data.


## `Camera3DController`

This controller should:

1. interpret drag, scroll, and keyboard navigation as camera updates,
2. mutate the panel’s camera state,
3. mark `PanelTransformDirty`,
4. request redraw.

It may also:

1. invalidate view-dependent overlays,
2. schedule picking or probes if the interaction model wants that.


## `GlobeController`

This controller supports navigation on a 3D geographic globe.

It should:

1. constrain the camera to always point toward the globe center,
2. interpret drag as rotation on the sphere surface — expressed as longitude and latitude of
   the view center,
3. interpret scroll or pinch as altitude change — moving the camera closer to or farther from
   the surface,
4. enforce a minimum altitude to prevent the camera from passing through the surface,
5. enforce a maximum altitude beyond which the full globe is visible,
6. mark `PanelTransformDirty` on any navigation change.

It is the natural paired controller for `DVZ_TRANSFORM_GEO_GLOBE` visual attachments.

Unlike `Camera3DController`, navigation is expressed in geographic terms (longitude, latitude,
altitude) rather than free 3D space.
The camera is always oriented with the sphere's north pole up unless an explicit up-vector
override is applied.

The queryable domain interface for globe axes should return the currently visible longitude and
latitude ranges rather than raw `VisualSpace` extents. When `dvz_controller_query_domain` is
called on a `GlobeController`, `visible_min` and `visible_max` are in geographic units:
longitude (`DVZ_DIM_X`) in degrees east, latitude (`DVZ_DIM_Y`) in degrees north, altitude
(`DVZ_DIM_Z`) in the same distance unit used by `dvz_globe_controller_set_altitude`.

Constructor:

```text
globe = dvz_globe_controller(scene, flags)
dvz_globe_controller_set_altitude(globe, min_alt, max_alt)
dvz_panel_bind_controller(panel, globe, DVZ_DIM_XYZ)   // binds all three spatial dimensions
```


## `HoverController`

This controller should:

1. track current pointer position per panel,
2. decide whether hover picking should be requested,
3. coalesce or replace stale hover requests,
4. update hover state when a valid pick result returns,
5. request redraw when hover-visible state changes.

This controller should cooperate closely with the picking model in `PICKING.md`.

The default freshness rule should be:

1. hover follows latest-request-wins semantics,
2. stale hover results are discarded rather than applied late,
3. request identity and scene or panel generation are checked before hover state mutates.


## `SelectionController`

This controller should:

1. interpret click or drag-based selection gestures,
2. decide whether picking or geometric selection is needed,
3. mutate scene selection state,
4. mark the correct invalidation scopes,
5. request redraw.

Selection state should remain scene-owned rather than visual-owned.


## `LinkedPanelsController`

This controller should support coordinated multi-panel interaction.

Examples:

1. several panels share the same 2D visible domain,
2. one cursor position is mirrored across panels,
3. one selection or hover drives linked updates in other panels.

This controller is scene-global in effect even if it listens to one panel-local source event.


## Controller Handles

Controllers are first-class scene objects with stable handles.

This means:

1. a controller is created and owned by the scene,
2. it is returned as an opaque handle (`DvzController*`),
3. panels reference that handle rather than embedding controller state internally,
4. one controller handle may be bound to multiple panels,
5. the scene owns the controller lifecycle and destroys it when the scene is destroyed.

Making handles explicit is required for panel linking: two panels that share the same controller
handle automatically have their navigation synchronized, with no additional linking API.


## Controller Construction

Family-specific constructors are the preferred public surface, mirroring the visual construction
model in `PREFERRED_API_PROFILE.md`.

Conceptually:

```text
// Create a controller (scene-owned)
panzoom  = dvz_panzoom(scene, flags)
camera3d = dvz_camera3d(scene, flags)
hover    = dvz_hover(scene, flags)

// Optionally configure before binding
dvz_panzoom_set_bounds(panzoom, &bounds)
dvz_panzoom_set_aspect(panzoom, DVZ_ASPECT_EQUAL)  // see Aspect Ratio below
```

A generic `dvz_controller(scene, type, flags)` may exist internally but is not the user-facing
default.

Controllers are destroyed when the scene is destroyed.
Explicit `dvz_controller_destroy()` is available for earlier release.


## Aspect Ratio

`dvz_panzoom` supports an aspect ratio constraint that locks X and Y zoom together.

```text
dvz_panzoom_set_aspect(panzoom, DVZ_ASPECT_EQUAL)
```

| Value | Behavior |
|---|---|
| `DVZ_ASPECT_FREE` | X and Y scale independently (default) |
| `DVZ_ASPECT_EQUAL` | zoom constrains X and Y to the same data-unit-per-pixel ratio |

When `DVZ_ASPECT_EQUAL` is active, any zoom gesture that would scale X and Y differently is
adjusted so both dimensions receive the same scale factor.
Pan remains unconstrained.

The constraint operates on `VisualSpace` after normalization.
See `TRANSFORM_PIPELINE.md` for how the panel domain and normalization interact with aspect
ratio.


## Per-Dimension Binding

Panels bind controllers per dimension.

This allows partial linking: sharing X navigation while keeping Y independent, or binding a
3D camera globally while leaving a secondary dimension panel-local.

Conceptually:

```text
// Bind a panzoom controller to a panel's X dimension
dvz_panel_bind_controller(panel, panzoom, DVZ_DIM_X)
dvz_panel_bind_controller(panel, local_y, DVZ_DIM_Y)
```

A panel that has a controller bound for a dimension delegates all navigation on that dimension
to the controller.

For 2D panzoom, binding the same controller to both `DVZ_DIM_X` and `DVZ_DIM_Y` covers the
common case of full-panel panzoom.
For 3D, a `Camera3DController` handle covers all three dimensions through a single binding.

Panels retain the binding reference; the controller handle is the stable identity.


## Panel Linking Via Shared Handles

Sharing a controller handle across panels is the panel-linking mechanism.

Conceptually:

```text
// Create one shared X controller
shared_x = dvz_panzoom(scene, flags)

// Bind it to two panels on the X dimension only
dvz_panel_bind_controller(panel_a, shared_x, DVZ_DIM_X)
dvz_panel_bind_controller(panel_b, shared_x, DVZ_DIM_X)

// Each panel keeps its own independent Y controller
dvz_panel_bind_controller(panel_a, local_y_a, DVZ_DIM_Y)
dvz_panel_bind_controller(panel_b, local_y_b, DVZ_DIM_Y)
```

Both panels' X axes query `shared_x` and therefore always show the same visible X domain.
No separate linking API is needed: sharing the handle IS the link.

This pattern covers common scientific visualization layouts:

1. spike raster + PSTH sharing a time axis,
2. multi-panel time series locked to the same X range,
3. scatter plot + marginal histograms sharing X and Y,
4. overview + detail with one synchronized dimension.


## Domain Query Interface

Axes and other scene objects query controllers for the current visible domain.

This is a pull model: the scene does not push domain changes to axes.
Instead, during the axis update step of the frame lifecycle, the axis queries its bound
controller directly.

Conceptually:

```text
// Axis queries its panel's X controller for the currently visible range
// visible_min and visible_max are double* out-parameters; they receive data-space scalar bounds.
dvz_controller_query_domain(panel_x_controller, DVZ_DIM_X, &visible_min, &visible_max)
```

`visible_min` and `visible_max` are `double` scalars giving the data-space bounds of the
currently visible range along the queried dimension. For `GlobeController` these are in
geographic units; for other controllers they are in the same units as the declared
`DvzDataDomain` (see `TRANSFORM_PIPELINE.md`).

This makes axis update deterministic and avoids event-routing complexity for something that
happens every frame anyway.

If no controller is bound to a dimension, the panel uses its full data-space domain.


## Interaction State

The scene layer should represent interaction state explicitly.

Useful conceptual state includes:

1. focused panel,
2. captured controller,
3. hovered panel,
4. current pointer position per active panel,
5. hover target,
6. selection state,
7. active gesture state.

This state belongs to the scene layer, not to backend callbacks.


## Gesture Model

The scene layer should think in terms of gestures, not just raw events.

Examples:

1. press-drag-release pan gesture,
2. wheel zoom gesture,
3. click-to-select gesture,
4. drag-box selection gesture,
5. hover probe gesture.

Controllers may internally keep gesture state machines, but the scene spec should remain focused on
their observable effects.


## Touch And Multi-Touch Input

Touch input is translated into the same semantic gesture events as mouse input before reaching
controllers. Controllers require no touch-specific code.

A **gesture recognizer** layer sits between the platform input source and the scene event
system. It tracks active touch points and emits `DvzGestureEvent` objects when a recognized
gesture is detected.

### Supported Gestures

| Gesture | Touch pattern | Controller action |
|---|---|---|
| `DVZ_GESTURE_PAN` | 1-finger drag, or 2-finger drag | pan |
| `DVZ_GESTURE_PINCH` | 2-finger pinch / spread | zoom |
| `DVZ_GESTURE_ROTATE` | 2-finger rotation | 3D rotation (`ArcballController`) |
| `DVZ_GESTURE_TAP` | 1-finger tap | pick / click |

Gestures above are in scope for v0.4.
3+ finger gestures, pressure sensitivity, stylus tilt, and multi-touch picking are deferred.

### Platform Sources

| Platform | Touch source |
|---|---|
| Desktop touchscreen / tablet | GLFW touch callbacks |
| Browser (WebGPU / Emscripten) | Emscripten touch events |
| macOS trackpad pinch / scroll | existing GLFW scroll events — no change needed |

The gesture recognizer is initialized by the runtime with the appropriate platform source.
The scene layer consumes only `DvzGestureEvent` and is unaware of the source.

### ImGui Routing

Touch input follows the same routing rule as mouse input.
`io.WantCaptureMouse` is checked first; gestures claimed by ImGui are not forwarded to scene
controllers.


## Picking Integration

Controllers should not interpret GPU payloads directly.

Instead:

1. controllers may request picking,
2. picking results return to scene-level routing,
3. controllers consume interpreted scene-level pick results.

This preserves the boundary:

1. controllers decide interaction policy,
2. picking resolves scene identity,
3. controllers mutate scene state from that identity.

Controllers should not need to guess whether a pick result is current.

That freshness decision should already have been made by scene-level routing using the request
identity and revision data defined by `PICKING.md`.


## Hover Flow

The intended hover flow is:

1. pointer motion enters a panel,
2. hover controller updates current pointer position,
3. if hover policy requires it, a pick request is issued or refreshed,
4. the next suitable frame includes or reuses the picking path,
5. a pick result returns,
6. hover state is updated if the result is current,
7. redraw is requested if the visible hover state changed.


## Click Selection Flow

The intended click-selection flow is:

1. pointer press occurs in a panel,
2. selection controller records gesture state,
3. pointer release or click confirmation occurs,
4. a pick request is issued or consumed,
5. the pick result is mapped back to scene identity,
6. selection state is mutated,
7. redraw is requested if the visible selection changed.


## Box Or Region Selection

The spec should leave room for region-based selection without forcing it immediately into the first
family contracts.

At the interaction level, it should still fit the same pattern:

1. controller owns gesture state,
2. panel-local geometry defines the query region,
3. selection state is updated at scene level,
4. redraw and invalidation follow normal rules.


## Axes And Controllers

Axes should participate in interaction through controllers rather than by owning backend callbacks.

Possible axis-related interactions include:

1. hover on ticks or labels,
2. drag on an axis-linked range control,
3. scroll or zoom that changes axis layout indirectly through panel navigation,
4. click on axis annotations.

Axis interaction should still route through scene-owned identities and invalidation.


## Controllers And Invalidation

Controllers should be one of the main producers of invalidation.

Typical mappings:

1. panzoom gesture -> `PanelTransformDirty`, maybe `AxisLayoutDirty`
2. camera gesture -> `PanelTransformDirty`
3. hover state change -> redraw, maybe style-related `VisualPropsDirty`
4. selection change -> redraw, maybe style-related `VisualPropsDirty`
5. controller mode change -> `FramePlanDirty` only if scene participation or picking policy changes

Controllers should not bypass `INVALIDATION_AND_CACHING.md`.


## Controllers And Redraw

Not every event should force immediate redraw.

The controller layer should be free to:

1. mark state dirty,
2. coalesce events,
3. request redraw only when the visible result may change.

This matters especially for:

1. hover motion,
2. repeated wheel events,
3. linked multi-panel interaction.


## Animation And Controllers

Animations and controllers are related but distinct.

The useful separation is:

1. controllers respond to external interaction events,
2. animations evolve scene properties over time,
3. both feed the same invalidation and redraw system.

This keeps the frame lifecycle clean:

1. input and controller update,
2. animation update,
3. invalidation resolution,
4. `FramePlan` build.


## Determinism

For a given:

1. scene state,
2. event sequence,
3. controller state,
4. capability record,

the resulting scene mutations and invalidation decisions should be deterministic.

This is important for:

1. tests,
2. replayable interaction traces,
3. debugging.


## Diagnostics

The scene layer should be able to report:

1. which controller handled the last event,
2. which panel currently owns focus or capture,
3. which invalidation scopes were produced by a gesture,
4. whether a hover request was coalesced or discarded,
5. whether a selection change triggered plan rebuild or only redraw.


## API-Sketch Consequences

This document defines the following concrete API-shape commitments:

1. controllers are created by family-specific constructors returning `DvzController*` handles,
2. panels bind controllers per dimension via `dvz_panel_bind_controller(panel, controller, dim)`,
3. panel linking is achieved by sharing a controller handle — no separate linking API,
4. axes and scene objects query controllers via `dvz_controller_query_domain()` in a pull model,
5. events are routed to the scene, which dispatches to the correct panel and its bound controllers,
6. hover and selection state remain queryable at scene level,
7. focus and capture remain scene-owned, not backend-owned.

The exact final naming follows the resolved `dvz_` convention from `PREFERRED_API_PROFILE.md`.


## Recommended Next Step

The next useful spec iteration is a worked 2D axis example that traces:

1. domain source → tick generation → `FramePlan` contribution,
2. panzoom interaction → controller domain query → axis regeneration decision,
3. panel linking with shared controller → synchronized axes across panels.

This will pressure-test the axis binding model and the pull-model query interface defined above.
