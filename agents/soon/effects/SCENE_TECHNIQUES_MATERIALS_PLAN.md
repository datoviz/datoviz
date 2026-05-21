# Scene Techniques And Materials Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining effects/materials pickup work after graph-backed technique,
>   transparency/MSAA, and occlusion implementation contracts were split into `spec/scene`.


## Current State

Durable implementation contracts live in:

1. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)
2. [`../../../spec/scene/implementation/TRANSPARENCY_MSAA.md`](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md)
3. [`../../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md)
4. [`../../../spec/scene/semantics/EFFECTS.md`](../../../spec/scene/semantics/EFFECTS.md)
5. [`../../../spec/scene/proposals/active/MATERIAL_LIGHTING_API.md`](../../../spec/scene/proposals/active/MATERIAL_LIGHTING_API.md)

Use this file only for execution sequencing and status. Do not duplicate graph resource/pass,
transparency, MSAA, SSAO, or screen-space effect contracts here.

The active stack already has:

1. internal graph-backed technique planning;
2. internal material state and shared material shader helpers;
3. visual pass capability metadata;
4. G-buffer graph resources and opt-in DRP2/vklite lowering;
5. panel-local EDL and SSAO techniques;
6. sphere impostor G-buffer and SSAO participation;
7. panel-local MSAA lowering with explicit resolve metadata.

All new work should stay on the retained scene -> FramePlan graph -> DRP2 -> vklite/canvas route.


## Remaining Techniques And Materials Work

Recommended follow-up commits:

1. Keep technique-builder cleanup focused on removing remaining graph setup clutter from
   `scene_emit.c` without changing graph names, pass order, or stream output.
2. Extend visual pass capability tests as each family joins G-buffer, EDL, SSAO, outline, or
   screen-space effect paths.
3. Decide whether EDL becomes a generic graph post-process that can compose after selected
   transparent, volume, or SSAO branches.
4. Add object-id or mask resources only when outline/selection semantics require them; keep the
   stable identity contract aligned with `spec/scene/interaction/SELECTION.md`.
5. Continue material polish through explicit retained material fields and capability resolution,
   not one-off visual-family checks.
6. Keep scalar material modulation for curvature, cavity, accessibility, uncertainty, and similar
   channels deferred until retained scalar slots are represented in visual/material state.
7. Keep full PBR, light objects, shadowing, and ray-tracing-forward policies in the material and
   lighting proposal lane until the current Phong/material slice is stable.


## Related Follow-Up Notes

1. [`SCENE_SCREEN_SPACE_EFFECTS_PLAN.md`](SCENE_SCREEN_SPACE_EFFECTS_PLAN.md): outline, edge
   enhancement, and bloom pickup order.
2. [`SCENE_SSAO_IMPLEMENTATION_PLAN.md`](SCENE_SSAO_IMPLEMENTATION_PLAN.md): SSAO runtime and
   graph follow-up.
3. [`SCENE_SSAO_QUALITY_PLAN.md`](SCENE_SSAO_QUALITY_PLAN.md): SSAO quality tuning.
4. [`SCENE_MSAA_PLAN.md`](SCENE_MSAA_PLAN.md): remaining MSAA and alpha-to-coverage work.
5. [`../../done/DUAL_DEPTH_PEELING_IMPLEMENTATION.md`](../../done/DUAL_DEPTH_PEELING_IMPLEMENTATION.md):
   completed dual depth peeling implementation record.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes in this lane, use the narrowest relevant validation:

```text
just build
just test scene
just test drp2
```

Add offscreen or bounded GLFW smoke coverage when a change affects runtime graph resources,
fullscreen passes, descriptor refresh, sampled intermediates, or pipeline state.
