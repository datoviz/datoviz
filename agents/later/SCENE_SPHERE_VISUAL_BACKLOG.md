# Scene Sphere Visual Backlog

> **Execution Status**
> - **Status:** `LATER / FEATURE BACKLOG`
> - **Updated on:** `2026-05-21`
> - **Purpose:** preserve non-immediate sphere feature work after the core retained impostor path
>   landed.


## Current State

The durable sphere contract lives in
[`../../spec/scene/visuals/SPHERE.md`](../../spec/scene/visuals/SPHERE.md). That spec owns
the stable semantics for canonical `position`/`color`/`radius` data, `dvz_sphere()`,
`dvz_sphere_mode()`, material participation, G-buffer/SSAO behavior, and deferred texture/PBR
lanes.

The active codebase already has:

1. `DVZ_VISUAL_TYPE_SPHERE` wired into retained scene state and emission;
2. `dvz_sphere()`, `dvz_sphere_mode()`, and generic data upload for `position`, `color`, and
   `radius`;
3. GLSL color, alpha-to-coverage, and G-buffer variants;
4. scene tests for sphere emission, mode retention, and SSAO/G-buffer execution;
5. a GLFW SSAO example that exercises retained spheres.

Use this file only for deferred execution sequencing and validation notes. Do not duplicate stable
visual family rules here.


## Deferred Sphere Work

Recommended future commits:

1. Add screen-radius sphere support through the shared `radius_space` contract if a future use case
   needs zoom-invariant sphere markers. Do not revive a sphere constructor flag for this mode.
2. Port or redesign the v0.3 textured and equirectangular sphere lanes as a separate feature slice.
   Keep texture projection semantics in `SPHERE.md` before adding API.
3. Add sphere picking/probing only after the shared picking payload contract can represent
   impostor-hit depth, object id, and sphere index consistently.
4. Extend material coverage only through the shared material layer. Avoid sphere-private lighting
   uniforms unless the shared contract cannot represent the feature.
5. Refresh public examples and gallery coverage after the mode, material, and SSAO paths settle.


## v0.3 Reference

Use `v0.3.5` as a reference, not a compatibility target:

1. `include/datoviz/scene/visuals/sphere.h`
2. `src/scene/visuals/sphere.c`
3. `src/scene/glsl/graphics_sphere.vert`
4. `src/scene/glsl/graphics_sphere.frag`

Useful v0.3 ideas to keep available:

1. compact per-sphere vertex payload;
2. screen-radius mode;
3. texture and equirectangular projection flags;
4. analytic point-sprite sphere reconstruction.

The active v0.4 path should continue to use generic visual data upload for payloads and typed
sphere setters only for behavior.


## Validation

For remaining sphere changes, prefer focused validation:

```text
just build
just test scene
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

If a change adds shader variants, also run the relevant shader-registry and visual-pipeline tests.
