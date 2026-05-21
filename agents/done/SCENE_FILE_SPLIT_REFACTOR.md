# Scene File Split Refactor

Status: completed 2026-05-21.

This record closes the former `agents/soon/scene/SCENE_FILE_SPLIT_REFACTOR_PLAN.md` plan. The work
was an organization-only extraction pass for the active v0.4 scene stack. It preserved the existing
scene -> frame-plan -> DRP2 -> vklite/canvas behavior and kept test registration names stable.


## Commits

1. `471bef3d` - Split scene visual pipeline helpers.
2. `c03e1a09` - Split scene frame-plan emission helpers.
3. `4589fc20` - Split scene frame-plan runtime emission.
4. `f824f1d6` - Split scene visual and technique helpers.
5. `480d990f` - Split scene graph tests.


## Source Splits

`src/scene/visual_pipeline.c` is now a facade over:

1. `visual_desc.c`
2. `visual_pass_caps.c`
3. `visual_shader_desc.c`
4. `visual_pipeline_desc.c`
5. `visual_bind_desc.c`
6. `_visual_pipeline_internal.h`

`src/scene/scene_emit.c` now delegates to:

1. `visual_uploads.c`
2. `visual_metadata.c`
3. `panel_render_emit.c`
4. `_scene_emit_internal.h`

`src/scene/frame_plan_runtime.c` now keeps the public DRP2 emission entry point while delegating to:

1. `runtime_bind_groups.c`
2. `runtime_graph_resources.c`
3. `runtime_technique_targets.c`
4. `runtime_render_emit.c`
5. `_frame_plan_runtime_internal.h`

`src/scene/visual.c` and `src/scene/technique.c` are now small facades over:

1. `visual_attrs.c`
2. `visual_material.c`
3. `visual_styles.c`
4. `visual_families.c`
5. `_visual_internal.h`
6. `technique_state.c`
7. `technique_graph_wboit.c`
8. `technique_graph_depth_peel.c`
9. `technique_graph_gbuffer.c`
10. `technique_graph_ssao.c`
11. `_technique_internal.h`

`src/scene/tests/scene_graph.c` now keeps only the `test_scene_graph()` registration facade. The
case bodies moved to:

1. `scene_visuals.c`
2. `scene_techniques.c`
3. `scene_runtime.c`
4. `scene_interaction_graph.c`
5. `scene_graph_utils.c`
6. `scene_graph_utils.h`


## Validation

Validation was run after each extraction slice:

1. `just build`
2. `git diff --check`
3. `just test scene`

The final focused scene validation passed `366/366` tests.
