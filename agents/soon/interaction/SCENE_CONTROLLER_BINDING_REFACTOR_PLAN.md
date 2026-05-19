# Scene Controller Binding Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the near-term implementation sequence for scene-owned controller handles,
>   panel bindings, linked panels, and binding-friendly controller APIs.


## Current State

Durable controller contracts live in:

1. [`../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md)
2. [`../../../spec/scene/interaction/CONTROLLERS.md`](../../../spec/scene/interaction/CONTROLLERS.md)
3. [`../../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md)
4. [`../../../spec/scene/proposals/active/TRANSFORM_CONTROLLER_DESIGN.md`](../../../spec/scene/proposals/active/TRANSFORM_CONTROLLER_DESIGN.md)
5. [`../../../spec/scene/proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md`](../../../spec/scene/proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md)

Use this file only for execution sequencing and validation. Do not duplicate the accepted public
controller ownership, binding, input routing, or axis-domain rules here.

The active scene code still has panel-owned navigation controllers. The accepted target is
scene-owned opaque `DvzController*` handles that panels borrow and bind by dimension mask. Linked
panels share controller identity instead of copying mutable panzoom fields.


## Remaining Binding Work

Recommended follow-up commits:

1. Add the public opaque controller type and dimension-mask surface without changing existing panel
   controller behavior.
2. Add scene-owned controller storage and lifecycle rules; panels should borrow controller handles
   and never destroy them.
3. Move panzoom behind the generic controller handle first, with typed POD state get/set helpers.
4. Add `dvz_panel_bind_controller()` and `dvz_panel_controller()` and validate family/dimension
   compatibility.
5. Route input through panel bindings while keeping viewport and panel-local coordinate context on
   the event path.
6. Add internal axis/domain queries through the bound controller instead of direct panel panzoom
   access.
7. Migrate the linked-panels example away from copied panzoom fields and onto shared controller
   handles.
8. Move arcball, fly, and turntable behind `DvzController*` only after the panzoom/binding path is
   tested.


## First Acceptance Tests

1. Two panels bound to one XY panzoom report identical visible X and Y domains.
2. Two panels bound to one X panzoom and separate Y panzooms share X only.
3. Linked panels work with different viewport rectangles and sizes.
4. Rebinding X does not change an existing Y binding.
5. Wrong-family or incompatible-dimension binding fails validation.
6. Destroying a panel does not destroy a shared controller.
7. Typed panzoom state get/set rejects non-panzoom controllers.
8. `examples/c/techniques/linked_panels.c` no longer copies public panzoom fields.


## Files To Update As Implementation Lands

1. public scene headers;
2. `src/scene/` controller storage, panel binding, and input routing internals;
3. [`../../../spec/scene/examples/core/LINKED_PANELS_AXES_PANZOOM.md`](../../../spec/scene/examples/core/LINKED_PANELS_AXES_PANZOOM.md);
4. [`../../../docs/architecture/manual_scene_smoke.md`](../../../docs/architecture/manual_scene_smoke.md);
5. `examples/c/techniques/linked_panels.c`;
6. WASM/API-surface notes touched by the new declarations.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation slices, use:

```text
just build
just test scene
```

For live input/routing changes, add a bounded linked-panel smoke such as:

```text
./build/examples/c/techniques/linked_panels 300
```
