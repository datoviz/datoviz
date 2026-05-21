/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph test registration                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_graph_utils.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_scene_graph(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("scene-graph");

    _scene_graph_register_gpu_fixture(suite);

    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_point_emit_glsl_executes);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_sphere_emit_glsl_executes);
    TST_CASE(test_scene_sphere_mode);
    TST_CASE(test_scene_segment_caps);
    TST_CASE(test_scene_path_stroke_style);
    TST_CASE(test_scene_segment_emit_glsl);
    TST_CASE(test_scene_point_like_lowering_policy);
    TST_CASE(test_scene_point_emit_glsl_native_points);
    TST_CASE(test_scene_point_style_emits_glsl_and_wgsl);
    TST_CASE(test_scene_marker_api_and_emit_glsl);
    TST_CASE(test_scene_pixel_emit_glsl_native_square_points);
    TST_CASE(test_scene_point_emit_wgsl_instanced_quads);
    TST_CASE(test_scene_pixel_emit_wgsl_instanced_quads);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_primitive_triangle_list_glsl_executes);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_primitive_line_strip_glsl_executes);
    TST_CASE(test_scene_primitive_triangle_list_emit_wgsl);
    TST_CASE(test_scene_mesh_indexed_default_color_emits_draw_indexed);
    TST_CASE(test_scene_mesh_instance_transform_emits_instanced_draw);
    TST_CASE(test_scene_mesh_emits_depth_attachment);
    TST_CASE(test_scene_indexed_primitive_emits_draw_indexed);
    TST_CASE(test_scene_shared_index_buffer_emits_one_upload);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_mesh_glsl_executes);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_path_glsl_executes);
    TST_CASE(test_scene_path_line_width_emit_glsl);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_image_glsl_executes);
    TST_CASE(test_scene_json);
    TST_CASE(test_scene_json_includes_field_dirty_metadata);
    TST_CASE(test_scene_json_includes_buffer_binding_metadata);
    TST_CASE(test_scene_z_layer_orders_emit);
    TST_CASE(test_scene_background_color_creates_fixed_quad);
    TST_CASE(test_scene_panel_plot_clip_rect_metadata);
    TST_CASE(test_scene_controller_mode_fixed_emits_separate_mvp);
    TST_CASE(test_scene_panel_one_pass_per_panel);
    TST_CASE(test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state);
    TST_CASE(test_scene_multi_panel_glsl_emits_viewport_scissor_commands);
    TST_CASE(test_scene_rejects_cross_scene_visual);
    TST_CASE(test_scene_rejects_unsupported_point_attribute);
    TST_CASE(test_scene_visual_alpha_mode);
    TST_CASE(test_scene_visual_depth_test);
    TST_CASE(test_scene_visual_scene_occlusion_flags);
    TST_CASE(test_scene_visual_scene_occlusion_frame_plan);
    TST_CASE(test_scene_visual_scene_occlusion_emits_drp2);
    TST_CASE(test_scene_volume_slice_uses_volume_occlusion);
    TST_CASE(test_scene_volume_slice_uses_generic_scene_occlusion);
    TST_CASE(test_scene_visual_internal_material_state);
    TST_CASE(test_scene_visual_material_setter);
    TST_CASE(test_scene_pixel_depth_cue_toggle_switches_pipeline);
    TST_CASE(test_scene_visual_pass_capabilities);
    TST_CASE(test_scene_draw_contract_resolver_matrix);
    TST_CASE(test_scene_role_work_label_mapping_complete);
    TST_CASE(test_scene_render_contract_validation_errors);
    TST_CASE(test_scene_frame_plan_missing_graph_pass_fails_contract);
    TST_CASE(test_scene_panel_graph_failure_reports_specific_diagnostic);
    TST_CASE(test_scene_gbuffer_runtime_lowering);
    TST_CASE(test_scene_frame_plan_node_reallocation_safe);
    TST_CASE(test_scene_msaa_runtime_lowering);
    TST_CASE(test_scene_msaa_runtime_capability_lowering);
    TST_CASE(test_scene_edl_runtime_lowering);
    TST_CASE(test_scene_edl_depth_producer_capabilities);
    TST_CASE(test_scene_edl_ignores_ineligible_passes);
    TST_CASE(test_scene_ssao_graph_foundation);
    TST_CASE(test_scene_ssao_runtime_lowering);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_ssao_glsl_executes);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_sphere_ssao_glsl_executes);
    TST_CASE(test_scene_ssao_ignores_ineligible_visuals);
    TST_CASE(test_scene_visual_alpha_mode_standard_blend);
    TST_CASE(test_scene_visual_alpha_mode_splits_frame_plan_passes);
    TST_CASE(test_scene_visual_alpha_mode_wboit_transparent_only_depth);
    TST_CASE(test_scene_visual_alpha_mode_depth_peel_frame_plan);
    TST_CASE(test_scene_visual_alpha_mode_mixed_oit_rejected);
    TST_CASE(test_scene_visual_alpha_mode_emits_depth_peel_drp2);
    TST_CASE(test_scene_visual_alpha_mode_requires_wboit_capabilities);
    TST_CASE(test_scene_visual_alpha_mode_emits_wboit_drp2);
    TST_CASE(test_scene_drp2_contract_checker_rejects_pipeline_drift);
    TST_CASE(test_scene_drp2_contract_checker_rejects_raster_drift);
    TST_CASE(test_scene_alpha_mode_toggle_refreshes_drp2_contracts);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_visual_alpha_mode_wboit_glsl_executes);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_visual_alpha_mode_depth_peel_glsl_executes);
    TST_CASE(test_scene_blended_mesh_orders_after_volume_slice);
    TST_CASE(test_scene_blended_mesh_occlusion_contracts);
    TST_CASE(test_scene_visual_attr_source_and_mutability_metadata);
    TST_CASE(test_scene_visual_data_view);
    TST_CASE(test_scene_point_external_position_buffer_emits_no_upload);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_point_external_position_buffer_executes);
    TST_CASE(test_scene_point_rejects_texcoords_attribute);
    TST_CASE(test_scene_primitive_rejects_size_attribute);
    TST_CASE(test_scene_path_rejects_size_attribute);
    TST_CASE(test_scene_image_rejects_size_attribute);
    TST_CASE(test_scene_emit_warns_visual_with_no_position);
    TST_CASE(test_scene_rejects_mismatched_point_attribute_counts);
    TST_CASE(test_scene_rejects_range_update_without_full_allocation);
    TST_CASE(test_scene_rejects_mutation_while_emitted_stream_is_live);
    TST_CASE(test_scene_rejects_scale_binding_while_emitted_stream_is_live);
    TST_CASE(test_scene_rejects_range_mutation_while_emitted_stream_is_live);
    TST_CASE(test_scene_rejects_destroy_while_emitted_stream_is_live);
    TST_CASE(test_scene_rejects_visual_destroy_while_emitted_stream_is_live);
    TST_CASE(test_scene_live_stream_count_tracks_multiple_emits);
    TST_CASE(test_scene_point_emit);
    TST_CASE(test_scene_path_emit);
    TST_CASE(test_scene_image_emit);
    TST_CASE(test_scene_image_multi_item_emit);
    TST_CASE(test_scene_glyph_emit_glsl);
    TST_CASE(test_scene_image_emit_wgsl);
    TST_CASE(test_scene_image_emit_uses_common_and_texture_sets);
    TST_CASE(test_scene_visual_common_binding_layout_order);
    TST_CASE(test_scene_empty_figure_emit_clear_only);
    TST_CASE(test_scene_point_emit_has_vertex_layout);
    TST_CASE(test_scene_point_visual_resizes_existing_attributes);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_indexed_primitive_shading_updates_runtime);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_point_large_count_executes);
    TST_CASE(test_scene_second_emit_no_uploads_when_not_dirty);
    TST_CASE(test_scene_pending_render_work_tracks_volume_state);
    TST_CASE(test_scene_pending_render_work_clears_unlit_background);
    TST_CASE(test_scene_hidden_visual_first_visible_later_uploads);
    TST_CASE(test_scene_hidden_indexed_mesh_first_visible_later_uploads);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes);
    TST_CASE(test_scene_partial_update_uploads_only_range);
    TST_CASE(test_scene_repeated_partial_updates_across_frames);
    TST_CASE(test_scene_partial_update_merges_ranges_before_emit);
    TST_CASE(test_scene_multiple_panels_multiple_point_visuals_emit);
    TST_CASE(test_scene_render_pass_scope_excludes_resource_commands);

    return 0;
}
