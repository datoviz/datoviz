# Scene Fly Camera Implementation Record

> **Execution Status**
> - **Status:** `IMPLEMENTED / ARCHIVED`
> - **Updated on:** `2026-05-21`
> - **Purpose:** record the completed fly/FPS/pivot camera implementation and the current
>   validation surface.


## Current State

Durable fly, pivot-orbit, input-default, and pivot-marker semantics live in
[`../../spec/scene/interaction/CAMERA_CONTROLLERS.md`](../../spec/scene/interaction/CAMERA_CONTROLLERS.md).
General ownership and panel-binding rules live in
[`../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md`](../../spec/scene/decisions/CONTROLLER_BINDING_MODEL.md).

This is a completed implementation record. Do not treat it as an active `soon/` execution plan.
The fly camera updates the panel camera before the existing MVP emission path runs; it did not
change FramePlan, DRP2, visual, or runtime contracts.


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
8. Public scene API exposes `dvz_scene_fly()`, `dvz_controller_fly()`,
   `dvz_panel_bind_controller()`, `dvz_panel_controller()`, and compatibility
   `dvz_panel_set_fly()` / `dvz_panel_fly()` helpers.
9. `examples/c/showcase/lidar.c` exercises the scene-owned fly controller through a GLFW app
   window and accepts a bounded frame count for smoke runs, although it is a showcase example that
   depends on prepared local LIDAR data.


## Residual Follow-Ups

1. A small fly-only GLFW smoke example is still useful, but example and gallery planning belongs
   under `spec/scene/examples/`, not as an active fly implementation blocker.
2. The broader controller-binding migration for panzoom, arcball, and turntable remains tracked by
   [`../soon/interaction/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md`](../soon/interaction/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md).


## Focused Test Coverage

1. WASD and arrow keys set equivalent movement bits.
2. Releasing keys clears movement bits.
3. Shift changes speed while pressed.
4. Pointer drag updates yaw/pitch.
5. Reset restores the initial pose.
6. Router/figure loops advance movement while a key is held and stop after release.
7. Fly state remains instance-scoped across unrelated figures or panels.


## Current Code Anchors

1. `src/scene/fly.c`: deterministic fly pose, movement, input, pivot, and camera writeback logic.
2. `src/scene/scene.c`: scene-owned fly controller storage, panel binding, figure update, and
   compatibility panel helpers.
3. `src/app/app.c`: per-frame fly updates before DRP2 frame emission and request-frame scheduling
   while movement is active.
4. `src/scene/tests/fly.c`: focused unit and scene tests for the completed fly surface.
5. `examples/c/showcase/lidar.c`: live GLFW app path using the scene-owned fly controller.


## Validation

The archived status was verified on `2026-05-21` by checking the current code and test surface for
the public fly API, scene-owned controller path, frame update integration, pivot marker path, and
registered fly tests. This cleanup was docs-only, so the relevant validation is:

1. `rg` for stale links to the old `soon/interaction` fly-camera plan
2. `git diff --check`
3. `git status --short`
