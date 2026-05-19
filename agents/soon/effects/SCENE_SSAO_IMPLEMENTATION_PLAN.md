# Scene SSAO Implementation Plan

> **Execution Status**
> - **Status:** `IMPLEMENTED THROUGH SPHERE SSAO; QUALITY FOLLOW-UP OPEN`
> - **Updated on:** `2026-05-19`
> - **Purpose:** keep the remaining SSAO pickup work aligned with the active scene FramePlan graph,
>   runtime graph-resource emission, and DRP2/vklite descriptor-refresh path.


## Durable Contract

Use the shared occlusion implementation contract:
[../../../spec/scene/implementation/OCCLUSION_EFFECTS.md](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md).

The generic graph-technique rules are in
[../../../spec/scene/implementation/GRAPH_TECHNIQUES.md](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md).

This file should only track remaining SSAO execution order, validation, and immediate blockers.
Do not add a parallel renderer, presentation layer, or ad-hoc Vulkan path for scene SSAO.


## Current Baseline

The scene SSAO lane has landed the graph-backed runtime foundation:

1. eligible visuals write normal/depth information into a G-buffer pass;
2. a fullscreen SSAO pass samples graph resources;
3. a composite pass darkens the final target;
4. sphere impostors feed analytic normals and linear view-distance values;
5. optional bilateral blur is present in the active graph-backed path;
6. runtime smoke, offscreen image-difference, and sphere-impostor coverage exist.

The current shader remains a foundation, not the final quality target. The next work should follow
[SCENE_SSAO_QUALITY_PLAN.md](SCENE_SSAO_QUALITY_PLAN.md).


## Remaining DRP2 / Runtime Watch Items

Track these while changing SSAO:

1. sampler configuration for future noise textures, especially nearest/repeat support;
2. single-channel render-target format serialization in DRP2 fixtures;
3. descriptor refresh coverage for SSAO-shaped graph texture recreation;
4. explicit capability diagnostics for unsupported formats or attachment counts;
5. stable bind-group behavior when target extents change.

If a gap appears in DRP2, extend the existing command model narrowly rather than bypassing it with
vklite-only code.


## Tests

Use tests in increasing cost order:

1. graph-shape tests for SSAO resources and dependencies;
2. scene command-shape tests for SSAO render roles and graph passes;
3. DRP2 emission tests for graph-created textures, bind groups, fullscreen draws, and declared
   depth;
4. semantic runtime resize tests with stable graph texture ids;
5. offscreen/GPU smoke tests when the environment supports them;
6. disabled-versus-enabled capture comparisons.


## Validation

Use the narrowest available validation loop while working:

```sh
just build
just test scene
just test drp2
git diff --check
```

For Vulkan-path changes, also run a focused GPU/offscreen smoke when the environment supports it.


## Remaining First-Slice Guardrails

Keep these boundaries unless the active implementation explicitly revisits them:

1. no public framegraph API;
2. no private Vulkan SSAO path;
3. no point/image/volume participation until their G-buffer or occlusion policy is explicit;
4. no WGSL parity claim until committed WGSL source, registry coverage, and fixtures exist;
5. no broad GUI controls before shader semantics and quality presets stabilize.
