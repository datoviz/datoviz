# Scene Fly Camera Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-19`
> - **Purpose:** stage v0.4 fly/FPS/pivot camera controller work without changing the scene ->
>   FramePlan -> DRP2 runtime contract.


## Durable Contract

Fly, pivot-orbit, input-default, and pivot-marker semantics live in
[../../../spec/scene/interaction/CAMERA_CONTROLLERS.md](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md).

General controller ownership and binding rules live in
[../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

This file tracks implementation stages, focused tests, and validation only.


## Goal

Add a v0.4 scene navigation controller that reuses the panel camera as the single source for
view/projection matrices. The controller owns interactive navigation state and updates the camera.
Rendering, frame planning, runtime resources, visual transforms, and DRP2 emission remain
unchanged.

The first implementation should reuse useful v0.3 fly math ideas without copying v0.3 event/app
wiring. Movement must be driven from key state and frame `dt`, not OS key repeat cadence.


## Stage 1: Core Math And Public Header

Add `include/datoviz/scene/fly.h` and `src/scene/fly.c`, or the equivalent controller-family files
if the scene-owned controller table lands first.

Implement:

1. `DvzFly` state, flags, mode, viewport size, pose, initial pose, speed values, key-state bits,
   optional pivot, and optional camera pointer;
2. pose math from v0.3, updated for v0.4 allocation and style rules;
3. deterministic movement helpers independent from input callbacks;
4. an internal helper that writes `eye`, `target`, and `up` to a `DvzCamera`.

Focused tests:

1. default pose looks down `-Z`;
2. `initial_lookat()` computes yaw/pitch correctly;
3. pitch clamp prevents singular up/front vectors;
4. free movement follows the full front vector;
5. plane movement projects forward/right onto the configured plane;
6. reset restores pose and clears transient key/interaction state.


## Stage 2: Input Router Integration

Add pointer, keyboard, connect/disconnect, and update entry points.

Keyboard handlers should set/release key state. The update function should apply movement based on
current key state, speed, modifiers, and `dt`.

Focused tests:

1. WASD and arrow keys set equivalent movement bits;
2. release clears movement bits;
3. Shift changes speed while pressed;
4. pointer drag updates yaw/pitch;
5. double-click or reset key restores the initial pose.


## Stage 3: Panel Or Scene-Owned Controller Wiring

If this lands before the scene-owned controller refactor:

1. add `DvzFly* fly` to `DvzPanel`;
2. destroy it in `dvz_panel_destroy()`;
3. resize it in `dvz_figure_resize()`;
4. add a descriptor-based `dvz_panel_set_fly()` and `dvz_panel_fly()`;
5. ensure setting fly creates or reuses a panel camera.

If the controller-binding refactor lands first, expose fly as a scene-owned `DvzController*` family
and bind it to panels with `DVZ_DIM_XYZ`.

Focused tests:

1. panel or scene owns and returns the fly controller;
2. destruction releases fly without leaking ownership;
3. resize updates fly viewport and camera aspect;
4. fly and camera compose through the panel MVP path.


## Stage 4: App Frame Update

Integrate fly update into the app/scene frame path once per frame for panels with an attached fly
controller and camera.

Requirements:

1. use an app-owned frame delta, clamped to a conservative max to avoid huge jumps after stalls;
2. mark frame emission dirty or ensure the next emitted MVP reflects the updated camera pose;
3. keep update state instance-scoped.

Focused tests:

1. offscreen/app frame loop advances movement while a key is held;
2. releasing the key stops movement;
3. runtime reuse does not accumulate controller state across unrelated figures or panels.


## Stage 5: Pivot Orbit

Add optional pivot helpers after the base fly controller is stable:

1. set, clear, query, and look-at-pivot helpers;
2. direct orbit math helper;
3. optional pointer gesture for pivot orbit;
4. optional transient pivot marker state or extension point.

Focused tests:

1. orbit preserves distance to pivot;
2. orbit updates camera look direction toward pivot;
3. clearing pivot returns to normal fly behavior;
4. moving the pivot recomputes yaw, pitch, and pivot distance with preserve-eye policy;
5. pivot marker state, if included, becomes transiently visible after pivot changes and while
   orbiting.


## Stage 6: Example And Smoke Validation

Add a focused GLFW example, likely `examples/c/hello_fly_glfw.c`, using a mesh or point-cloud
scene.

Validation checklist:

1. `git diff --check`;
2. `just build`;
3. `just test scene`;
4. focused input tests if split into a narrower runner;
5. bounded GLFW smoke, for example `./build/examples/c/hello_fly_glfw 60`.

For graphics-path validation, prefer the unsandboxed or `direnv exec .` approach used by existing
Vulkan/GLFW tests when needed.
