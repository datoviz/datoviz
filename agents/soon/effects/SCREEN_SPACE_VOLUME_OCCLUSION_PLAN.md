# Screen-Space Volume Occlusion Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining volume-occlusion work after the first prepass and Allen slice
>   integration landed.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md)
2. [`../../../spec/scene/visuals/VOLUME.md`](../../../spec/scene/visuals/VOLUME.md)

Use this file only for implementation status, remaining commit order, open questions, and
validation. Do not duplicate volume occlusion texture semantics, fragment behavior, descriptor
shape, rollout order, or validation expectations here.

The first volume occlusion slice has landed:

1. retained volume occlusion state;
2. graph prepass resource, pass, render node, and sampled reads;
3. DRP2/Vulkan runtime lowering for the prepass resources and shaders;
4. Allen mouse brain slice integration.

The Allen example now treats the full 3D volume visual as the panel volume occluder and the slice
visual as an embedded consumer.


## Remaining Volume Occlusion Work

Recommended follow-up commits:

1. Keep atlas mesh occlusion disabled until mesh/primitive shader families support generic
   occlusion.
2. Add mesh or primitive consumers through
   [`SCENE_MESH_VOLUME_OCCLUSION_PLAN.md`](SCENE_MESH_VOLUME_OCCLUSION_PLAN.md).
3. Migrate volume-specific plumbing into generic scene occlusion when
   [`SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md`](SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md) lands.
4. Keep unsupported visual families explicit and tested.
5. Decide whether the public API remains volume-specific or becomes generic scene occlusion.
6. Keep Allen controls focused on `Volume hides slice`, threshold, fade distance, and hidden alpha.


## Tests To Keep Or Add

1. Panel state accepts a volume visual as occluder.
2. Non-volume occluders are rejected until generic scene occlusion supports them.
3. Visual `volume_occluded` state is retained.
4. FramePlan emits a `VOLUME_OCCLUSION` pass only when both source and target exist.
5. Frame graph contains the transient occlusion texture and sampled dependency.
6. DRP2 stream contains texture creation, render pass, and target bind group.
7. Unsupported visual families return a clear error, warning, or explicit no-op.
8. Allen example one-frame smoke succeeds.


## Open Questions

1. Should occlusion depth be measured in linear view space or normalized device depth everywhere?
2. Should the occlusion texture be panel-sized or figure-sized with panel viewport/scissor mapping?
3. Should volume alpha threshold use raw sampled alpha, post-opacity alpha, or accumulated alpha?
4. How should WBOIT and source-over panels share the occlusion texture and pass ordering?
5. Should unsupported visual families fail hard or silently render without occlusion?
6. Should the volume-specific API remain public after the generic scene occlusion path lands?


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
./build/testing/dvztest_scene volume
./build/examples/c/allen_mouse_brain_slice_glfw 1 --downsample=2
```
