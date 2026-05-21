# Scene Turntable Controller Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / PARTIAL IMPLEMENTATION`
> - **Updated on:** `2026-05-21`
> - **Purpose:** track remaining stable-up orbit controller work without duplicating durable
>   camera-controller or binding semantics.


## Current State

Durable turntable, pivot, input-default, and pivot-marker semantics live in
[`../../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md).
General ownership and panel-binding rules live in
[`../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

Use this file only for remaining implementation sequencing, focused tests, and validation.
Turntable updates a camera pose before the existing MVP emission path runs; it is not implemented
as a DRP2, visual, or runtime feature.


## Completed Turntable Work

Implemented slices:

1. Deterministic turntable math exists in `src/scene/turntable.c`: descriptor defaults,
   eye/pivot spherical conversion, yaw wrap, pitch clamp, distance clamp, dolly, and view-plane
   pan.
2. Pointer input and input-router integration exist for left-drag orbit, middle/right-drag pan,
   wheel dolly, double-click reset, resize, and panel viewport filtering.
3. The transitional panel-owned API exists through `dvz_panel_set_turntable()` and
   `dvz_panel_turntable()`.
4. Camera-mode turntable creates or reuses a panel camera and updates the existing panel MVP path.
5. Focused tests cover default pose, horizontal orbit distance, preserve-eye pivot changes,
   pan translating pivot and eye, and panel camera integration.
6. The agreed first interaction contract is: left-drag orbit, wheel dolly, middle/right-drag pan,
   double-click reset, and explicit API pivot set preserving the current eye.


## Remaining Turntable Work

Recommended follow-up commits:

1. Expose turntable as a scene-owned `DvzController*` family after the broader controller-binding
   path supports non-fly controller families.
2. Add missing focused tests for vertical pitch clamp, distance clamp, dolly behavior, and outside
   viewport drag rejection.
3. Add focused tests for double-click reset and explicit pivot set preserving the current eye.
4. Add pivot helpers for scene or visual bounds, and later pick/probe-derived pivots after base
   orbit behavior is stable.
5. Add an example that makes the difference between arcball, turntable, and fly clear.


## Focused Tests

Landed:

1. Default pose looks at the pivot with the expected distance.
2. Horizontal orbit preserves distance and changes yaw.
3. Panning translates pivot and eye consistently.
4. Changing pivot with preserve-eye policy does not move the camera eye.
5. Panel turntable creates/reuses a camera and feeds panel MVP emission.

Still needed:

1. Vertical orbit clamps pitch and avoids flipping.
2. Dolly clamps distance.
3. Drag outside the panel viewport is ignored.
4. Double-click resets the initial pose.
5. Scene-owned turntable binding works once the generic binding path supports turntable.


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
