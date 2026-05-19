# Scene Turntable Controller Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-19`
> - **Purpose:** stage a stable-up orbit controller for v0.4 scene panels without mixing it with
>   fly-camera navigation or changing the scene -> FramePlan -> DRP2 runtime contract.


## Durable Contract

Turntable, pivot, input-default, and pivot-marker semantics live in
[../../../spec/scene/interaction/CAMERA_CONTROLLERS.md](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md).

General controller ownership and binding rules live in
[../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

This file tracks implementation stages, focused tests, and validation only.


## Goal

Add turntable navigation for object- and pivot-centered 3D inspection. Turntable should update a
camera pose or model transform before the existing MVP emission path runs. It should not be
implemented as a DRP2, visual, or runtime feature.

The preferred first implementation is an arcball-family controller mode or small sibling
controller, not a fly option.


## Stage 1: Math Core

Implement pure deterministic helpers before router integration:

1. descriptor defaults;
2. eye-from-pivot spherical conversion;
3. pose-from-eye-and-pivot conversion;
4. pitch clamp and yaw wrap;
5. distance clamp;
6. view-plane pan delta from pixel drag;
7. optional model-matrix pivot rotation if model mode is included in the first patch.

Focused tests:

1. default pose looks at pivot with expected distance;
2. horizontal orbit preserves distance and changes yaw;
3. vertical orbit clamps pitch and avoids flipping;
4. dolly clamps distance;
5. panning translates pivot and eye consistently;
6. changing pivot with preserve-eye policy does not move the camera eye.


## Stage 2: Public Controller And Panel Wiring

Add the public header and panel/controller ownership once the math tests are stable.

Implementation tasks:

1. add `include/datoviz/scene/turntable.h` if implemented as a separate controller;
2. include it from `include/datoviz/scene.h`;
3. add panel-owned storage only if this lands before the scene-owned controller table;
4. otherwise expose turntable as a scene-owned `DvzController*` family and bind it with
   `DVZ_DIM_XYZ`;
5. ensure camera-mode turntable creates or reuses a panel camera;
6. keep model-mode turntable compatible with existing model-matrix controller composition.

Focused tests:

1. panel or scene creates and returns a turntable;
2. destruction releases it;
3. resize updates viewport-dependent pan scale;
4. MVP output changes as expected after an orbit.


## Stage 3: Input Router Integration

Add pointer and keyboard handling:

1. subscribe through `DvzInputRouter`;
2. use union input events so gesture-derived drag/double-click events are available;
3. filter pointer events by panel viewport;
4. keep interaction state instance-scoped.

Focused tests:

1. drag inside viewport updates yaw/pitch;
2. drag outside viewport is ignored;
3. wheel changes distance;
4. reset restores initial pose;
5. pan gesture moves pivot and eye together.


## Stage 4: Pivot Sources

Add helper APIs after base turntable behavior is stable:

1. set pivot from explicit point;
2. set pivot from scene or visual bounds;
3. set pivot from a pick/probe result when available;
4. optionally expose a user callback for custom pivot resolution;
5. add transient pivot marker state or an extension point for a later overlay marker.

These helpers should remain scene-layer conveniences. The core controller should only know about a
world-space point.


## Stage 5: Examples And Validation

Add an example that makes the difference between arcball, turntable, and fly clear.

Validation checklist:

1. `git diff --check`;
2. `just build`;
3. `just test scene`;
4. focused controller tests;
5. bounded GLFW smoke if an interactive example is added.

The first turntable patch should stay CPU/test-heavy. Vulkan/GLFW validation is only needed once
the controller is wired into an app example or frame loop.
