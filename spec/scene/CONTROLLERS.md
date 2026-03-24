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
3. `HoverController`
4. `SelectionController`
5. `LinkedPanelsController`

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

This document suggests the scene API should eventually support concepts like:

1. attach controller to panel,
2. route event to scene,
3. query current hover or selection state,
4. query focused or captured controller,
5. request or cancel interaction capture.

The final naming is still open, but the semantics should remain.


## Recommended Next Step

The next useful spec iteration is probably `ANNOTATIONS.md` or `LEGENDS_AND_COLORBARS.md`.

The scene now has enough structure for:

1. visuals,
2. transforms,
3. axes,
4. picking,
5. controllers,

and the next likely composite objects are legends, labels, guides, and colorbar-like annotation.
