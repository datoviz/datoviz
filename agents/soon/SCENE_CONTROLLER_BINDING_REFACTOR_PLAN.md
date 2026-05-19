# Scene Controller Binding Refactor Plan

> **Execution Status**
> - **Status:** `IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-19`
> - **Purpose:** turn the accepted controller-binding decision into a concrete near-term
>   implementation lane for linked panels, shared axes, and binding-friendly controller APIs.


## Summary

The accepted v0.4 controller model lives in
[`../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).
This file is only the tactical execution plan. Do not redefine the API here; keep this note aligned
with the canonical spec and use it to guide implementation sequencing.

The target model is:

```c
DvzController* dvz_panzoom(DvzScene* scene, int flags);
DvzController* dvz_arcball(DvzScene* scene, int flags);
DvzController* dvz_fly(DvzScene* scene, const DvzFlyDesc* desc);
DvzController* dvz_turntable(DvzScene* scene, const DvzTurntableDesc* desc);

int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);
```

Linked panels are created by binding the same scene-owned controller handle to multiple panels. No
separate panzoom-specific linked-panel API should be introduced.


## Current Baseline

The active scene code still attaches navigation controllers directly to panels:

1. `dvz_panel_set_panzoom()` creates and owns a `DvzPanzoom` inside the panel.
2. `dvz_panel_panzoom()` exposes that panel-owned object.
3. Arcball, fly, and turntable follow the same panel-owned pattern.
4. The `linked_panels` example copies mutable panzoom fields in a frame callback to simulate
   linking.

That shape works for single-panel navigation, but it makes shared X, shared Y, shared cameras,
bindings, and WASM-facing APIs awkward.


## Goals

1. Introduce scene-owned opaque `DvzController*` handles.
2. Bind controllers to panels by dimension mask.
3. Make linked panels use shared controller identity instead of state copying.
4. Support partial links: shared X with independent Y, shared XY, and future XYZ camera links.
5. Keep input routing panel-local so one shared controller can receive gestures from several
   differently sized panels.
6. Provide typed POD state get/set APIs for tests, examples, external UI, serialization, and
   generated bindings.
7. Keep axis domain lookup as a pull model: axes query the controller bound to their panel and
   dimension during frame preparation.


## Non-Goals

1. Do not add a separate `dvz_panel_link_panzoom()` API.
2. Do not expose mutable controller structs or public controller unions.
3. Do not add a generic string/property controller API.
4. Do not couple scene controller APIs to GLFW, Vulkan, vklite, WebGPU, or native window types.
5. Do not make `LinkedPanelsController` the spatial-navigation link primitive; reserve that concept
   for higher-level crosshair, hover, selection, brushing, and semantic coordination.


## Implementation Slices

### 1. Public Type And Enum Surface

Add or promote:

```c
typedef struct DvzController DvzController;
typedef uint32_t DvzDimMask;
```

Add dimension masks for X, Y, Z, XY, and XYZ. Keep single-dimension query APIs using `DvzDim` where
that is clearer.


### 2. Scene-Owned Controller Storage

Add a scene-owned controller table in the scene internals. The private implementation can use a
tagged union or ops table, but the layout must stay out of installed headers.

Required ownership rules:

1. scene destruction frees controllers once,
2. panels borrow controller handles,
3. panel destruction clears bindings but does not destroy controllers,
4. explicit controller destruction, if exposed, clears or invalidates panel bindings.


### 3. Panzoom First

Move panzoom behind the generic controller handle first. Implement:

```c
DvzController* dvz_panzoom(DvzScene* scene, int flags);
int dvz_panzoom_get_state(const DvzController* controller, DvzPanzoomState* out);
int dvz_panzoom_set_state(DvzController* controller, const DvzPanzoomState* state);
```

Keep the first state struct as a POD snapshot of semantic pan/zoom state, not panel viewport state.


### 4. Panel Binding

Implement:

```c
int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);
```

Validation should reject incompatible family/dimension combinations. Panzoom may bind to X, Y, or
XY. Arcball, fly, and turntable should bind to XYZ in the first implementation.


### 5. Input Routing And Viewport Context

Route input through the panel binding rather than through a controller-owned canonical viewport.

The panel supplies the current viewport and panel-local coordinate context for each event. The
controller stores semantic state only. This is required for linked panels with different sizes and
positions.


### 6. Axis And Domain Query Integration

Add the internal query path that lets axes and panel domain helpers ask the bound controller for the
visible domain of one dimension.

Shared controller identity should imply shared visible domain, not shared axis resources. Each panel
keeps its own axis geometry, tick labels, and cached covered-domain state.


### 7. Legacy API Migration

Migrate existing examples and tests from:

```c
dvz_panel_set_panzoom(panel, router, flags);
DvzPanzoom* pz = dvz_panel_panzoom(panel);
```

to scene-owned controllers and panel binding. During the transition, keep legacy wrappers only if
they make the migration safer; clearly document whether they create a private panel-local
controller.


### 8. Other Controller Families

After panzoom and binding are stable, migrate arcball, fly, and turntable behind `DvzController*`.
Keep family-specific state snapshots and wrong-family validation tests.


## First Linked-Panel Acceptance Tests

1. Two panels bound to one XY panzoom report identical visible X and Y domains.
2. Two panels bound to one X panzoom and separate Y panzooms share X only.
3. Linked panels work with different viewport rectangles and sizes.
4. Rebinding X does not change an existing Y binding.
5. Wrong-family or incompatible-dimension binding fails validation.
6. Destroying a panel does not destroy a shared controller.
7. Typed panzoom state get/set rejects non-panzoom controllers.
8. `examples/c/techniques/linked_panels.c` no longer copies public panzoom fields.


## Documentation And Example Updates

Update these docs or examples as the implementation lands:

1. [`../../spec/scene/interaction/CONTROLLERS.md`](../../spec/scene/interaction/CONTROLLERS.md)
2. [`../../spec/scene/examples/core/LINKED_PANELS_AXES_PANZOOM.md`](../../spec/scene/examples/core/LINKED_PANELS_AXES_PANZOOM.md)
3. [`../../docs/architecture/manual_scene_smoke.md`](../../docs/architecture/manual_scene_smoke.md)
4. `examples/c/techniques/linked_panels.c`
5. public scene API headers and any WASM/API-surface notes touched by the new declarations


## Validation

For narrow API or panzoom-only slices:

```bash
just build
just test scene
git diff --check
```

For live input/routing changes, also run a bounded linked-panel smoke:

```bash
./build/examples/c/techniques/linked_panels 300
```

Use the graphics-test environment guidance in the root `AGENTS.md` when running Vulkan/GLFW paths.
