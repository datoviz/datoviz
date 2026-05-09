# Agents Index

This directory is organized by lifecycle state rather than by topic.


## Current Priority

Active execution surface:

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)
3. [now/MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_SHADING_DESIGN.md)
4. [now/GEOM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/GEOM_DESIGN.md)
5. [now/HIGH_PRIORITY_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/HIGH_PRIORITY_SPEC_DECISIONS.md)
6. [now/MESH_API_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_API_DESIGN.md)
7. [now/TEXT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TEXT_DESIGN.md)
8. [now/TRANSPARENCY_WBOIT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSPARENCY_WBOIT_DESIGN.md)
9. [now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
10. [now/TRANSFORM_CONTROLLER_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSFORM_CONTROLLER_DESIGN.md)
11. [now/ANNOTATION_MEASUREMENT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/ANNOTATION_MEASUREMENT_DESIGN.md)
12. [now/RESOURCE_UPDATE_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/RESOURCE_UPDATE_DESIGN.md)
13. [now/MATERIAL_LIGHTING_API.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MATERIAL_LIGHTING_API.md)
14. [now/AXES_DOMAIN_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/AXES_DOMAIN_DESIGN.md)
15. [now/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VOLUME_DESIGN.md)
16. [now/COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/COLORBAR_COLORMAP_DESIGN.md)

Current status:

1. The low-level graphics stack cleanup has moved from active plan to completed context. The useful
   phase records live under `agents/done/`.
2. `drp2` and `scene` are active default-build modules. The first scene -> DRP2 -> vklite/canvas
   vertical slice exists, with focused tests and basic C examples.
3. The active execution plan is now a phased scene roadmap: native 3D baseline first (`mesh`,
   depth, viewport UBO, arcball validation), then early browser/WebGPU feasibility work, then
   transparency, axes/text, and picking.
4. The DRP2 spec/fixture lane remains active and should stay aligned with implementation changes,
   especially around depth state, dynamic viewport/scissor, multi-pass sequencing, and backend
   parity pressure from browser experiments.
5. The active mesh design note records the intended Phase 1 shading/material contract and the
   deferred path for contour/isoline and PBR growth.
6. The active geometry design note records the intended `geom` module, `DvzGeometry` direction,
   procedural-shape scope, and triangulation split for v0.4.
7. The active cross-cutting spec note records current decisions around resource updates,
   model-space arcball, picking precision, WBOIT, text, and measurement annotations.
8. The active mesh API note records how `DvzGeometry`, scene mesh resources, mesh visuals,
   materials, picking, and partial updates should fit together.
9. The active text note records the intended font/atlas/shaping/render split, world-space text
   requirements, and equation-backend direction.
10. The active transparency note records how WBOIT should appear at the visual, frame-plan, DRP2,
    runtime, and capability levels.
11. The active picking note records per-family picking precision, scene-owned identity routing, and
    the intended request/readback contract.
12. The active transform/controller note records the model-space arcball contract, fixed-camera
    policy, and the split between world-space and screen-space placement.
13. The active annotation/measurement note records the intended scale-bar, dimension, and bounding-box
    overlay model, including unit-aware formatting and placement rules.
14. The active resource-update note records the intended retained partial-update contract for mesh,
    image, text-atlas, and grouped-resource mutations.
15. The active material/lighting note records the intended panel-light-set model, visual-owned
    materials, and the narrow first classic-lit shading contract.
16. The active axes/domain note records the intended panel-owned domain semantics, unit-sharing with
    measurement overlays, and the split between 2D axes and early 3D orientation aids.
17. The active volume note records the intended mixed volume/mesh/slice design, including semantic
    slice picking and shared scientific framing with 3D meshes.
18. The active colorbar/colormap note records the intended shared-scale model, panel-attached
    colorbars, and interactive range-control direction.


## Start Here

If resuming work on the branch:

1. Read [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. Read [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md) if touching
   `spec/drp2/`, `src/drp2/`, or DRP2-emitting scene code.
3. Read [now/MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_SHADING_DESIGN.md)
   before implementing the first `mesh` visual family or changing mesh shading/material direction.
4. Read [now/GEOM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/GEOM_DESIGN.md) before
   reviving `DvzShape`-like functionality or implementing the v0.4 geometry module.
5. Read [now/HIGH_PRIORITY_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/HIGH_PRIORITY_SPEC_DECISIONS.md)
   before locking scene resource updates, controller behavior, picking, transparency, or text
   direction.
6. Read [now/MESH_API_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_API_DESIGN.md)
   before implementing the scene-facing `mesh` resource/visual API.
7. Read [now/TEXT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TEXT_DESIGN.md) before
   implementing the text visual family, world-space labels, or equation/text-resource direction.
8. Read [now/TRANSPARENCY_WBOIT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSPARENCY_WBOIT_DESIGN.md)
   before implementing transparent visual modes or WBOIT-related frame-plan/runtime work.
9. Read [now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
   before implementing object/face/item picking paths or result-routing APIs.
10. Read [now/TRANSFORM_CONTROLLER_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSFORM_CONTROLLER_DESIGN.md)
   before changing controller semantics, placement modes, or model/camera ownership rules.
11. Read [now/ANNOTATION_MEASUREMENT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/ANNOTATION_MEASUREMENT_DESIGN.md)
   before implementing scale bars, dimension helpers, bounding-box overlays, or mixed placement
   annotations.
12. Read [now/RESOURCE_UPDATE_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/RESOURCE_UPDATE_DESIGN.md)
   before adding mesh-resource updates, texture-region writes, atlas updates, or new retained
   mutable resource types.
13. Read [now/MATERIAL_LIGHTING_API.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MATERIAL_LIGHTING_API.md)
   before implementing scene light objects, material setters, or lit-family shading selection.
14. Read [now/AXES_DOMAIN_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/AXES_DOMAIN_DESIGN.md)
   before implementing panel-owned domains, unit-aware axes, or scale-bar/axis shared formatting.
15. Read [now/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VOLUME_DESIGN.md)
   before implementing active volume work, slice-plane interaction, or mixed mesh/volume picking.
16. Read [now/COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/COLORBAR_COLORMAP_DESIGN.md)
   before implementing shared scales, panel colorbars, or interactive colormap/range editing.
17. Read [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
   for the completed first vertical slice.
18. Read [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
   before changing runtime/frame-target lifetime, borrowed canvas frames, object tables, or failure paths.
19. Read [done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)
   when touching panel transforms, per-panel UBOs, or controller input flow.
20. Read [done/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/done/VK_REFACTOR.md) and
   [done/LOW_LEVEL_CONSISTENCY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/LOW_LEVEL_CONSISTENCY.md)
   only when a task touches low-level ownership or naming contracts.
21. Treat [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
   and [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md) as backlog, not the default
   next-task list.


## Directory Layout

### `now/`

Files in this directory are actionable today and should stay short enough to drive real execution.

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)
3. [now/MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_SHADING_DESIGN.md)
4. [now/GEOM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/GEOM_DESIGN.md)
5. [now/HIGH_PRIORITY_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/HIGH_PRIORITY_SPEC_DECISIONS.md)
6. [now/MESH_API_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_API_DESIGN.md)
7. [now/TEXT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TEXT_DESIGN.md)
8. [now/TRANSPARENCY_WBOIT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSPARENCY_WBOIT_DESIGN.md)
9. [now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
10. [now/TRANSFORM_CONTROLLER_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSFORM_CONTROLLER_DESIGN.md)
11. [now/ANNOTATION_MEASUREMENT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/ANNOTATION_MEASUREMENT_DESIGN.md)
12. [now/RESOURCE_UPDATE_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/RESOURCE_UPDATE_DESIGN.md)
13. [now/MATERIAL_LIGHTING_API.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MATERIAL_LIGHTING_API.md)
14. [now/AXES_DOMAIN_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/AXES_DOMAIN_DESIGN.md)
15. [now/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VOLUME_DESIGN.md)
16. [now/COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/COLORBAR_COLORMAP_DESIGN.md)

### `done/`

Files in this directory are completed phase records. They are useful context, but they are not
current execution plans.

1. [done/EXTERNAL.md](/home/cyrille/GIT/Viz/datoviz/agents/done/EXTERNAL.md)
2. [done/BOOTSTRAP_GPU_CTX_MIGRATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/BOOTSTRAP_GPU_CTX_MIGRATION.md)
3. [done/OFFSCREEN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OFFSCREEN.md)
4. [done/OWNERSHIP.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OWNERSHIP.md)
5. [done/PRESENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PRESENTATION.md)
6. [done/PROTO_FIXTURE_MIGRATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PROTO_FIXTURE_MIGRATION.md)
7. [done/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/done/VK_REFACTOR.md)
8. [done/LOW_LEVEL_CONSISTENCY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/LOW_LEVEL_CONSISTENCY.md)
9. [done/SCENE_DRP2_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_SPEC_DECISIONS.md)
10. [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
11. [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
12. [done/RENDER_PASS_BATCHING.md](/home/cyrille/GIT/Viz/datoviz/agents/done/RENDER_PASS_BATCHING.md)
13. [done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)

### `later/`

Files in this directory are backlog, strategic direction, or secondary cleanup tracks.

1. [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
2. [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)


## Maintenance Rules

1. If a document is not actionable this week, it should not live under `now/`.
2. Completed plans should move to `done/` once the code and tests agree they are complete.
3. Long-horizon architecture and speculative work should live under `later/`.
4. Keep the number of active docs under `now/` small; one primary active plan is ideal.
5. On the `v0.4` branch, backward compatibility with earlier `v0.4` code or with `v0.3` is not a priority.
6. Prefer changes that improve architecture, correctness, and long-term maintainability, even when they
   require API or ABI breakage.
