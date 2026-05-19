# Screen-Space Volume Occlusion for Embedded Visuals

> **Execution Status**
> - **Status:** `FIRST SLICE IMPLEMENTED`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining volume-occlusion work after the first screen-space volume
>   occlusion prepass and Allen slice integration landed.


## Durable Contract

The volume-occlusion texture semantics, fragment behavior, descriptor shape, rollout order, and
validation expectations live in
[../../../spec/scene/implementation/OCCLUSION_EFFECTS.md](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md).

This file keeps implementation status, remaining commit plan, open questions, and validation.


## Goal

Let selected visuals be partially hidden by dense volume material in front of them:

```text
front volume density attenuates embedded visual fragments
embedded visual remains visible where front volume is sparse
back volume stays contextual through normal volume compositing
```

This remains a practical, interactive screen-space approximation that fits the retained scene,
FramePlan, DRP2, and runtime architecture.


## Implementation Status - 2026-05-16

The first implementation is split across focused commits:

1. `35cff902 Add retained volume occlusion state` adds the retained API and frame-plan metadata.
2. `849be6e9 Plan volume occlusion prepass` emits the prepass resource, graph pass, render node,
   and sampled reads.
3. `791010b3 Lower volume occlusion prepass at runtime` lowers the prepass to DRP2/Vulkan runtime
   resources and shaders.
4. `8c8aeb44 Enable volume occlusion for Allen slice` opts the Allen mouse brain slice into volume
   occlusion.

The Allen example now treats the full 3D volume visual as the panel volume occluder and the slice
visual as an embedded ordinary visual. The prepass writes the first screen-space volume hit depth
into an `R32_SFLOAT` attachment, and opted-in volume visuals sample that texture through the
existing volume depth binding.


## Remaining Work

1. Keep atlas mesh occlusion disabled until mesh/primitive shader families support generic
   occlusion.
2. Add mesh/primitive consumers through
   [SCENE_MESH_VOLUME_OCCLUSION_PLAN.md](SCENE_MESH_VOLUME_OCCLUSION_PLAN.md).
3. Migrate volume-specific plumbing into generic scene occlusion when
   [SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md](SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md) lands.
4. Keep unsupported visual families explicit and tested.
5. Decide whether the public API remains volume-specific or becomes generic scene occlusion.


## Allen Example Controls

Keep the example controls focused:

```text
Volume hides slice
Occlusion threshold
Occlusion fade distance
Hidden slice alpha
```


## Tests

Add or keep focused tests before relying on the live example:

1. panel state accepts a volume visual as occluder;
2. non-volume occluders are rejected;
3. visual `volume_occluded` flag is retained;
4. frame plan emits a `VOLUME_OCCLUSION` pass only when both source and target exist;
5. frame graph contains the transient occlusion texture and sampled dependency;
6. DRP2 stream contains texture creation, render pass, and target bind group;
7. unsupported visual families return a clear error or warning;
8. Allen example one-frame smoke run succeeds.


## Validation

```bash
just build
./build/testing/dvztest_scene volume
./build/examples/c/allen_mouse_brain_slice_glfw 1 --downsample=2
git diff --check
```


## Commit Plan

Use a short feature series rather than one large commit:

1. mesh/primitive follow-up;
2. generic scene occlusion producer/consumer migration;
3. Allen example control cleanup;
4. unsupported-family diagnostics.


## Open Questions

1. Should occlusion depth be measured in linear view space or normalized device depth everywhere?
2. Should the occlusion texture be panel-sized or figure-sized with panel viewport/scissor mapping?
3. Should volume alpha threshold use raw sampled alpha, post-opacity alpha, or full accumulated
   alpha?
4. How should WBOIT and source-over blended panels share the occlusion texture and pass ordering?
5. Should unsupported visual families fail hard or silently render without occlusion?
6. Should the volume-specific API remain public after the generic scene occlusion path lands?
