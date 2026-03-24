# External UI Integration

This document defines how external UI frameworks should interact with the future scene layer.

An external UI framework may be ImGui, a native widget toolkit, a web-side control surface, or any
other app-level interface that reads and mutates scene-owned semantic state.


## Normative Status

This document is normative for the boundary between scene semantics and external UI.

It should be read together with:

1. `SCENE_API_SKETCH.md` for the scene-owned state surface,
2. `CONTROLLERS.md` for scene-native interaction behavior,
3. `FRAME_LIFECYCLE.md` for per-frame ordering,
4. `RUNTIME_BOUNDARY.md` for what must remain below the scene semantic layer.


## Core Rule

External UI is a client of scene state, not part of the scene object model.

That means:

1. tree views, sliders, buttons, menus, inspectors, and similar widgets are not scene primitives,
2. these widgets may read and mutate scene-owned semantic state,
3. the scene remains the owner of the resulting meaning such as selection, visibility, opacity,
   active slice, filter choice, probe state, or panel configuration.


## What External UI May Do

An external UI layer may:

1. inspect scene objects and scene-owned semantic state,
2. mutate scene-owned state through setters, descriptors, or app-level commands,
3. request redraw after mutating scene-visible state,
4. observe scene-produced picking, probe, validation, and diagnostic results,
5. display app-level controls that coordinate several scene objects at once.


## What External UI Must Not Be

External UI should not be modeled as:

1. a visual family,
2. an annotation family,
3. a backend-specific scene escape hatch,
4. a second planner parallel to scene.

If an app uses ImGui, the preferred interpretation is:

1. ImGui is app-owned native UI,
2. scene remains the semantic producer of visualization state and `FramePlan`,
3. the runtime remains the execution service for scene work.


## Relationship To Controllers

Scene controllers and external UI may coexist, but they play different roles.

The preferred split is:

1. scene controllers own panel-native interaction such as camera navigation, picking-driven
   selection, hover, and linked-panel behaviors,
2. external UI owns widget interaction such as tree toggles, filter selectors, numeric inspectors,
   and tool panels,
3. both mutate the same scene-owned semantic state when they affect the same concept.

For example:

1. clicking a region in a panel may update selected-region state through a scene controller,
2. clicking a region entry in an ImGui tree may update that same selected-region state through
   app-level UI code.

The state should remain single-sourced at the scene level.


## Input Routing

The spec should allow an app-level event policy in which external UI receives input before scene
controllers when the app chooses that behavior.

The preferred native-app pattern is:

1. raw runtime input arrives,
2. the external UI layer consumes the event first if appropriate,
3. unconsumed or forwarded input is translated into scene-level events,
4. scene controllers process those events and mutate scene state.

This keeps widget focus, typing, dragging, and menu interaction out of scene-native controller
policy.


## Rendering Relationship

External UI rendering is outside the scene plan.

The scene spec should assume:

1. scene builds one scene-level `FramePlan`,
2. runtime executes that plan,
3. an app may optionally render an external UI overlay after scene execution and before present,
4. this overlay rendering does not redefine scene semantics or bypass scene validation and planning.

The exact overlay implementation may be native and backend-specific.
That implementation detail belongs below the scene semantic layer.


## DRP2 Boundary

External UI should not be required to appear as scene-emitted DRP2.

The current preferred direction is:

1. DRP2 carries scene-planned visualization work,
2. app-owned UI overlays may use a separate native rendering path when the runtime supports it,
3. future generic retained UI systems, if desired later, should be specified separately rather than
   smuggled into scene through an ImGui-shaped shortcut.


## Examples

Good fits for external UI:

1. an ImGui tree controlling atlas-region visibility and opacity,
2. a filter combo box selecting the active slice-processing mode,
3. a tool panel showing the latest picked world coordinates and sampled value,
4. an inspector editing panel layout or camera presets.

Good fits for scene-native semantics instead:

1. crosshairs,
2. probe labels anchored inside a panel,
3. colorbars and legends,
4. pickable region highlights,
5. camera navigation.


## Rule Summary

1. external UI is app-owned and widget-oriented,
2. scene owns semantic visualization state,
3. scene controllers own panel-native interaction,
4. runtime executes scene work,
5. optional external UI overlay rendering may happen after scene execution and before present.
