# Screen-Space Scene Occlusion Plan


## Durable Contract

The scene-occlusion visual model, resource/pass contract, shader feature policy, and validation
expectations live in
[../../../spec/scene/implementation/OCCLUSION_EFFECTS.md](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md).

This file tracks the remaining implementation phases and first commit sequence.


## Goal

Add a generic screen-space occlusion system so ordinary visuals can be visually embedded inside
volumes and surface shells without relying on volume-specific shader paths.

This is a pragmatic scientific-visualization approximation, not a physically correct unified
volume/geometry renderer. It should make cases such as Allen atlas shells hiding internal slices,
small meshes embedded in translucent volumes, and image planes inside volumes look coherent.


## Non-Goals

1. Do not implement physically based volume/geometry integration.
2. Do not require every visual to use one monolithic renderer.
3. Do not duplicate GLSL files for occluded and non-occluded variants.
4. Do not make WBOIT behave like true opaque rendering.


## Implementation Phases

Phase 1: Design scaffolding

1. add scene occlusion flags and descriptors;
2. add frame-graph resource/pass naming helpers;
3. add tests for graph emission only.

Phase 2: Mesh occlusion producer

1. add mesh/primitive/sphere depth prepass;
2. write `R32_SFLOAT` front depth;
3. wire Allen atlas mesh as occluder.

Phase 3: Generic occlusion consumer for volume slice

1. add shared occlusion bind group;
2. add `scene_occlusion.glsl`;
3. compile `volume_slice.frag` with `DVZ_SCENE_OCCLUSION` when needed;
4. route Allen slice through generic occlusion.

Phase 4: Volume producer migration

1. move current volume front-depth prepass into the generic scene occlusion producer path;
2. add merge pass for mesh + volume depth;
3. remove or deprecate volume-specific occlusion plumbing once equivalent behavior is covered.

Phase 5: Additional consumers

1. add occlusion support to primitive/mesh/image/sphere/point shader paths;
2. keep non-occluded visuals zero-cost.

Phase 6: GUI and polish

1. expose scene occlusion controls in Allen GUI;
2. reorganize controls into clear sections;
3. keep advanced producer controls hidden unless needed.


## Tests

Add focused coverage:

1. hidden/visible occluder toggles do not produce invalid runtime streams;
2. mesh occluder depth pass appears before occluded visual passes;
3. occluded visual pass declares graph read on scene occlusion depth;
4. mixed WBOIT and blended passes remain valid;
5. fully opaque atlas mesh uses opaque alpha mode in the Allen example;
6. shader/pipeline feature keys differ for occluded versus non-occluded variants.


## Recommended First Commit Sequence

1. Add retained scene occlusion flags/API with graph-only tests.
2. Add mesh occlusion prepass emission.
3. Add shader preprocessing support for feature defines/includes.
4. Add generic occlusion bind group and volume-slice consumer.
5. Update Allen example to use mesh + volume scene occlusion.
6. Migrate current volume occlusion path into generic scene occlusion.
