# Scene Controller Binding Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / PARTIAL FLY SLICE LANDED`
> - **Updated on:** `2026-05-21`
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

The active scene code now has the first generic controller slice for fly only: public
`DvzController*`, dimension masks, scene-owned controller storage, `dvz_scene_fly()`,
`dvz_controller_fly()`, `dvz_panel_bind_controller()`, and `dvz_panel_controller()` are present.
`dvz_panel_bind_controller()` currently accepts only fly controllers bound with
`DVZ_DIM_MASK_XYZ`.

Panzoom, arcball, and turntable remain panel-owned navigation controllers. The accepted target is
scene-owned opaque `DvzController*` handles that panels borrow and bind by dimension mask. Linked
panels should share controller identity instead of copying mutable panzoom fields.


## Completed Binding Work

Implemented slices:

1. Public opaque `DvzController*` and `DvzDimMask` declarations are present.
2. `DvzScene` owns a fixed controller table and destroys scene-owned fly payloads.
3. Panels have per-dimension borrowed controller bindings.
4. The first `dvz_panel_bind_controller()` / `dvz_panel_controller()` path is implemented and
   validated for fly plus `DVZ_DIM_MASK_XYZ`.
5. Focused fly tests cover scene-owned controller creation, incompatible dimension rejection,
   panel destruction preserving a shared controller, and one shared fly updating once for two
   panels.


## Remaining Binding Work

Recommended follow-up commits:

1. Move panzoom behind the generic controller handle first, with typed POD state get/set helpers.
2. Broaden `dvz_panel_bind_controller()` validation beyond fly to panzoom family/dimension
   compatibility, including split X/Y bindings.
3. Route input through panel bindings while keeping viewport and panel-local coordinate context on
   the event path.
4. Add internal axis/domain queries through the bound controller instead of direct panel panzoom
   access.
5. Migrate the linked-panels example away from copied panzoom fields and onto shared controller
   handles.
6. Move arcball and turntable behind `DvzController*` after the panzoom/binding path is tested.
7. Revisit the compatibility wrappers so `dvz_panel_set_panzoom()`, `dvz_panel_set_arcball()`, and
   `dvz_panel_set_turntable()` either create scene-owned handles or are clearly transitional.


## First Acceptance Tests

Landed for the fly slice:

1. Incompatible fly dimension binding fails validation.
2. Destroying a panel does not destroy a shared scene-owned fly controller.
3. A shared fly controller bound to two panels updates once and writes back to both cameras.

Still needed for the panzoom-first binding migration:

1. Two panels bound to one XY panzoom report identical visible X and Y domains.
2. Two panels bound to one X panzoom and separate Y panzooms share X only.
3. Linked panels work with different viewport rectangles and sizes.
4. Rebinding X does not change an existing Y binding.
5. Wrong-family or incompatible-dimension binding fails validation across all landed families.
6. Typed panzoom state get/set rejects non-panzoom controllers.
7. `examples/c/techniques/linked_panels.c` no longer copies public panzoom fields.


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
