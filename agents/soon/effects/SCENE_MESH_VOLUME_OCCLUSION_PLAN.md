# Mesh And Primitive Volume Occlusion Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the next narrow consumer-family slice for letting mesh-like visuals sample
>   the existing screen-space volume occlusion depth prepass.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md)
2. [`../../../spec/scene/visuals/VOLUME.md`](../../../spec/scene/visuals/VOLUME.md)
3. [`../../../spec/scene/visuals/MESH.md`](../../../spec/scene/visuals/MESH.md)

Use this file only for the immediate mesh/primitive consumer pickup order. Do not duplicate volume
occlusion texture semantics, scene occlusion resource/pass rules, shader feature policy, or rollout
contracts here.

The active volume-slice path already validates graph resources, DRP2 bindings, sampled texture
usage, and slice shader semantics for identity/offscreen, perspective-camera, generic
scene-occlusion, and local-region volume occlusion cases.


## Remaining Mesh/Primitive Consumer Work

Recommended follow-up commits:

1. Choose `primitive` or unlit `mesh` as the first non-volume consumer; prefer `primitive` if it
   avoids material and normal complexity.
2. Confirm that a non-volume visual marked `volume_occluded` declares the graph read, requests the
   occlusion bind layout, receives runtime bindings, and selects the shader variant only when a
   panel volume occluder exists.
3. Resolve the non-volume occlusion bind group without disturbing material, image, or generic scene
   occlusion set usage.
4. Add shader sampling with the same no-hit, in-front, behind-volume, hidden-alpha, and fade
   semantics as the volume-slice path.
5. Add contract tests before pixel tests.
6. Add deterministic offscreen image-difference coverage after the stream contract is stable.
7. Keep broader family support, WBOIT/depth-peel/material interactions, and physically correct
   volume/object integration deferred.


## First Contract Tests

1. A `volume_occlusion` render pass exists when source and target are present.
2. The volume occlusion depth resource exists.
3. The embedded primitive or mesh draw reads `.volume_occlusion.depth`.
4. The selected shader and pipeline variant indicate volume-occluded behavior.
5. The embedded draw is absent from the occlusion prepass unless it is the panel volume occluder.


## Pixel Tests

Start with a minimal fixture:

1. dense scalar volume occluder;
2. one embedded primitive or mesh visual behind the front volume depth;
3. disabled-versus-enabled capture comparison;
4. assertion that enabled occlusion visibly dims the embedded visual.

After that, add a shrunken or clipped occluder fixture that verifies covered regions dim and
uncovered regions remain close to the disabled baseline.


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
direnv exec . ./build/testing/dvztest_scene test_scene_volume
direnv exec . ./build/testing/dvztest_scene test_app_offscreen_volume
```

For descriptor refresh, bind group layout, or command-order changes, add the narrow new contract
and app tests before broadening the scene slice.
