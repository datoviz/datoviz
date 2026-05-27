# Scene Volume Rendering Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining volume visual and napari-style 3D clipping work after the
>   retained v0.4 baseline landed.


## Current State

The durable volume contract lives in
[`../../../spec/scene/visuals/VOLUME.md`](../../../spec/scene/visuals/VOLUME.md). That spec owns the
stable semantics for `DvzSampledField` binding, volume modes, bounds/crop/axis mapping, transfer
state, sampler state, clipping, and slice probe/readout payloads.

The active implementation already supports retained `volume` visuals backed by 3D sampled fields,
scene -> DRP2 emission, full-volume composite rendering, explicit slice rendering, opacity and
sampling controls, transfer texture generation, normalized clipping boxes, one arbitrary clipping
plane, and CPU slice probe/readout.

Use this file only for implementation sequencing, example pressure tests, and remaining feature
work. Do not duplicate stable volume visual rules here.


## Napari Clipping Example Target

The target example is
[`../../../spec/scene/examples/scenarios/v05/NAPARI_PRESSURE_TESTS.md`](../../../spec/scene/examples/scenarios/v05/NAPARI_PRESSURE_TESTS.md).
It should demonstrate:

1. a small real 3D microscopy or medical-style volume;
2. live 3D camera navigation;
3. composite or MIP rendering;
4. an arbitrary clipping or slice plane;
5. optional bounds, points, or image/label overlays.

Keep the example on the active path:

```text
SampledField -> scene frame plan -> DRP2 command stream -> vklite/canvas runtime
```

Do not create a parallel Vulkan renderer, presentation loop, or volume-private data model.


## Remaining Volume Work

Recommended follow-up commits:

1. Harden MIP validation and example coverage for the napari clipping demo. Keep mode semantics in
   `VOLUME.md`.
2. Add DVR/MIP picking only after the shared picking payload contract can report UVW, object
   coordinate, sampled value, visual id, and ray hit depth consistently.
3. Expand example controls for opacity, transfer range, sample count, sampler mode, render mode,
   clipping box, and clipping plane.
4. Add screenshot or smoke coverage for the napari volume clipping example once assets and runtime
   expectations are stable.
5. Keep full MPR, isosurfaces, gradient-lighted surfaces, bricking, out-of-core streaming, and
   WebGPU/WGSL parity as separate follow-up lanes unless a specific task activates one of them.
   Categorical signed/unsigned label volumes are covered by
   [`../../done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md`](../../done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md)
   and
   [`../../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md`](../../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md).


## v0.3 Reference

The v0.3 tree contains useful reference code:

1. `v0.3/src/scene/visuals/volume.c`
2. `v0.3/include/datoviz/scene/visuals/volume.h`
3. `v0.3/src/scene/glsl/graphics_volume.vert`
4. `v0.3/src/scene/glsl/graphics_volume.frag`
5. `v0.3/include/datoviz/scene/glsl/utils_volume.glsl`
6. `v0.3/src/scene/visuals/slice.c`
7. `v0.3/src/scene/glsl/graphics_slice.frag`
8. `v0.3/src/scene/glsl/graphics_volume_slice.frag`
9. `v0.3/examples/visuals/volume.py`
10. `v0.3/tests/scene/visuals/test_volume.c`

Useful ideas to retain:

1. cube-proxy rendering around volume bounds;
2. shader ray entry/exit against the volume box;
3. 3D sampler use;
4. separate semantic bounds and texture coordinates;
5. axis permutation and axis flip support;
6. arcball camera pressure tests.

Avoid reviving v0.3 resource ownership, direct public `DvzTexture*` binding, fixed step constants,
or shader-specialization constants as the public mode API.


## Validation

For remaining volume work:

```text
just build
just test scene
just test drp2
git diff --check
```

For example work, also run the narrow example target and a short offscreen or GLFW smoke on a
graphics-capable machine.
