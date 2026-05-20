# Scene Fly Camera Follow-Up

> **Execution Status**
> - **Status:** `IMPLEMENTED / LIVE-SMOKE FOLLOW-UP`
> - **Updated on:** `2026-05-20`
> - **Purpose:** track remaining fly/FPS/pivot camera implementation work without redefining
>   durable camera-controller semantics.


## Current State

Durable fly, pivot-orbit, input-default, and pivot-marker semantics live in
[`../../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../../spec/scene/interaction/CAMERA_CONTROLLERS.md).
General ownership and panel-binding rules live in
[`../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

Use this file only for implementation sequencing, focused tests, and validation. The fly camera
should update the panel camera before the existing MVP emission path runs; it should not change
FramePlan, DRP2, visual, or runtime contracts.


## Completed Fly Work

Implemented follow-up slices:

1. Deterministic fly pose math exists independently from input callbacks: yaw/pitch, speed,
   key-state bits, optional pivot, reset state, and camera writeback helper.
2. Focused tests cover default pose, `lookat` initialization, pitch clamp, free movement, plane
   movement, and reset behavior.
3. Input-router integration sets/releases movement bits, and frame update applies movement from
   key state and `dt`, not OS key repeat cadence.
4. Fly now has a scene-owned `DvzController*` path with `DVZ_DIM_MASK_XYZ` panel binding and a
   compatibility `dvz_panel_set_fly()` wrapper returning the borrowed fly payload.
5. App/figure frame update advances fly controllers once per frame with a conservative `dt` clamp
   after stalls.
6. Pivot helpers exist for preserve-eye pivot changes and orbiting around the active pivot.
7. A transient point-based pivot marker is realized for bound fly panels while pivot feedback is
   visible.


## Remaining Fly Work

1. Add a focused GLFW example with a bounded smoke path once runtime wiring exists.
2. Finish the broader controller-binding migration for panzoom, arcball, and turntable so the
   whole navigation stack uses the same opaque controller ownership model.


## Focused Tests

1. WASD and arrow keys set equivalent movement bits.
2. Releasing keys clears movement bits.
3. Shift changes speed while pressed.
4. Pointer drag updates yaw/pitch.
5. Reset restores the initial pose.
6. Router/figure loops advance movement while a key is held and stop after release.
7. Fly state remains instance-scoped across unrelated figures or panels.


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

Once a live example exists, add a bounded smoke such as:

```text
./build/examples/c/hello_fly_glfw 60
```
