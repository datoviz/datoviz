# Mesh and Primitive Volume Occlusion Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define the next narrow slice for letting mesh-like scene visuals appear embedded
>   inside a volume by sampling the existing screen-space volume occlusion depth prepass.


## Durable Contract

Use the volume-occlusion and rollout contract in
[../../../spec/scene/implementation/OCCLUSION_EFFECTS.md](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md).

This file tracks the next consumer-family implementation slice.


## Goal

Extend the current volume-occlusion path beyond volume slices so one ordinary scene visual family
can be attenuated by the panel volume occlusion prepass.

The first implementation should stay narrow:

1. one panel volume occluder;
2. one embedded visual family;
3. one deterministic offscreen render fixture;
4. no broader renderer contract redesign.

Prefer `primitive` first if it avoids material/normal complexity. Move to unlit `mesh` next once
the bind layout and shader path are proven.


## Current Baseline

The existing volume-slice path has focused runtime coverage:

1. identity/offscreen volume-slice dimming;
2. perspective-camera volume-slice dimming;
3. generic scene-occlusion volume-slice dimming;
4. local region behavior with a shrunken volume occluder.

Those tests prove that the graph resource, DRP2 binding, sampled texture, and slice shader
semantics work. The next risk is whether a non-volume visual can participate without colliding with
existing bind layouts and shader variants.


## Implementation Plan

1. Confirm whether a non-volume visual marked `volume_occluded` declares the graph read, requests
   the bind layout, receives runtime bindings, and selects the shader variant.
2. Choose `primitive` or unlit `mesh` as the first target visual.
3. Make `dvz_visual_set_volume_occluded(visual, true)` imply the graph read, occlusion bind layout,
   shader feature mask, and runtime binding only when a panel volume occluder exists.
4. Resolve the non-volume occlusion bind group without disturbing material, image, or scene
   occlusion set usage.
5. Add shader sampling with the same no-hit, in-front, behind-volume, hidden-alpha, and fade
   semantics as the volume-slice path.
6. Add contract tests before pixel tests.
7. Add deterministic offscreen image-difference coverage.


## Contract Tests

The first test should assert:

1. a `volume_occlusion` render pass exists;
2. the volume occlusion depth resource exists;
3. the embedded primitive/mesh draw reads `.volume_occlusion.depth`;
4. the selected shader/pipeline variant indicates volume-occluded behavior;
5. the draw is absent from the occlusion prepass unless it is the panel volume occluder.


## Pixel Tests

Minimum fixture:

1. dense scalar volume occluder;
2. one embedded primitive/mesh visual behind the front volume depth;
3. disabled-versus-enabled capture comparison;
4. assertion that enabled occlusion visibly dims the embedded visual.

Second fixture, after the first is stable:

1. shrunken/clipped volume occluder;
2. compare two screen regions;
3. assert the covered region dims significantly;
4. assert the uncovered region stays close to the disabled baseline.

Keep the fixture visually simple and numeric. Optional PNG dumps should remain env-gated debug
helpers and not be required by tests.


## Validation

Run the narrowest validation loop first:

```sh
just build
direnv exec . ./build/testing/dvztest_scene test_scene_volume
direnv exec . ./build/testing/dvztest_scene test_app_offscreen_volume
direnv exec . ./build/testing/dvztest_scene <new-contract-test-name>
direnv exec . ./build/testing/dvztest_scene <new-app-test-name>
git diff --check
```

For changes touching descriptor refresh, bind group layouts, or command stream ordering, also run a
broader scene slice:

```sh
direnv exec . ./build/testing/dvztest_scene test_scene_volume_slice_uses
direnv exec . ./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts
```


## Non-Goals

This slice should not:

1. support every visual family;
2. redesign the volume occlusion texture semantics;
3. add physically correct volume/object ray integration;
4. solve WBOIT/depth-peel/material interactions broadly;
5. fork a parallel renderer contract.

Finish one embedded primitive or unlit mesh path first, with contract and pixel coverage.
