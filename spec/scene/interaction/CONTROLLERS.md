# Scene Controllers And Interaction

> **Status:** normative scene interaction model for v0.4.
> **Authority:** this file defines shared controller routing, state, capture, and invalidation
> rules. Family-specific camera behavior lives in
> [`CAMERA_CONTROLLERS.md`](CAMERA_CONTROLLERS.md), and the accepted ownership/binding API lives in
> [`../decisions/CONTROLLER_BINDING_MODEL.md`](../decisions/CONTROLLER_BINDING_MODEL.md).


## Purpose

Controllers are scene-side state machines. They translate scene-level events into scene-state
mutations, invalidation, and redraw scheduling.

The model must:

1. keep input policy above DRP2 and backend internals;
2. make panel-local navigation, routing, focus, and capture explicit;
3. give hover, picking, and selection a scene-level home;
4. fit the frame lifecycle in [`../pipeline/FRAME_LIFECYCLE.md`](../pipeline/FRAME_LIFECYCLE.md)
   and invalidation rules in
   [`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md);
5. expose opaque, WASM-portable handles as required by
   [`../api/WASM_PORTABILITY.md`](../api/WASM_PORTABILITY.md).


## Core Rules

A controller mutates scene, panel, selection, hover, or navigation state. It does not emit DRP2,
record backend commands, own backend synchronization, bypass invalidation, or interpret raw GPU
pick payloads.

The normal order is:

1. runtime input is translated into a scene-level event;
2. routing selects a panel, focused target, hovered target, or captured controller;
3. controllers consume the event in deterministic order;
4. state is mutated and dirty scopes are marked;
5. the next frame resolves validation, planning, DRP2 emission, and runtime execution.


## Boundary And Scope

| Scope | Owns | Examples |
|---|---|---|
| Panel-local | one panel's navigation, view, probes, or gestures | panzoom, orbit/fly, box selection, panel probes |
| Scene-global | state shared across panels or the whole scene | linked panning, shared cursor, animation scrubber, global selection |

Navigation belongs to the panel, not to visuals. Visuals consume panel-local transform state from
[`../pipeline/TRANSFORM_PIPELINE.md`](../pipeline/TRANSFORM_PIPELINE.md).


## Routing, Focus, And Capture

Event routing is explicit and panel-aware.

| Concept | Required behavior |
|---|---|
| Scene event | Backend events are normalized into semantic pointer, wheel, keyboard, resize, timer, gesture, and pick-result events. Backend-specific fields are not authoritative. |
| Hovered panel | Pointer motion updates the panel under the pointer when no capture overrides routing. |
| Focused panel | Keyboard and mode-changing events may use the focused panel. Focus is scene-owned. |
| Capture | A drag or gesture may temporarily route events to the initiating controller even after the pointer leaves the panel. Capture ends on release or cancel. |
| Order | Dispatch order must be deterministic for a fixed scene state, event sequence, controller state, and capability record. |

Touch is translated before this routing stage. Controllers consume gestures, not platform touch
callbacks; see [`../integration/TOUCH_SUPPORT.md`](../integration/TOUCH_SUPPORT.md).


## Controller Families

| Family | Scope | State mutated | Dirty/redraw consequences | Notes |
|---|---|---|---|---|
| `PanZoom2DController` | panel-local or shared handle | visible X/Y domain or panzoom state | `PanelTransformDirty`; `AxisLayoutDirty` only when domains/layout-affecting values change; redraw | `DVZ_ASPECT_EQUAL` locks X/Y data-unit-per-pixel ratio; pan remains free. |
| `Camera3DController` | panel-local or shared handle | camera state | `PanelTransformDirty`; redraw; optional view-dependent overlay invalidation | Fly, arcball/pivot-orbit, and turntable semantics are canonical in [`CAMERA_CONTROLLERS.md`](CAMERA_CONTROLLERS.md). |
| `GlobeController` | panel-local or shared handle | longitude, latitude, altitude, optional up-vector | `PanelTransformDirty`; redraw | Natural controller for `DVZ_TRANSFORM_GEO_GLOBE`; domain queries return geographic units. |
| `HoverController` | usually panel-local | pointer position and hover target | redraw when hover-visible state changes; optional style dirtiness | Issues/coalesces pick requests and applies only fresh latest-request-wins results from [`PICKING.md`](PICKING.md). |
| `SelectionController` | panel-local gestures, scene-owned selection | selection state | redraw; style dirtiness when selection affects visual props | Click/drag gestures may use picking or geometric region queries. |
| `LinkedPanelsController` | scene-global | shared domains, cursors, hover, or selection | affected panel transforms, overlays, or redraw | Sharing one controller handle is the linking mechanism. |


## State Inspection And Editing

Controllers expose family-specific state snapshots for UI inspectors, tests, serialization, and
programmatic navigation. State snapshots carry semantic values such as pan, zoom, quaternion, pivot,
yaw, pitch, distance, position, target, roll, movement speed, and clamp limits. Derived view,
projection, and model matrices are not authoritative controller state.

Applying a snapshot is semantically equivalent to a user gesture:

1. controller state is updated;
2. attached panel or camera state is updated;
3. `PanelTransformDirty` is marked;
4. axis/layout dirtiness is marked only when visible domains or layout-affecting values change;
5. redraw is requested.

Arcball pan is controller state, not visual geometry. Right/middle drag moves the apparent rotation
center through panel-plane translation; fixed overlays attached with `DVZ_CONTROLLER_FIXED` remain
unaffected. Pivot picking may later set the arcball center from a semantic pick target, but default
pan remains lightweight and deterministic.


## Binding, Linking, And Domain Queries

The public binding model is fixed by
[`../decisions/CONTROLLER_BINDING_MODEL.md`](../decisions/CONTROLLER_BINDING_MODEL.md):

| Topic | Rule |
|---|---|
| Handles | Controllers are scene-owned opaque `DvzController*` objects with stable identity. |
| Construction | Family-specific constructors are the public surface; generic constructors may remain internal. |
| Lifetime | The scene destroys controllers with the scene; explicit early destruction may exist. |
| Per-dimension binding | Panels bind controllers per dimension so X, Y, Z, or full camera navigation can be linked independently. |
| Linking | Sharing a controller handle across panels is the linking API; no separate link object is required. |
| Query model | Axes and scene objects pull visible domains from bound controllers during update. If none is bound, the panel uses its full data-space domain. |

Domain query values are data-space scalar bounds for ordinary controllers and longitude, latitude,
or altitude for `GlobeController`.


## Interaction Flows

| Flow | Required sequence |
|---|---|
| Hover | pointer motion -> hover controller updates panel position -> latest hover pick is issued/refreshed when needed -> fresh result mutates hover state -> redraw if visible hover changed |
| Click selection | press records gesture -> release/click confirmation issues or consumes pick -> scene identity is resolved -> selection state mutates -> redraw if visible selection changed |
| Box/region selection | controller owns gesture -> panel-local query region is resolved -> scene selection state mutates -> normal invalidation/redraw follows |
| Animation plus interaction | controller updates and animation updates both feed the same invalidation system before validation and planning |

Picking integration uses interpreted scene identity only. Request identity, freshness, and
latest-wins behavior are defined in [`PICKING.md`](PICKING.md).


## Invalidation And Diagnostics

Typical controller invalidation:

| Mutation | Consequence |
|---|---|
| panzoom or camera navigation | `PanelTransformDirty`, redraw |
| visible domain change | `AxisLayoutDirty` where axes depend on the domain |
| hover or selection state change | redraw; optional `VisualPropsDirty` for style-driven highlighting |
| controller mode or picking policy change | `FramePlanDirty` only when scene participation or picking topology changes |

Diagnostics should use the schema in
[`../validation/DIAGNOSTICS.md`](../validation/DIAGNOSTICS.md) and report the smallest useful
scene scope: last handling controller, focus/capture owner, produced dirty scopes, coalesced or
discarded hover requests, and whether a selection change required plan rebuild or redraw only.


## Non-Goals

This document does not define exact final C names, callback signatures, platform event adapters,
threading policy, backend window behavior, or camera-family math already covered by
[`CAMERA_CONTROLLERS.md`](CAMERA_CONTROLLERS.md).
