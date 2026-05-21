# Scene Sphere Visual

> **Execution Status**
> - **Status:** `DONE / BASELINE LANDED`
> - **Updated on:** `2026-05-21`
> - **Purpose:** record the retained v0.4 sphere visual baseline and point future work to the
>   durable spec/backlog.


## Landed Baseline

The durable sphere contract lives in
[`../../spec/scene/visuals/SPHERE.md`](../../spec/scene/visuals/SPHERE.md).

The active codebase has:

1. `DVZ_VISUAL_TYPE_SPHERE` wired into retained scene state and emission;
2. `dvz_sphere()`, `dvz_sphere_mode()`, and generic data upload for `position`, `color`, and
   `radius`;
3. fast and raycast impostor modes;
4. GLSL color, alpha-to-coverage, and G-buffer variants;
5. scene tests for sphere emission, mode retention, and SSAO/G-buffer execution;
6. GLFW and showcase examples that exercise retained spheres.


## Deferred Work

Non-immediate sphere feature backlog now lives in
[`../later/SCENE_SPHERE_VISUAL_BACKLOG.md`](../later/SCENE_SPHERE_VISUAL_BACKLOG.md). Keep stable
semantics in `SPHERE.md`; use the backlog only for execution sequencing when a future sphere
feature becomes active.


## Validation Record

Focused validation recorded for the landed slice included:

```text
cmake --build build --target dvztest_scene hello_sphere_ssao_glfw -j 8
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
./build/testing/dvztest_scene test_scene_sphere_ssao_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```
