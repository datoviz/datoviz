# Agents Index

This directory contains execution guidance for automation agents.

Stable scene semantics belong in [../spec/scene](../spec/scene). Completed implementation records
belong in [done/](done/). Long-horizon backlog belongs in [later/](later/).


## Current Priority

Start with the small active set in [now/](now/):

1. [now/V0_4_NEXT_STEPS.md](now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](now/DRP2_SPEC.md)
3. [now/SCENE_PUBLIC_API_HEADER_PLAN.md](now/SCENE_PUBLIC_API_HEADER_PLAN.md)
4. [now/SCENE_CONVERTER_REFACTOR_PLAN.md](now/SCENE_CONVERTER_REFACTOR_PLAN.md)
5. [now/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md](now/SCENE_DRP2_REFACTOR_OPPORTUNITIES.md)
6. [now/IMAGE_PICKING_RECOVERY_PLAN.md](now/IMAGE_PICKING_RECOVERY_PLAN.md)
7. [now/WBOIT_MESH_INTERACTIVE_PLAN.md](now/WBOIT_MESH_INTERACTIVE_PLAN.md)
8. [now/V0_4_RELEASE_READINESS_PLAN.md](now/V0_4_RELEASE_READINESS_PLAN.md)

Near-term work that is expected soon, but should not crowd the active entry-point directory, lives
in [soon/](soon/). This includes the WebGPU, WASM, dual-depth-peeling, and screen-space volume
occlusion/effects tracks.


## Start Here

If resuming work on the branch:

1. Read [now/V0_4_NEXT_STEPS.md](now/V0_4_NEXT_STEPS.md) for the current practical task list.
2. Read [now/V0_4_RELEASE_READINESS_PLAN.md](now/V0_4_RELEASE_READINESS_PLAN.md) when working on
   post-feature-completion quality, API review, documentation, bindings, gallery, packaging,
   release candidates, or communication planning.
3. Read [../spec/scene/README.md](../spec/scene/README.md) before changing scene semantics,
   public scene API shape, frame planning, visual families, interaction, annotations, scales, or
   runtime boundaries.
4. Read [../spec/scene/api/API_SURFACE.md](../spec/scene/api/API_SURFACE.md) before changing public
   scene API shape or adding new scene object families.
5. Read [now/SCENE_PUBLIC_API_HEADER_PLAN.md](now/SCENE_PUBLIC_API_HEADER_PLAN.md) to distinguish
   implemented public scene APIs from drafted-but-not-yet-implemented APIs.
6. Read [now/DRP2_SPEC.md](now/DRP2_SPEC.md) if touching `spec/drp2/`, `src/drp2/`, or
   DRP2-emitting scene code.
7. Read [done/SCENE_DRP2_IMPLEMENTATION.md](done/SCENE_DRP2_IMPLEMENTATION.md) and
   [done/DRP2_SCENE_SAFETY.md](done/DRP2_SCENE_SAFETY.md) when touching the completed first
   scene -> DRP2 -> runtime slice.


## Directory Layout

### `now/`

Small active execution notes. These files should answer what to do next and where to read the
normative spec. They should not be the long-term home for scene semantics.

### `soon/`

Imminent execution lanes that are expected to be implemented soon, but are not the first file a
future agent should read when landing in the branch.

Runtime, graph, and backend lanes:

1. [soon/DRP2_NORMAL_TRACE_NORMALIZATION_PLAN.md](soon/DRP2_NORMAL_TRACE_NORMALIZATION_PLAN.md)
2. [soon/DRP2_WEBGPU_SUPPORT_PLAN.md](soon/DRP2_WEBGPU_SUPPORT_PLAN.md)
3. [soon/FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md](soon/FRAME_PLAN_GRAPH_TRANSPARENCY_PLAN.md)
4. [soon/DUAL_DEPTH_PEELING_PLAN.md](soon/DUAL_DEPTH_PEELING_PLAN.md)
5. [soon/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md](soon/SCREEN_SPACE_SCENE_OCCLUSION_PLAN.md)
6. [soon/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md](soon/SCREEN_SPACE_VOLUME_OCCLUSION_PLAN.md)
7. [soon/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md](soon/SCENE_SCREEN_SPACE_EFFECTS_PLAN.md)
8. [soon/SCENE_WGSL_SHADER_VARIANTS_PLAN.md](soon/SCENE_WGSL_SHADER_VARIANTS_PLAN.md)
9. [soon/SCENE_WASM_WEBGPU_PORT_PLAN.md](soon/SCENE_WASM_WEBGPU_PORT_PLAN.md)

Scene feature lanes:

1. [soon/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md](soon/SCENE_2D_AXES_IMPLEMENTATION_PLAN.md)
2. [soon/SCENE_POINT_PIXEL_MARKER_PLAN.md](soon/SCENE_POINT_PIXEL_MARKER_PLAN.md)
3. [soon/SCENE_VECTOR_VISUALS_PLAN.md](soon/SCENE_VECTOR_VISUALS_PLAN.md)
4. [soon/SCENE_TEXT_GLYPH_PLAN.md](soon/SCENE_TEXT_GLYPH_PLAN.md)
5. [soon/SCENE_VOLUME_RENDERING_PLAN.md](soon/SCENE_VOLUME_RENDERING_PLAN.md)
6. [soon/SCENE_NAPARI_IMAGE_LABELS_PLAN.md](soon/SCENE_NAPARI_IMAGE_LABELS_PLAN.md)
7. [soon/SCENE_SPHERE_VISUAL_PLAN.md](soon/SCENE_SPHERE_VISUAL_PLAN.md)
8. [soon/SCENE_SPHERE_RENDER_MODES_PLAN.md](soon/SCENE_SPHERE_RENDER_MODES_PLAN.md)
9. [soon/SCENE_MSAA_PLAN.md](soon/SCENE_MSAA_PLAN.md)
10. [soon/SCENE_SSAO_IMPLEMENTATION_PLAN.md](soon/SCENE_SSAO_IMPLEMENTATION_PLAN.md)
11. [soon/SCENE_SSAO_QUALITY_PLAN.md](soon/SCENE_SSAO_QUALITY_PLAN.md)
12. [soon/SCENE_TECHNIQUES_MATERIALS_PLAN.md](soon/SCENE_TECHNIQUES_MATERIALS_PLAN.md)
13. [soon/SCENE_VISUAL_SHADER_ABI_REFACTOR_PLAN.md](soon/SCENE_VISUAL_SHADER_ABI_REFACTOR_PLAN.md)
14. [soon/SCENE_FLY_CAMERA_PLAN.md](soon/SCENE_FLY_CAMERA_PLAN.md)
15. [soon/SCENE_TURNTABLE_CONTROLLER_PLAN.md](soon/SCENE_TURNTABLE_CONTROLLER_PLAN.md)

Analysis notes:

1. [soon/SCENE_EXAMPLES_GAP_REPORT.md](soon/SCENE_EXAMPLES_GAP_REPORT.md)

### `done/`

Completed phase records. These are useful context, but they are not current execution plans.

### `later/`

Backlog, strategic direction, or secondary cleanup tracks.

API design backlog:

1. [later/SCENE_SHARED_VISUAL_DATA_API.md](later/SCENE_SHARED_VISUAL_DATA_API.md)


## Maintenance Rules

1. Keep `agents/now/` small and practical.
2. Put imminent but not immediately active implementation plans in `agents/soon/`.
3. Move stable scene semantics to specialized files under `spec/scene/`.
4. Keep active not-yet-promoted design addenda in `spec/scene/proposals/`.
5. Move completed implementation records to `agents/done/`.
6. Move speculative or long-horizon execution ideas to `agents/later/`.
7. Keep example and gallery planning out of `agents/`; it belongs under `spec/scene/examples/`.
8. On the `v0.4` branch, prefer architecture, correctness, and maintainability over API or ABI
   compatibility with earlier work.
