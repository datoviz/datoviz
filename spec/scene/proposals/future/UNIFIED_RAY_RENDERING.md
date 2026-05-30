# Unified Ray Rendering

Status: future roadmap.

This note preserves long-horizon direction for unified volume and geometry ray rendering without
turning it into a v0.4 implementation requirement.


## Target

Unified ray rendering should be a panel renderer family, not just another ordinary visual. It owns a
primary ray per output pixel and can integrate sampled fields, volumes, implicit objects, and
embedded geometry in one composition model.

The active boundary should remain:

```text
scene retained state -> FramePlan -> DRP2 command stream -> runtime
```

Scene describes spatial objects, fields, materials, cameras, panels, and requests. DRP2/runtime
decide whether those inputs lower to graphics draws, compute dispatches, hardware ray tracing, or a
hybrid.


## First Practical Path

Start with compute raymarching, not hardware ray tracing.

Reasons:

1. compute is more portable across Vulkan-class hardware and future WebGPU targets;
2. it integrates naturally with storage or sampled image targets;
3. it avoids exposing Vulkan ray-tracing extension concepts too early;
4. it is enough for scalar volume rendering, MIP, emission-absorption, clipping, probes, and basic
   embedded-surface tests.

Likely sequence:

1. raymarch one 3D scalar `DvzSampledField` through a transfer function into panel color;
2. emit depth or hit distance;
3. share the path with query/readback requests;
4. add clipping and simple lighting;
5. add one embedded opaque mesh or primitive family;
6. add retained resource reuse and multi-panel support.


## Scene Semantics To Preserve

1. sampled fields carry dimensionality, format, bounds, axis mapping, and sampler policy;
2. transfer functions map scalar or categorical values to color, opacity, or material parameters;
3. retained geometry keeps transform, material, and visibility state;
4. panel-local camera/ray state remains explicit;
5. renderer or technique descriptors attach to a panel rather than only to individual visuals;
6. raster draw order remains useful for overlays and UI, not for physical 3D transparent
   composition.


## DRP2 Pressure

Future DRP2 command families may need:

1. create/update sampled field resources, including 3D textures;
2. create/update transfer resources;
3. create/update geometry or acceleration resources;
4. bind ray-renderer inputs for one panel;
5. dispatch an integration pass into named outputs;
6. read back probe, pick, depth, scalar, id, or auxiliary outputs.

Capabilities should cover compute dispatch, storage images, 3D sampled textures, transfer textures,
depth or hit-distance output, auxiliary query buffers, and eventually ray-query or hardware
ray-tracing support.


## Query Payloads

Ray-friendly query results should be extensible and typed:

1. panel id and visual/object id;
2. pixel coordinate and normalized device coordinate;
3. ray origin and direction;
4. distance along the ray;
5. world coordinate and data coordinate;
6. scalar, vector, or category value;
7. accumulated opacity or confidence;
8. flags describing which fields are valid.


## Transfer Resources

The existing `DvzScale` and `DvzColormap` direction should harden into a reusable transfer-resource
realization:

```text
DvzScale + DvzColormap -> transfer table -> DRP2 texture/buffer -> shader/raymarch lookup
```

Rules:

1. scale/colormap updates should update small transfer resources, not full field textures;
2. several images, slices, volumes, and future ray renderers should share the same transfer
   realization;
3. categorical palettes and continuous transfer tables should use the same resource pattern;
4. CPU fallback remains useful for tests, debug output, and unsupported runtimes.


## Avoid For Now

1. Do not expose public Vulkan ray-tracing concepts.
2. Do not add acceleration-structure handles, shader binding tables, or raygen/miss/hit shader
   groups to scene APIs.
3. Do not build a volume-private renderer outside the active scene -> DRP2 -> runtime path.
4. Do not promise hardware ray tracing before compute raymarching validates the abstraction.
