# Unified Ray Rendering Roadmap

> **Execution Status**
> - **Status:** `STRATEGIC BACKLOG`
> - **Updated on:** `2026-05-16`
> - **Purpose:** preserve a future path toward unified volume and geometry ray rendering without
>   distracting from the active v0.4 scene -> DRP2 -> runtime hardening work.


## Context

This note is long-horizon design guidance. It is not an implementation checklist for the current
v0.4 milestone.

The future capability under discussion is a renderer where volume and embedded geometry are
evaluated along the same camera ray:

1. sampled fields, volumes, implicit objects, and mesh geometry contribute to one per-pixel
   integration problem,
2. depth, ordering, opacity, and surface/volume interactions are resolved inside one renderer,
3. emission, absorption, and eventually scattering can be modeled physically rather than by
   layering independent raster passes.

The current Datoviz direction already has the right high-level boundary:

```text
scene retained state -> frame plan -> DRP2 command stream -> vklite/canvas runtime
```

The goal is to keep that boundary broad enough that a future renderer can dispatch ray integration
work without forcing scene semantics, panel ownership, picking, or transfer functions to be
redesigned.


## Long-Term Target

A unified ray renderer should be closer to a panel renderer than to one more ordinary visual. The
renderer needs ownership of a primary ray per output pixel, so it cannot be cleanly expressed as
"draw this visual after that visual" once volumes, translucent surfaces, and embedded geometry need
physically meaningful composition.

The target architecture should allow a panel to be rendered by one of several renderer families:

1. raster renderer for the current built-in point, primitive, mesh, path, image, and overlay paths,
2. raymarch/ray-integration renderer for fields, volumes, and embedded geometry,
3. hybrid renderer where the ray path produces color/depth/auxiliary outputs and raster overlays are
   drawn afterward.

Scene should describe spatial objects, fields, materials, cameras, panels, and requests. DRP2 and
the runtime should decide whether those inputs lower to draw commands, compute dispatches, hardware
ray tracing commands, or a hybrid of those.


## First Practical Ray Path

The first implementation should be compute raymarching, not hardware ray tracing.

Reasons:

1. compute is more portable across Vulkan-class hardware and future WebGPU targets,
2. it is easier to integrate with existing vklite/canvas storage or sampled image targets,
3. it avoids exposing Vulkan ray tracing extension concepts before the scene/DRP2 contract is ready,
4. it is enough for scalar volume rendering, MIP, emission-absorption, clipping planes, probes, and
   basic embedded surface tests.

A plausible sequence is:

1. raymarch one 3D scalar `DvzSampledField` through a transfer function into a panel color target,
2. emit an explicit depth or hit-distance output from the same pass,
3. share the same path with probe/readback requests,
4. add clipping and simple lighting,
5. add one embedded opaque mesh or analytic primitive family,
6. add retained resource reuse and multi-panel support,
7. consider more advanced scattering or hardware ray tracing only after the compute contract is
   stable.


## Scene Semantics To Preserve

The scene model should stay spatial and declarative. It should not become more raster-shaped than it
needs to be.

Useful long-term scene concepts:

1. sampled fields with dimensionality, format, bounds, axis mapping, and sampler policy,
2. transfer functions that map scalar or categorical values to color/opacity/material parameters,
3. retained geometry objects with transform, material, and visibility state,
4. panel-local camera/ray state,
5. renderer or technique descriptors attached to a panel rather than only to individual visuals,
6. output requests for color, depth, object id, visual id, world/data position, scalar value,
   accumulated opacity, and hit distance.

Raster draw order can remain useful for 2D overlays, annotations, and UI. It should not be the only
semantic tool available for 3D transparent composition.


## DRP2 And Runtime Direction

DRP2 should remain backend-agnostic, but it should not assume that every renderer command is a
graphics draw.

Future command families may include:

1. create/update sampled field resources, including 3D textures,
2. create/update transfer-function resources,
3. create/update geometry or acceleration resources,
4. bind ray-renderer inputs for one panel,
5. dispatch a ray integration pass into named output attachments,
6. read back probe, pick, depth, scalar, or auxiliary outputs.

Capability reporting should grow before the public API promises a ray renderer. Useful capabilities
include compute dispatch, storage images, 3D sampled textures, sampled 1D transfer textures, depth or
hit-distance output, auxiliary pick buffers, and eventually ray-query or hardware ray-tracing
support.

The ray renderer should produce ordinary scene outputs:

1. color for presentation,
2. depth or hit distance for later overlay composition,
3. optional id/value attachments for picking and probing,
4. optional accumulation state for progressive or stochastic rendering.

This lets Datoviz draw axes, annotations, selections, and UI after the ray pass without making those
overlays part of the physical integration problem.


## Picking And Probing

Picking and probing should be designed as ray-friendly requests now.

A future ray-rendered probe should be able to return:

1. panel id,
2. visual or object id,
3. pixel coordinate,
4. normalized device coordinate,
5. ray origin and direction,
6. distance along the ray,
7. world coordinate,
8. data coordinate,
9. scalar/vector/category value,
10. accumulated opacity or confidence,
11. flags describing which fields are valid.

The current point-pick and image-probe path should keep moving in that direction: probe results
should be extensible, spatial, and typed rather than tied to "read one RGBA pixel".


## Colormaps As Full Objects

This is the easiest useful preparation to do early.

The current v0.4 scene already has `DvzColormap` and `DvzScale` as retained objects. Image and volume
visuals can bind a `"colormap"` scale, and the current implementation can resolve colors on the CPU.
The future-oriented improvement is to treat the scale/colormap pair as an independent transfer
resource with its own identity, dirty state, runtime realization, and update path.

That does not require a large new public API. The useful near-term work is internal hardening:

1. keep `DvzColormap` as a scene-owned object with stable identity,
2. keep `DvzScale` responsible for scalar domain, visible range, formatting, and the colormap binding,
3. give the resolved transfer representation its own runtime cache/version instead of hiding it
   inside one visual's texture conversion,
4. upload a compact 1D RGBA transfer texture or palette buffer when the scale or colormap changes,
5. allow several visuals, images, slices, and future volumes to share that transfer resource,
6. make scale/colormap updates independent from field-data uploads,
7. preserve a CPU fallback for tests, JSON/debug output, and runtimes without the needed GPU path.

This split matters for ray rendering because the transfer function is not merely decoration. In a
volume renderer it controls emission, absorption, opacity, window/level behavior, and sometimes
material classification. If colormaps remain visual-private CPU-expanded image data, a later volume
renderer will need a separate transfer-function system and the two concepts will drift apart.

The public naming can stay conservative. "Colormap" is appropriate for 2D scalar images and
colorbars; "transfer function" is the broader rendering concept. A practical API model is:

```c
DvzColormap* cmap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
DvzScale* scale = dvz_scale(scene, &scale_desc);
dvz_scale_set_domain(scale, min_value, max_value);
dvz_scale_set_colormap(scale, cmap);
dvz_visual_set_scale(image_or_volume, "colormap", scale);
```

Internally, that can lower to a reusable transfer resource:

```text
DvzScale + DvzColormap -> transfer table -> DRP2 texture/buffer -> shader/raymarch lookup
```

Recommended near-term scope:

1. do not rename the public `DvzColormap`/`DvzScale` API yet,
2. do not expose a broad `DvzTransferFunction` public type until volume needs are clearer,
3. do make the runtime realization reusable and independently dirty,
4. do make categorical palettes and continuous transfer tables share the same resource pattern,
5. do document that image, slice, volume, and future ray renderers consume the same scale/colormap
   semantics.

So the answer is yes: make colormaps "full objects" now, but define that as stronger retained and
runtime-resource semantics for the existing objects, not as a big new public API family.


## Low-Regret Work To Do Early

These items make the future path easier without committing the current branch to a ray renderer:

1. Normalize `DvzSampledField` as the common source for 2D images, slices, and future 3D volumes.
2. Keep field metadata explicit: dimensionality, scalar/vector/category semantic, format, bounds,
   axis mapping, sampler policy, and dirty regions.
3. Move scalar colormapping toward shared scale/colormap transfer resources.
4. Ensure colormap/scale changes update only small transfer resources, not full field textures.
5. Make probe/pick result structs spatial and extensible, including world/data coordinates and typed
   values.
6. Add internal room for panel-level renderer or technique selection, even while the only active
   renderer remains raster.
7. Keep scene frame-plan concepts named broadly enough to include dispatch/integration passes, not
   only raster draw lists.
8. Add DRP2 capability flags for compute, storage images, 3D textures, transfer textures, and
   auxiliary output buffers as soon as a concrete consumer needs them.
9. Keep renderer outputs explicit: color, depth/hit distance, ids, values, and auxiliary request
   products should be named outputs rather than ad-hoc side effects.
10. Document that 3D transparent physical composition is renderer-defined; insertion order should
    remain an overlay/UI ordering tool, not the long-term solution for volume/surface interaction.


## What To Avoid For Now

Avoid exposing public promises around hardware ray tracing until a compute raymarch path validates
the scene and DRP2 contracts.

Do not introduce public Vulkan ray tracing concepts such as acceleration structure handles, shader
binding tables, raygen/miss/hit shader groups, or `VK_KHR_ray_tracing_pipeline`-specific options into
scene APIs. Those may eventually be backend implementation details or advanced capability-gated
escape hatches, but they are not the right first abstraction.

Also avoid implementing a volume-private renderer outside the active scene -> DRP2 -> vklite/canvas
path. That would solve the first demo quickly but would make unified volume/geometry rendering harder
later by creating a parallel resource model and presentation path.


## Relationship To Existing Notes

This document complements:

1. `agents/soon/SCENE_VOLUME_RENDERING_PLAN.md` for the near-term 3D volume implementation path,
2. `agents/soon/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md` for screen-space volume interaction ideas,
3. `agents/soon/SCENE_NAPARI_IMAGE_LABELS_PLAN.md` for image/label colormap and palette semantics,
4. `agents/later/DRP2_WEBGPU_ROADMAP.md` for long-horizon backend portability and compute pressure,
5. `spec/scene/semantics/SCALES.md` for stable scale and colormap semantics once this direction is
   promoted from backlog to specification.
