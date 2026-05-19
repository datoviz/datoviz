# Scene Turntable Controller Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining stable-up orbit controller work without duplicating durable
>   camera-controller or binding semantics.


## Current State

Durable turntable, pivot, input-default, and pivot-marker semantics live in
[`../../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md).
General ownership and panel-binding rules live in
[`../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

Use this file only for implementation sequencing, focused tests, and validation. Turntable should
update a camera pose or model transform before the existing MVP emission path runs; it should not be
implemented as a DRP2, visual, or runtime feature.


## Remaining Turntable Work

Recommended follow-up commits:

1. Add pure deterministic math helpers first: descriptor defaults, eye/pivot spherical conversion,
   pose conversion, yaw wrap, pitch and distance clamps, and view-plane pan delta.
2. Add tests for orbit, dolly, pan, pivot changes, and clamp behavior before wiring input events.
3. Expose turntable as a scene-owned `DvzController*` family if controller binding lands first;
   otherwise keep any temporary panel-owned path clearly transitional.
4. Ensure camera-mode turntable creates or reuses a panel camera and updates the existing MVP path.
5. Add input-router integration for drag, wheel, reset, and panel viewport filtering.
6. Add pivot helpers for explicit points, scene or visual bounds, and later pick/probe-derived
   pivots after base orbit behavior is stable.
7. Add an example that makes the difference between arcball, turntable, and fly clear.


## Focused Tests

1. Default pose looks at the pivot with the expected distance.
2. Horizontal orbit preserves distance and changes yaw.
3. Vertical orbit clamps pitch and avoids flipping.
4. Dolly clamps distance.
5. Panning translates pivot and eye consistently.
6. Changing pivot with preserve-eye policy does not move the camera eye.
7. Drag outside the panel viewport is ignored.


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

Run a bounded GLFW smoke only after the controller is wired into an app example or frame loop.
