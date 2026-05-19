# Screen-Space Scene Occlusion Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the remaining generic scene occlusion producer/consumer sequence after the
>   durable model and graph contract moved to `spec/scene`.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md)
2. [`../../../spec/scene/visuals/VOLUME.md`](../../../spec/scene/visuals/VOLUME.md)
3. [`../../../spec/scene/visuals/MESH.md`](../../../spec/scene/visuals/MESH.md)

Use this file only for implementation sequencing and first-commit guidance. Do not duplicate the
scene occlusion visual model, resource/pass contract, shader feature policy, or validation
expectations here.

Generic scene occlusion remains a pragmatic screen-space approximation for hiding or attenuating
embedded visuals behind volumes and surface shells. It is not a physically correct unified
volume/geometry renderer.


## Remaining Generic Occlusion Work

Recommended follow-up commits:

1. Add retained scene occlusion flags and descriptors with graph-only tests.
2. Add graph resource/pass naming helpers only where the generic occlusion contract needs them.
3. Add a mesh, primitive, or sphere depth prepass that writes front depth for surface occluders.
4. Wire the Allen atlas mesh as a first surface occluder after the prepass contract is tested.
5. Add a generic occlusion bind group and `scene_occlusion.glsl` include path for consumers.
6. Route the Allen slice through generic scene occlusion once the volume-slice behavior matches the
   existing volume-specific path.
7. Migrate the current volume front-depth prepass into the generic scene occlusion producer path.
8. Add a merge pass for mesh and volume depth when both producers are active.
9. Remove or deprecate volume-specific occlusion plumbing only after equivalent generic behavior is
   covered.


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
