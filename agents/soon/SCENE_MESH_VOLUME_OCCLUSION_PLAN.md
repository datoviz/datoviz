# Mesh and Primitive Volume Occlusion Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-17`
> - **Purpose:** define the next narrow slice for letting mesh-like scene visuals appear embedded
>   inside a volume by sampling the existing screen-space volume occlusion depth prepass.


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

The existing volume-slice path now has focused runtime coverage:

1. identity/offscreen volume-slice dimming;
2. perspective-camera volume-slice dimming;
3. generic scene-occlusion volume-slice dimming;
4. local region behavior with a shrunken volume occluder.

Those tests prove that the graph resource, DRP2 binding, sampled texture, and slice shader
semantics work. The next risk is whether a non-volume visual can participate without colliding with
existing bind layouts and shader variants.


## Implementation Plan

### 1. Confirm the Support Boundary

Inspect the primitive and mesh visual descriptors, shader feature masks, bind layout requirements,
and runtime bind-group resolution.

Confirm whether a non-volume visual marked `volume_occluded` currently:

1. declares a sampled volume-occlusion resource read;
2. requests a compatible bind group layout;
3. receives the occlusion depth texture, sampler, and parameters at runtime;
4. compiles a shader variant with occlusion logic.

Expected result: this is probably only wired for volume shaders today.


### 2. Choose the First Target Visual

Start with the simplest visual family that can prove the path:

1. `primitive` if its shader and bind layout are simpler;
2. otherwise unlit `mesh`;
3. defer lit/material mesh variants until the unlit path is stable.

Avoid starting with material-model, WBOIT, depth-peel, or instanced mesh variants unless the simpler
path cannot represent the target behavior.


### 3. Extend the Draw Contract

For the selected visual family, make `dvz_visual_set_volume_occluded(visual, true)` imply:

1. the draw samples the panel `.volume_occlusion.depth` resource;
2. the draw needs the volume-occlusion bind layout;
3. the shader feature mask selects a volume-occluded variant;
4. the graph pass read is recorded only when a panel volume occluder exists.

Keep unsupported visual families explicit: either leave `volume_occluded` as a no-op with a test
showing no occlusion resources are requested, or add a warning/failure path if that matches the
current scene API style better.


### 4. Add Runtime Binding

Resolve a non-volume occlusion bind group without disturbing existing set usage.

The first pass should verify:

1. no collision with image, material, or scene-occlusion bind groups;
2. stable pipeline layout ordering;
3. descriptor refresh still works after resource recreation;
4. no dummy texture path accidentally masks a missing graph read.

If primitive/mesh already use set 1 for another resource, prefer the existing scene shader ABI
layout rules over adding an ad-hoc special case.


### 5. Add Shader Sampling

Add a shader variant for the selected visual family that samples the volume-occlusion depth texture
using `gl_FragCoord`.

Required behavior:

1. no occlusion when sampled depth is the no-hit sentinel;
2. no occlusion when the fragment is in front of the sampled volume depth;
3. attenuate alpha/color when the fragment is behind the volume depth;
4. use the same hidden-alpha and fade-distance semantics as the slice path where practical.

For the first slice, prefer alpha attenuation over hard discard. Hard discard can be added later as
a debug option if it proves useful.


### 6. Add Contract Tests

Add a focused FramePlan/DRP2 shape test before the pixel test.

The test should assert:

1. a `volume_occlusion` render pass exists;
2. the volume occlusion depth resource exists;
3. the embedded primitive/mesh draw reads `.volume_occlusion.depth`;
4. the selected shader/pipeline variant indicates volume-occluded behavior;
5. the draw is absent from the occlusion prepass unless it is the panel volume occluder.


### 7. Add Pixel Tests

Add deterministic offscreen app coverage.

Minimum fixture:

1. dense scalar volume occluder;
2. one embedded primitive/mesh visual behind the front volume depth;
3. disabled-vs-enabled capture comparison;
4. assertion that enabled occlusion visibly dims the embedded visual.

Second fixture, after the first is stable:

1. shrunken/clipped volume occluder;
2. compare two screen regions;
3. assert the covered region dims significantly;
4. assert the uncovered region stays close to the disabled baseline.

Keep the fixture visually simple and numeric. Optional PNG dumps should remain env-gated debug
helpers and not be required by the tests.


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
