# Scene Turntable Controller

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-21`
> - **Purpose:** final record for the stable-up turntable camera controller.


## Result

Turntable is implemented as a separate scene-owned controller family, not an arcball option.
It updates panel camera poses through normal scene MVP emission and remains outside DRP2/runtime
resource ownership.

Implemented behavior:

1. stable-up camera orbit around a pivot;
2. spherical eye/pivot conversion with yaw wrap, pitch clamp, and distance clamp;
3. left-drag orbit, wheel dolly, middle/right-drag pivot pan, and double-click reset;
4. explicit pivot changes preserve the current camera eye and recompute yaw, pitch, and distance;
5. panel-local input routing through `dvz_panel_connect_input()`;
6. scene-owned construction through `dvz_turntable(scene, desc)`;
7. camera writeback through `dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ)`.


## Coverage

Focused tests cover:

1. default pose and expected camera distance;
2. horizontal orbit preserving distance;
3. pivot changes preserving the camera eye;
4. panning translating pivot and eye consistently;
5. pitch clamp and distance clamp behavior;
6. double-click reset;
7. viewport-filtered panel input;
8. scene-owned turntable binding and panel camera MVP output.


## Remaining Follow-Up

Optional polish remains outside this plan:

1. pivot helpers from visual or scene bounds;
2. pick/probe-derived pivot selection;
3. a gallery example comparing arcball, turntable, and fly side by side.

Durable semantics live in
[`../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../spec/scene/interaction/CAMERA_CONTROLLERS.md).
