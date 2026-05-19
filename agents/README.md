# Agents Index

This directory contains execution guidance for automation agents.

Stable scene semantics belong in [../spec/scene](../spec/scene). Current execution entry points
belong in [now/](now/). Completed or historical records belong in [done/](done/). Long-horizon
backlog belongs in [later/](later/).


## Current Priority

Start with the small active set in [now/](now/). These are the active entry points:

1. [now/NEXT_STEPS.md](now/NEXT_STEPS.md)
2. [now/APP_FRAME_SCHEDULING_REFACTOR.md](now/APP_FRAME_SCHEDULING_REFACTOR.md)
3. [now/IMPLEMENTATION.md](now/IMPLEMENTATION.md)
4. [now/RELEASE.md](now/RELEASE.md)

Subsystem-specific execution notes and stable behavior records live outside `now/`:

1. [../spec/drp2/AGENT_SPEC_PHASE.md](../spec/drp2/AGENT_SPEC_PHASE.md)
2. [../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md](../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
3. [../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)

Near-term follow-up work that is expected soon, but should not crowd the active entry-point
directory, lives in [soon/](soon/). Those files are execution notes that point back to durable
contracts in `spec/scene`, `spec/drp2`, or completed records in `agents/done`.


## Start Here

If resuming work on the branch:

1. Read [now/NEXT_STEPS.md](now/NEXT_STEPS.md) for the current practical task list.
2. Read [now/APP_FRAME_SCHEDULING_REFACTOR.md](now/APP_FRAME_SCHEDULING_REFACTOR.md) before
   changing the app loop, frame pacing, window wait/wakeup behavior, or immediate-present CPU
   policy.
3. Read [now/IMPLEMENTATION.md](now/IMPLEMENTATION.md) to decide
   the next C implementation item and see which lanes can run in parallel.
4. Read [now/RELEASE.md](now/RELEASE.md) when working on
   post-feature-completion quality, API review, documentation, bindings, gallery, packaging,
   release candidates, or communication planning.
5. Read [done/TEST_RUNNER_MODERNIZATION.md](done/TEST_RUNNER_MODERNIZATION.md) for completed
   test-runner modernization history, and
   [soon/tooling/TEST_RUNNER_SCHEDULING.md](soon/tooling/TEST_RUNNER_SCHEDULING.md) before changing scheduling,
   process sharding, CI orchestration, or remaining skip/reporting behavior.
6. Read [../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md](../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
   before extending pixel, point, marker, segment, path, image, sphere, or mesh first-slice
   behavior.
7. Read [../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
   before changing image probe coordinates, hidden pick-capable image behavior, panzoom probe
   mapping, or CPU fallback behavior.
8. Read [../spec/scene/README.md](../spec/scene/README.md) before changing scene semantics,
   public scene API shape, frame planning, visual families, interaction, annotations, scales, or
   runtime boundaries.
9. Read [../spec/scene/api/API_SURFACE.md](../spec/scene/api/API_SURFACE.md) before changing public
   scene API shape or adding new scene object families.
10. Read [../spec/drp2/README.md](../spec/drp2/README.md) and
   [../spec/drp2/AGENT_SPEC_PHASE.md](../spec/drp2/AGENT_SPEC_PHASE.md) before touching
   `spec/drp2/`, `src/drp2/`, or
   DRP2-emitting scene code.
11. Read [done/SCENE_DRP2_IMPLEMENTATION.md](done/SCENE_DRP2_IMPLEMENTATION.md) and
   [done/DRP2_SCENE_SAFETY.md](done/DRP2_SCENE_SAFETY.md) when touching the completed first
   scene -> DRP2 -> runtime slice.


## Directory Layout

### `now/`

Small active execution notes. These files should answer what to do next and where to read the
normative spec. They should not be the long-term home for scene semantics, completed audit logs, or
plans that describe code already landed.

### `soon/`

Imminent execution lanes that are expected to be implemented soon, but are not the first file a
future agent should read when landing in the branch.

Runtime, graph, and backend lanes:

1. [soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md](soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md)
2. [soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md](soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md)
3. [soon/effects/FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md](soon/effects/FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md)
4. [soon/effects/DUAL_DEPTH_PEELING_PLAN.md](soon/effects/DUAL_DEPTH_PEELING_PLAN.md)
5. [soon/effects/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md](soon/effects/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md)
6. [soon/effects/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md](soon/effects/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md)
7. [soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md](soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md)
8. [soon/scene/SCENE_WGSL_SHADER_VARIANTS_PLAN.md](soon/scene/SCENE_WGSL_SHADER_VARIANTS_PLAN.md)

Scene feature lanes:

1. [soon/scene/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md](soon/scene/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md)
2. [soon/scene/SCENE_POINT_PIXEL_MARKER_PLAN.md](soon/scene/SCENE_POINT_PIXEL_MARKER_PLAN.md)
3. [soon/scene/SCENE_VECTOR_VISUALS_PLAN.md](soon/scene/SCENE_VECTOR_VISUALS_PLAN.md)
4. [soon/scene/SCENE_VOLUME_RENDERING_PLAN.md](soon/scene/SCENE_VOLUME_RENDERING_PLAN.md)
5. [soon/scene/SCENE_NAPARI_IMAGE_LABELS_PLAN.md](soon/scene/SCENE_NAPARI_IMAGE_LABELS_PLAN.md)
6. [soon/scene/SCENE_SPHERE_VISUAL_PLAN.md](soon/scene/SCENE_SPHERE_VISUAL_PLAN.md)
7. [soon/scene/SCENE_SPHERE_RENDER_MODES_PLAN.md](soon/scene/SCENE_SPHERE_RENDER_MODES_PLAN.md)
8. [soon/effects/SCENE_MSAA_PLAN.md](soon/effects/SCENE_MSAA_PLAN.md)
9. [soon/effects/SCENE_SSAO_IMPLEMENTATION_PLAN.md](soon/effects/SCENE_SSAO_IMPLEMENTATION_PLAN.md)
10. [soon/effects/SCENE_SSAO_QUALITY_PLAN.md](soon/effects/SCENE_SSAO_QUALITY_PLAN.md)
11. [soon/effects/SCENE_TECHNIQUES_MATERIALS_PLAN.md](soon/effects/SCENE_TECHNIQUES_MATERIALS_PLAN.md)
12. [soon/scene/SCENE_VISUAL_SHADER_ABI_REFACTOR_PLAN.md](soon/scene/SCENE_VISUAL_SHADER_ABI_REFACTOR_PLAN.md)
13. [soon/interaction/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md](soon/interaction/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md)
14. [soon/interaction/SCENE_FLY_CAMERA_PLAN.md](soon/interaction/SCENE_FLY_CAMERA_PLAN.md)
15. [soon/interaction/SCENE_TURNTABLE_CONTROLLER_PLAN.md](soon/interaction/SCENE_TURNTABLE_CONTROLLER_PLAN.md)

For visual-family lanes, durable contracts live under [../spec/scene/visuals](../spec/scene/visuals);
the `soon/` files above are execution follow-up notes.

Text/layout lanes:

1. [../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
2. [soon/text-layout/SCENE_TEXT_GLYPH_PLAN.md](soon/text-layout/SCENE_TEXT_GLYPH_PLAN.md)
3. [soon/text-layout/SCENE_TEXT_ATLAS_CACHE_PLAN.md](soon/text-layout/SCENE_TEXT_ATLAS_CACHE_PLAN.md)
4. [soon/text-layout/SCENE_HARFBUZZ_SHAPING_PLAN.md](soon/text-layout/SCENE_HARFBUZZ_SHAPING_PLAN.md)

Analysis notes:

1. [../spec/scene/examples/EXAMPLE_GAP_REPORT.md](../spec/scene/examples/EXAMPLE_GAP_REPORT.md)
2. [../spec/scene/examples/EXAMPLE_PRIORITIZATION.md](../spec/scene/examples/EXAMPLE_PRIORITIZATION.md)

Tooling follow-ups:

1. [soon/tooling/TEST_RUNNER_SCHEDULING.md](soon/tooling/TEST_RUNNER_SCHEDULING.md)

### `done/`

Completed phase records and historical checkpoints. These are useful context, but they are not
current execution plans.

Recently retired or historical notes:

1. [done/REFACTOR_STATUS_2026-03-23.md](done/REFACTOR_STATUS_2026-03-23.md)
2. [done/RENDER_CONTRACT_RESOLVER_AUDIT.md](done/RENDER_CONTRACT_RESOLVER_AUDIT.md)
3. [done/PROTEIN_RIBBON_MESH_IMPROVEMENT.md](done/PROTEIN_RIBBON_MESH_IMPROVEMENT.md)
4. [done/SCENE_PUBLIC_API_HEADER_PLAN.md](done/SCENE_PUBLIC_API_HEADER_PLAN.md)
5. [done/SCENE_CONVERTER_REFACTOR_PLAN.md](done/SCENE_CONVERTER_REFACTOR_PLAN.md)
6. [done/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md](done/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md)
7. [done/WBOIT_MESH_INTERACTIVE_PLAN.md](done/WBOIT_MESH_INTERACTIVE_PLAN.md)
8. [done/TEST_RUNNER_MODERNIZATION.md](done/TEST_RUNNER_MODERNIZATION.md)

### `later/`

Backlog, strategic direction, or secondary cleanup tracks.

API design backlog:

1. [later/SCENE_SHARED_VISUAL_DATA_API.md](later/SCENE_SHARED_VISUAL_DATA_API.md)

Strategic visual backlog:

1. [later/SPLATTING_TIERED_PLAN.md](later/SPLATTING_TIERED_PLAN.md)


## Maintenance Rules

1. Keep `agents/now/` small and practical.
2. Put imminent but not immediately active follow-up notes in `agents/soon/`.
3. Move stable scene semantics to specialized files under `spec/scene/`.
4. Keep active not-yet-promoted design addenda in `spec/scene/proposals/active/`.
5. Move completed implementation records to `agents/done/`.
6. Move speculative or long-horizon execution ideas to `agents/later/`.
7. Keep example and gallery planning out of `agents/`; it belongs under `spec/scene/examples/`.
8. On the `v0.4` branch, prefer architecture, correctness, and maintainability over API or ABI
   compatibility with earlier work.
