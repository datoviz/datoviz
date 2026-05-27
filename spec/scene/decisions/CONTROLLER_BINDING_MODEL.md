# Controller Binding Model

- Status: accepted for the v0.4 controller refactor
- Date: 2026-05-18
- Amended: 2026-05-24, to distinguish full identity sharing from partial controller state links
- Applies to: scene controllers, panel linking, axes, interaction routing, and WASM-facing API


## Context

The active scene implementation currently attaches navigation controllers directly to panels. That
works for one-panel interaction, but it makes linked panels awkward: examples have to detect which
panel-owned panzoom changed and copy mutable controller fields into another panel-owned panzoom.

The v0.4 scene API is still allowed to break compatibility. This is the right time to settle the
ownership and binding model before examples, bindings, axes, and WebGPU/WASM paths start depending
on the current panel-owned shape.

The model also needs to support more than panzoom. Arcball, fly, turntable, hover, selection, and
future interaction tools are controller families with different behavior. The public API should not
pretend that every controller is a panzoom, and it should not expose C inheritance casts or public
unions that are hostile to generated bindings and WASM.


## Decision

Controllers are scene-owned opaque handles.

`DvzController*` is the public controller handle type. Panzoom, arcball, fly, and turntable are
controller families created by typed constructors that all return `DvzController*`:

```c
DvzController* dvz_panzoom(DvzScene* scene, const DvzPanzoomDesc* desc);
DvzController* dvz_arcball(DvzScene* scene, const DvzArcballDesc* desc);
DvzController* dvz_fly(DvzScene* scene, const DvzFlyDesc* desc);
DvzController* dvz_turntable(DvzScene* scene, const DvzTurntableDesc* desc);
```

Panels bind controller handles by dimension mask:

```c
int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);
```

Full linked panels may be made by binding the same controller handle to multiple panels. This is
identity sharing: every panel observes the same full controller state. It is correct for shared X
panzoom, shared XY panzoom, or genuinely identical 3D navigation.

Partial synchronization uses explicit controller state links between distinct controllers. A
general controller-link API is preferred over family-specific sync helpers:

```c
DvzControllerLink* dvz_controller_link(
    DvzScene* scene,
    DvzController* source,
    DvzController* target,
    uint32_t components,
    DvzControllerLinkMode mode);
```

The link components are semantic state components such as `ROTATION`, `PAN`, `ZOOM`,
`EXTENT_X`, `EXTENT_Y`, and eventually camera-oriented components. For example, an orientation
gizmo should have its own arcball controller linked one-way from the main arcball by `ROTATION`
only. It should not share the main arcball handle, because the inset needs independent viewport,
camera-basis, and pan/zoom behavior.

No panzoom-specific or arcball-specific linked-panels API is needed.


## Non-Decision

This decision does not require a general-purpose property API such as
`dvz_controller_set_float(controller, key, value)`. Controller families should keep typed state APIs.

This decision also does not make `LinkedPanelsController` the way to synchronize navigation state.
`LinkedPanelsController` remains reserved for higher-level linked behavior such as shared cursors,
hover, selection, brushing, and cross-panel semantic coordination. Navigation-state links belong to
the controller binding/link system.


## Ownership

1. Scenes own controllers.
2. Panels borrow controller handles.
3. Destroying a panel never destroys a controller.
4. Destroying a scene destroys its controllers.
5. Explicit controller destruction, if exposed, releases the scene-owned handle and clears or
   invalidates panel bindings that reference it.
6. Rebinding a controller replaces previous bindings for the target dimensions only.
7. Rebinding X must not affect Y unless the binding mask includes Y.
8. A panel dimension may have at most one spatial navigation controller.
9. A controller link does not transfer ownership; source and target controllers remain
   scene-owned handles.


## Dimension Binding

Dimension binding uses an explicit mask type, not an overloaded single-dimension enum:

```c
typedef uint32_t DvzDimMask;
```

The public API should provide masks for common dimension sets, including X, Y, Z, XY, and XYZ.
Single-dimension query APIs may continue to use `DvzDim`.

Controller-family compatibility rules:

1. panzoom may bind to X, Y, or XY and supports partial linking,
2. arcball binds to XYZ only in the first implementation,
3. fly binds to XYZ only in the first implementation,
4. turntable binds to XYZ only in the first implementation,
5. hover and selection controllers are not spatial transform controllers and should define their
   own binding semantics when implemented.

Wrong-family or incompatible-dimension bindings should fail validation rather than silently degrade.


## Input Routing

Input routing is panel-local. A shared controller may receive gestures through multiple panels.

The panel binding supplies viewport-local event context, including the source panel rect and panel
coordinate mapping. The controller stores semantic state such as pan, zoom, orientation, pivot, or
camera navigation values.

A shared controller must not own one canonical native viewport. Different panels bound to the same
controller may have different sizes and positions.

When panel-local evaluation context differs meaningfully, prefer distinct controllers plus a
component link over sharing one handle. Arcball orientation gizmos are the canonical case: the main
view may pan and zoom while the inset controller receives only orientation and remains centered.


## Axis Pull Model

Axes do not own or subscribe to controllers.

During frame preparation, an axis queries the panel's bound controller for its dimension. If two
panels bind the same X controller, their X visible domains match. Each panel still owns and
regenerates its own axis geometry, labels, and tick resources.

Shared controller means shared visible domain. It does not mean shared axis resources.


## State APIs

Controller state should be exposed through typed POD snapshots:

```c
int dvz_controller_type(const DvzController* controller, DvzControllerType* out);

int dvz_panzoom_get_state(const DvzController* controller, DvzPanzoomState* out);
int dvz_panzoom_set_state(DvzController* controller, const DvzPanzoomState* state);

int dvz_arcball_get_state(const DvzController* controller, DvzArcballState* out);
int dvz_arcball_set_state(DvzController* controller, const DvzArcballState* state);
```

The same pattern should be used for fly and turntable when their state snapshots are finalized.
Typed state APIs validate the controller family and return an error for the wrong family.

State snapshots are the supported surface for tests, bindings, serialization, external UI
inspectors, and examples. Public code should not copy controller struct fields.

Controller state links should be implemented in terms of these typed semantic snapshots or
equivalent internal family ops, not by ad hoc public struct copying. Links propagate selected
components after input, animation, or typed setter changes and before frame-plan emission.


## Link Components

The general controller-link layer should define component masks instead of one API per family.
First-slice components:

1. `DVZ_CONTROLLER_LINK_ROTATION` for arcball/turntable orientation;
2. `DVZ_CONTROLLER_LINK_PAN` for panzoom pan or arcball panel-plane pan where supported;
3. `DVZ_CONTROLLER_LINK_ZOOM` for panzoom zoom or arcball uniform zoom where supported;
4. `DVZ_CONTROLLER_LINK_EXTENT_X` and `DVZ_CONTROLLER_LINK_EXTENT_Y` for domain/extent sharing.

Links validate family compatibility and reject unsupported components. Mixed-family links are
deferred until a specific semantic mapping is documented. Link propagation must be deterministic,
cycle-safe, and redraw-aware.


## WASM And Binding Constraints

The controller API must follow `../api/WASM_PORTABILITY.md`:

1. expose opaque `DvzController*` handles,
2. avoid public casts between controller families,
3. avoid public controller unions,
4. avoid public mutable controller structs,
5. use POD state structs and output-pointer APIs,
6. keep platform event and viewport details out of public controller structs.

Private implementation may use a named tagged union or ops table inside `src/scene`, but that
layout must not leak into installed headers or generated bindings.


## Consequences

Positive consequences:

1. fully linked panels are achieved by identity sharing instead of state copying,
2. partial links such as shared X with independent Y or rotation-only gizmos become first-class,
3. axes have one clear source for visible-domain queries,
4. generated bindings and WASM can expose one controller handle type,
5. controller families retain typed, discoverable state APIs,
6. panel destruction and controller destruction have clear ownership boundaries.

Costs:

1. existing panel-owned controller APIs and examples need to be rewritten,
2. input dispatch must route events through panel bindings rather than direct controller router
   subscriptions,
3. current panzoom, arcball, fly, and turntable storage needs consolidation behind the scene-owned
   controller table,
4. tests need to cover binding, rebinding, resize, wrong-family validation, component-link
   validation, and cycle-safe propagation.


## Acceptance Criteria

Implementation of this decision should include tests or examples for:

1. two panels bound to the same XY panzoom reporting identical visible X and Y domains,
2. two panels bound to the same X panzoom and different Y panzooms sharing X only,
3. linked panels with different viewport rectangles and sizes,
4. linked X axes sharing visible domain while owning independent tick resources,
5. rebinding X without changing Y,
6. wrong-family or incompatible-dimension bindings failing validation,
7. scene destruction freeing shared controllers once,
8. panel destruction not destroying shared controllers,
9. typed state get/set rejecting the wrong controller family,
10. arcball rotation-only link keeping an orientation gizmo centered while the source arcball is
    panned or zoomed,
11. link destruction or source/target controller destruction stopping propagation cleanly.


## Related Specs

1. `../interaction/CONTROLLERS.md`
2. `../semantics/AXES.md`
3. `../examples/scenarios/v04_required/CORE_RELEASE_PROOFS.md`
4. `../api/WASM_PORTABILITY.md`
