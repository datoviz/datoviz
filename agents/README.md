# Agents Index

This directory contains execution guidance for automation agents.

Stable scene semantics belong in [../spec/scene](../spec/scene). Current execution entry points
belong in [now/](now/). Completed or historical records belong in [done/](done/), which is the
archive for finished agent plans. Long-horizon backlog belongs in [later/](later/).


## Current Priority

Start with the small active set in [now/](now/). These are the active entry points:

1. [now/START.md](now/START.md)
2. [now/RELEASE.md](now/RELEASE.md)
3. [now/DOCUMENTATION.md](now/DOCUMENTATION.md)
4. [now/STATUS.md](now/STATUS.md)

Subsystem-specific execution notes and stable behavior records live outside `now/`:

1. [../spec/drp2/AGENT_SPEC_PHASE.md](../spec/drp2/AGENT_SPEC_PHASE.md)
2. [../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md](../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
3. [../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)

Near-term follow-up work that is expected soon, but should not crowd the active entry-point
directory, lives in [soon/](soon/). Those files are execution notes that point back to durable
contracts in `spec/scene`, `spec/drp2`, or completed records in `agents/done`.


## Start Here

If resuming work on the branch:

1. Read [now/RELEASE.md](now/RELEASE.md) when working
   toward v0.4 feature freeze, release candidates, final release, or release validation.
2. Read [now/START.md](now/START.md) for branch orientation and guardrails.
3. Read [now/DOCUMENTATION.md](now/DOCUMENTATION.md) when working on public documentation,
   API/docs inventory, migration notes, known issues, gallery documentation, or release docs.
4. Read [done/APP_FRAME_SCHEDULING_REFACTOR.md](done/APP_FRAME_SCHEDULING_REFACTOR.md) before
   changing the app loop, frame pacing, window wait/wakeup behavior, or immediate-present CPU
   policy.
5. Read [now/STATUS.md](now/STATUS.md) to decide
   the next C implementation item and see which lanes can run in parallel.
6. Read [soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md](soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md)
   when asked for shiny demo follow-up beyond WebGPU/WASM and raw `ctypes`.
7. Read [soon/scene/SCENE_GPU_QUERY_OVERHAUL.md](soon/scene/SCENE_GPU_QUERY_OVERHAUL.md) and
   [../spec/scene/interaction/GPU_QUERY_SYSTEM.md](../spec/scene/interaction/GPU_QUERY_SYSTEM.md)
   before changing pick/probe/query execution, GPU request readback, visual-family query policy, or
   CPU fallback behavior.
8. Read [done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md](done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md)
   and
   [../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md](../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md)
   before changing sampled-field format/semantic interpretation, categorical colorizers,
   label-volume support, or sampled visual query schemas.
9. Read [done/TEST_RUNNER_MODERNIZATION.md](done/TEST_RUNNER_MODERNIZATION.md) for completed
   test-runner modernization history, and
   [later/TEST_RUNNER_SCHEDULING.md](later/TEST_RUNNER_SCHEDULING.md) before changing scheduling,
   process sharding, CI orchestration, or remaining skip/reporting behavior.
10. Read [../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md](../spec/scene/visuals/IMPLEMENTATION_DECISIONS.md)
   before extending pixel, point, marker, segment, path, image, sphere, or mesh first-slice
   behavior.
11. Read [../spec/scene/validation/IMAGE_PICKING_RECOVERY.md](../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
   before changing image/labels probe coordinates, panzoom probe mapping, or CPU fallback behavior.
12. Read [later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md](later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md)
   before changing `request_execute.c`, GPU pick/probe targets, request payload decoding, or
   visual-family request execution policy.
13. Read [../spec/scene/README.md](../spec/scene/README.md) before changing scene semantics,
   public scene API shape, frame planning, visual families, interaction, annotations, scales, or
   runtime boundaries.
14. Read [../spec/scene/api/API_SURFACE.md](../spec/scene/api/API_SURFACE.md) before changing public
   scene API shape or adding new scene object families.
15. Read [../spec/drp2/README.md](../spec/drp2/README.md) and
   [../spec/drp2/AGENT_SPEC_PHASE.md](../spec/drp2/AGENT_SPEC_PHASE.md) before touching
   `spec/drp2/`, `src/drp2/`, or
   DRP2-emitting scene code.
16. Read [done/SCENE_DRP2_IMPLEMENTATION.md](done/SCENE_DRP2_IMPLEMENTATION.md) and
   [done/DRP2_SCENE_SAFETY.md](done/DRP2_SCENE_SAFETY.md) when touching the completed first
   scene -> DRP2 -> runtime slice.
17. Read [done/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md](done/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md)
   and [done/SCENE_TURNTABLE_CONTROLLER_PLAN.md](done/SCENE_TURNTABLE_CONTROLLER_PLAN.md) before
   changing scene-owned controller binding, panel-local input routing, or camera controller
   semantics.


## Directory Layout

### `now/`

Small active execution notes:

1. `START.md`: dispatch and branch orientation only.
2. `RELEASE.md`: release sequencing, scope, quality gates, and artifacts.
3. `DOCUMENTATION.md`: public documentation deliverables, API/docs inventory, and RC documentation
   gates.
4. `STATUS.md`: feature gateboard and parallel-work lanes.

These files should answer what to do next and where to read the normative spec. They should not be
the long-term home for scene semantics, completed audit logs, or plans that describe code already
landed.

### `soon/`

Imminent execution lanes that are expected to be implemented soon, but are not the first file a
future agent should read when landing in the branch.

Runtime, graph, and backend lanes:

1. [soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md](soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md)
2. [soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md](soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md)
3. [soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md](soon/effects/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md)
4. [soon/scene/SCENE_WGSL_SHADER_VARIANTS_PLAN.md](soon/scene/SCENE_WGSL_SHADER_VARIANTS_PLAN.md)

Scene feature lanes:

1. [soon/scene/SCENE_GPU_QUERY_OVERHAUL.md](soon/scene/SCENE_GPU_QUERY_OVERHAUL.md)
2. [soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md](soon/scene/SCENE_POINT_PIXEL_MARKER_FOLLOWUP.md)
3. [soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md](soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md)
4. [soon/scene/SCENE_VECTOR_VISUALS_PLAN.md](soon/scene/SCENE_VECTOR_VISUALS_PLAN.md)
5. [soon/scene/SCENE_VOLUME_RENDERING_FOLLOWUP.md](soon/scene/SCENE_VOLUME_RENDERING_FOLLOWUP.md)
6. [soon/scene/SCENE_NAPARI_IMAGE_LABELS_PLAN.md](soon/scene/SCENE_NAPARI_IMAGE_LABELS_PLAN.md)
7. [soon/scene/SCENE_ORIENTATION_GIZMO_PLAN.md](soon/scene/SCENE_ORIENTATION_GIZMO_PLAN.md)
8. [soon/effects/SCENE_MSAA_PLAN.md](soon/effects/SCENE_MSAA_PLAN.md)
9. [soon/effects/SCENE_SSAO_QUALITY_PLAN.md](soon/effects/SCENE_SSAO_QUALITY_PLAN.md)
10. [soon/effects/SCENE_TECHNIQUES_MATERIALS_PLAN.md](soon/effects/SCENE_TECHNIQUES_MATERIALS_PLAN.md)
11. [soon/scene/SCENE_VISUAL_SHADER_ABI_FOLLOWUP.md](soon/scene/SCENE_VISUAL_SHADER_ABI_FOLLOWUP.md)

For visual-family lanes, durable contracts live under [../spec/scene/visuals](../spec/scene/visuals);
the `soon/` files above are execution follow-up notes.

Text/layout lanes:

1. [../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
2. [soon/text-layout/SCENE_TEXT_GLYPH_PLAN.md](soon/text-layout/SCENE_TEXT_GLYPH_PLAN.md)
3. [soon/text-layout/SCENE_TEXT_ATLAS_CACHE_PLAN.md](soon/text-layout/SCENE_TEXT_ATLAS_CACHE_PLAN.md)
4. [soon/text-layout/SCENE_HARFBUZZ_SHAPING_PLAN.md](soon/text-layout/SCENE_HARFBUZZ_SHAPING_PLAN.md)

Analysis notes:

1. [../spec/scene/examples/PLANNING.md](../spec/scene/examples/PLANNING.md)
2. [../spec/scene/examples/PLANNING.md](../spec/scene/examples/PLANNING.md)

### `done/`

Completed phase records and historical checkpoints. These are useful context, but they are not
current execution plans. When a plan from `now/` or `soon/` is finished, remove it from its active
queue, move or rewrite the final implementation record under `done/`, and update any index links so
future agents do not keep treating completed work as pending.

Recently retired or historical notes:

1. [done/REFACTOR_STATUS_2026-03-23.md](done/REFACTOR_STATUS_2026-03-23.md)
2. [done/RENDER_CONTRACT_RESOLVER_AUDIT.md](done/RENDER_CONTRACT_RESOLVER_AUDIT.md)
3. [done/PROTEIN_RIBBON_MESH_IMPROVEMENT.md](done/PROTEIN_RIBBON_MESH_IMPROVEMENT.md)
4. [done/SCENE_PUBLIC_API_HEADER_PLAN.md](done/SCENE_PUBLIC_API_HEADER_PLAN.md)
5. [done/SCENE_CONVERTER_REFACTOR_PLAN.md](done/SCENE_CONVERTER_REFACTOR_PLAN.md)
6. [done/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md](done/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md)
7. [done/WBOIT_MESH_INTERACTIVE_PLAN.md](done/WBOIT_MESH_INTERACTIVE_PLAN.md)
8. [done/TEST_RUNNER_MODERNIZATION.md](done/TEST_RUNNER_MODERNIZATION.md)
9. [done/SCENE_FLY_CAMERA_PLAN.md](done/SCENE_FLY_CAMERA_PLAN.md)
10. [done/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md](done/SCENE_CONTROLLER_BINDING_REFACTOR_PLAN.md)
11. [done/SCENE_TURNTABLE_CONTROLLER_PLAN.md](done/SCENE_TURNTABLE_CONTROLLER_PLAN.md)
12. [done/SCENE_FILE_SPLIT_REFACTOR.md](done/SCENE_FILE_SPLIT_REFACTOR.md)
13. [done/SCENE_VISUAL_DATA_VIEW_API.md](done/SCENE_VISUAL_DATA_VIEW_API.md)
14. [done/SCENE_COLORBAR_RENDERING_SLICE.md](done/SCENE_COLORBAR_RENDERING_SLICE.md)
15. [done/SCENE_SCALEBAR_RENDERING_SLICE.md](done/SCENE_SCALEBAR_RENDERING_SLICE.md)
16. [done/SCENE_SCALEBAR_3D_REFERENCE_SLICE.md](done/SCENE_SCALEBAR_3D_REFERENCE_SLICE.md)
17. [done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md](done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md)
18. [done/SCENE_SSAO_IMPLEMENTATION_PLAN.md](done/SCENE_SSAO_IMPLEMENTATION_PLAN.md)
19. [done/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md](done/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md)
20. [done/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md](done/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md)
21. [done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md](done/SCENE_SAMPLED_FIELD_INTERPRETATION_REFACTOR.md)

### `later/`

Backlog, strategic direction, or secondary cleanup tracks.

API design backlog:

1. [later/SCENE_SHARED_VISUAL_DATA_API.md](later/SCENE_SHARED_VISUAL_DATA_API.md)

Strategic visual backlog:

1. [later/SPLATTING_TIERED_PLAN.md](later/SPLATTING_TIERED_PLAN.md)
2. [later/SCENE_SPHERE_VISUAL_BACKLOG.md](later/SCENE_SPHERE_VISUAL_BACKLOG.md)
3. [later/DRP2_WEBGPU_ROADMAP.md](later/DRP2_WEBGPU_ROADMAP.md)
4. [later/SCENE_2D_AXES_POLISH.md](later/SCENE_2D_AXES_POLISH.md)

Tooling backlog:

1. [later/POST_V0_4_REFACTOR_ROADMAP.md](later/POST_V0_4_REFACTOR_ROADMAP.md)
2. [later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md](later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md)
3. [later/TEST_RUNNER_SCHEDULING.md](later/TEST_RUNNER_SCHEDULING.md)


## Maintenance Rules

1. Keep `agents/now/` small and practical.
2. Put imminent but not immediately active follow-up notes in `agents/soon/`.
3. Move stable scene semantics to specialized files under `spec/scene/`.
4. Keep active not-yet-promoted design addenda in `spec/scene/proposals/active/`.
5. Move completed implementation records to `agents/done/`, remove the original active plan from
   `agents/now/` or `agents/soon/`, and update affected README/index links in the same cleanup.
6. Move speculative or long-horizon execution ideas to `agents/later/`.
7. Keep example and gallery planning out of `agents/`; it belongs under `spec/scene/examples/`.
8. On the `v0.4` branch, prefer architecture, correctness, and maintainability over API or ABI
   compatibility with earlier work.


## Progress Tracking Rules

Use one active status fact per feature. The preferred shape is a compact table row with:

1. current disposition,
2. evidence link,
3. next action,
4. validation command when recent.

Avoid copying the same status narrative across `agents/now`, `agents/soon`, and `spec/scene`.
If a note grows into an implementation diary, archive it under `agents/done/` after the slice
lands. If it describes stable behavior, promote the rule into the closest specialized `spec/`
document and leave only a link from `agents/`.
