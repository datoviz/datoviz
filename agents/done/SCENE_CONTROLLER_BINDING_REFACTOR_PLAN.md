# Scene Controller Binding Refactor

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-21`
> - **Purpose:** final record for the scene-owned controller binding refactor.


## Result

The scene controller model now uses scene-owned `DvzController*` handles for panzoom, arcball, fly,
and turntable. Panels borrow controllers through `dvz_panel_bind_controller()` and route input
through `dvz_panel_connect_input()`, which supplies panel-local viewport context before controller
state is updated.

Implemented public shape:

```c
DvzController* dvz_panzoom(DvzScene* scene, const DvzPanzoomDesc* desc);
DvzController* dvz_arcball(DvzScene* scene, const DvzArcballDesc* desc);
DvzController* dvz_fly(DvzScene* scene, const DvzFlyDesc* desc);
DvzController* dvz_turntable(DvzScene* scene, const DvzTurntableDesc* desc);

int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);
int dvz_panel_connect_input(DvzPanel* panel, DvzInputRouter* router);
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);
```

Old panel-owned controller entry points were removed from the public scene API. Existing C and Qt
examples were migrated to scene-owned construction and panel binding.


## Coverage

Focused tests now cover:

1. shared XY panzoom visible domains across panels;
2. shared X with independent Y panzoom bindings;
3. rebinding X without changing Y;
4. panel-local input routing and viewport filtering for panzoom, arcball, and turntable;
5. wrong-dimension rejection for camera controller families;
6. panel destruction preserving scene-owned controller payloads;
7. shared fly update-once behavior across two panels;
8. axis/domain queries through bound panzoom controllers.


## Remaining Follow-Up

Typed POD state get/set APIs remain the main follow-up before generated bindings should treat the
controller API as final. Durable requirements are in
[`../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md)
and [`../../spec/scene/api/WASM_PORTABILITY.md`](../../spec/scene/api/WASM_PORTABILITY.md).


## Validation

Recorded during implementation:

```text
just build
just test scene
```

`just test scene` passed all new controller-focused tests. The broad scene run hit an unrelated
order-sensitive GPU volume occlusion app failure twice; the same case passed when rerun in
isolation with `just test scene/app-offscreen/volume_slice_scene_occlusion_dimming`.
