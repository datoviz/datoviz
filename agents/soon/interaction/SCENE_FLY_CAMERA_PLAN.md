# Scene Fly Camera Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
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


## Remaining Fly Work

Recommended follow-up commits:

1. Add deterministic fly pose math independent from input callbacks: yaw/pitch, speed, key-state
   bits, optional pivot, reset state, and camera writeback helper.
2. Add tests for default pose, `lookat` initialization, pitch clamp, free movement, plane movement,
   and reset behavior.
3. Add input-router integration where keyboard handlers set/release movement bits and frame update
   applies movement from key state and `dt`, not OS key repeat cadence.
4. Wire fly as a scene-owned `DvzController*` family if controller binding lands first; otherwise
   keep any temporary panel-owned path clearly marked as transitional.
5. Integrate frame update once per app frame for panels with a fly controller and camera, with a
   conservative `dt` clamp after stalls.
6. Add pivot helpers only after base fly movement is stable.
7. Add a focused GLFW example with a bounded smoke path once runtime wiring exists.


## Focused Tests

1. WASD and arrow keys set equivalent movement bits.
2. Releasing keys clears movement bits.
3. Shift changes speed while pressed.
4. Pointer drag updates yaw/pitch.
5. Reset restores the initial pose.
6. Offscreen or app frame loops advance movement while a key is held and stop after release.
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
