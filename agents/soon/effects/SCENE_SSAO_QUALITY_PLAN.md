# Scene SSAO Quality Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define the next quality upgrades for the landed scene SSAO slice without replacing
>   the active FramePlan graph and DRP2/vklite runtime path.


## Durable Contract

The quality target, shader model, parameter semantics, and blur contract are recorded in
[../../../spec/scene/implementation/OCCLUSION_EFFECTS.md](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md).

This file tracks pickup order, example changes, and validation.


## Context

The current SSAO implementation validates the graph shape and runtime mechanics:

1. eligible visuals write normal/depth information into a G-buffer pass;
2. a fullscreen SSAO pass samples those textures;
3. a fullscreen composite pass darkens the final target;
4. sphere impostors feed analytic normals and a linear view-distance value.

The next quality work should improve shader behavior without changing the scene -> FramePlan graph
-> DRP2 -> vklite route.


## FramePlan And Runtime Changes

Extend the existing graph-backed path rather than replacing it:

1. keep the optional SSAO blur role and graph pass explicit;
2. keep blur output as a graph texture only when blur is enabled;
3. keep sampled bind groups based on graph resource ids;
4. add inverse projection or reconstruction parameters to the SSAO params uniform;
5. pass viewport/scissor information consistently with per-panel rendering.


## Example Changes

`hello_sphere_ssao_glfw` should remain the visual tuning surface:

1. keep toggles for SSAO and auto-rotation;
2. keep sphere size and material controls;
3. expose radius, bias, strength, sample count, and blur;
4. optionally expose a quality preset selector: `fast`, `balanced`, `high`.

The example should include enough overlapping spheres at different depths to reveal whether
occlusion remains stable while zooming.


## Implementation Order

Recommended commits:

1. Add inverse projection / viewport reconstruction parameters to the SSAO uniform upload path.
2. Rework the SSAO shader to reconstruct view-space position from depth.
3. Replace the fixed 2D kernel with a normal-oriented hemisphere kernel.
4. Add deterministic per-pixel kernel rotation.
5. Harden the bilateral blur graph pass and shader if the current implementation is still minimal.
6. Update the example UI defaults and labels.
7. Add offscreen smoke coverage comparing SSAO enabled, disabled, and blur enabled.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_scene hello_sphere_ssao_glfw -j 8
./build/testing/dvztest_scene test_scene_ssao
./build/testing/dvztest_scene test_scene_sphere_ssao_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

Before public API changes:

```text
just test scene
just spec-check
```
