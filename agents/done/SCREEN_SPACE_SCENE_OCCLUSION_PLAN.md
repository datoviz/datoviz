# Screen-Space Scene Occlusion Completed Baseline

> **Execution Status**
> - **Status:** `IMPLEMENTED BASELINE / FOLLOW-UP RECORD`
> - **Updated on:** `2026-05-25`
> - **Purpose:** record the landed generic scene occlusion producer/consumer baseline after the
>   durable model and graph contract moved to `spec/scene`.


## Current State

Durable contracts live in:

1. [`../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../spec/scene/implementation/OCCLUSION_EFFECTS.md)
2. [`../../spec/scene/visuals/VOLUME.md`](../../spec/scene/visuals/VOLUME.md)
3. [`../../spec/scene/visuals/MESH.md`](../../spec/scene/visuals/MESH.md)

Use this file only for implementation history, residual sequencing, and validation guidance. Do not
duplicate the scene occlusion visual model, resource/pass contract, shader feature policy, or
validation expectations here.

Generic scene occlusion remains a pragmatic screen-space approximation for hiding or attenuating
embedded visuals behind volumes and surface shells. It is not a physically correct unified
volume/geometry renderer.


## Landed Generic Occlusion Baseline

The current baseline includes:

1. retained scene occlusion flags and descriptors;
2. graph resource/pass naming for scene occlusion;
3. graph-backed producer and consumer plumbing through the scene FramePlan and DRP2 runtime path;
4. shared `scene_occlusion.glsl` consumer include plumbing;
5. Allen slice routing through the generic scene occlusion path where supported.


## Remaining Generic Occlusion Work

Recommended follow-up commits:

1. Add or broaden mesh, primitive, and sphere depth producer coverage where the visual family
   contract is explicit.
2. Keep the Allen atlas mesh surface-occluder behavior covered as mesh/material handling evolves.
3. Merge mesh and volume depth cleanly when both producers are active.
4. Remove or deprecate volume-specific occlusion plumbing only after equivalent generic behavior is
   covered.
5. Keep unsupported visual families explicit and tested.


## Focused Tests

1. Hidden or visible occluder toggles do not produce invalid runtime streams.
2. Mesh or primitive occluder depth pass appears before occluded visual passes.
3. Occluded visual passes declare a graph read on scene occlusion depth.
4. Mixed WBOIT and blended passes remain valid.
5. The Allen atlas mesh uses the intended alpha mode when acting as an occluder.
6. Shader and pipeline feature keys differ for occluded versus non-occluded variants.


## Non-Goals

1. No physically based volume/geometry integration.
2. No monolithic renderer requirement for all visuals.
3. No duplicated `_occluded.frag` shader families.
4. No assumption that WBOIT behaves like true opaque rendering.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes, use:

```text
just build
just test scene
```

Add offscreen image-difference or bounded Allen-example smoke coverage before relying on a live GUI
path.
