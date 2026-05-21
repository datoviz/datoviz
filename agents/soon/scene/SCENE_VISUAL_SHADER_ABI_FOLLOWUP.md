# Scene Visual Shader ABI Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining shader ABI cleanup after the first common-binding and registry
>   pass landed.


## Current State

The durable implementation-facing shader ABI contract lives in
[`../../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md`](../../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md).
That file owns vertex attribute locations, bind-group set ordering, shader identity/cache-key
rules, WGSL parity lanes, descriptor checklists, and validation guidance.

The first behavior-preserving cleanup pass has landed:

1. `_scene_shader_abi.h` names the common, material, image, volume, and occlusion set/binding
   constants used by runtime emission;
2. common GLSL/WGSL includes share MVP, viewport, and transform helpers across active visual
   families where applicable;
3. image shaders use split texture/sampler bindings in GLSL and WGSL;
4. `visual_pipeline.c` centralizes visual vertex attribute descriptor writes and depth-state
   decisions;
5. `frame_plan_runtime.c` centralizes runtime pipeline bind-layout ordering;
6. `just shader-abi-check` validates the documented shader ABI.

Use this file only for remaining execution sequencing. Do not duplicate ABI tables here.


## Remaining Shader ABI Work

Recommended follow-up commits:

1. Extract material, image, volume, and sampled-pass bind-group helpers from
   `frame_plan_runtime.c` into focused scene-private helpers when the move can preserve persistent
   object keys.
2. Replace repeated table-shaped switch logic in `visual_pipeline.c` with small internal descriptors
   for common vertex layouts.
3. Keep shader and pipeline cache keys explicit whenever topology, shader format, pass role, bind
   layout, depth state, or blending changes.
4. Add unsupported WGSL diagnostics for active visual/technique combinations before they can fail
   later through a missing shader source pointer.
5. Keep `spec/scene/implementation/VISUAL_SHADER_REFACTOR.md` synchronized whenever a visual family,
   shader variant, bind layout, or cache-key rule changes.


## Validation

For docs-only edits:

```text
git diff --check
```

For shader ABI or runtime-emitter changes:

```text
just build
just test scene
just shader-abi-check
python3 tools/generate_missing_scene_wgsl.py --check
git diff --check
```

Broaden to DRP2 fixture/preflight or Vulkan validation smoke only when the patch changes runtime
resource binding, render targets, synchronization, or backend-facing shader modules.
