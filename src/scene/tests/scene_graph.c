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
    TST_CASE(test_scene_vector_style_and_bounds);
    TST_CASE(test_scene_path_stroke_style);
    TST_CASE(test_scene_segment_emit_glsl);
    TST_CASE(test_scene_vector_emit_glsl);
    TST_CASE(test_scene_vector_curved_emit_glsl);
    TST_CASE(test_scene_point_like_lowering_policy);
    TST_CASE(test_scene_splat_api_and_attrs);
    TST_CASE(test_scene_splat_emit_instanced_quads);
    TST_CASE(test_scene_point_emit_glsl_native_points);
    TST_CASE(test_scene_point_style_emits_glsl_and_wgsl);
    TST_CASE(test_scene_point_filled_no_stroke_uses_fill_shader);
    TST_CASE(test_scene_marker_api_and_emit_glsl);
    TST_CASE(test_scene_pixel_emit_glsl_native_square_points);
    TST_CASE(test_scene_point_emit_wgsl_instanced_quads);
    TST_CASE(test_scene_pixel_emit_wgsl_instanced_quads);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_primitive_triangle_list_glsl_executes);
    TST_CASE(test_scene_primitive_lit_glsl_uses_spirv);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_primitive_line_strip_glsl_executes);
    TST_CASE(test_scene_primitive_triangle_list_emit_wgsl);
    TST_CASE(test_scene_mesh_indexed_default_color_emits_draw_indexed);
    TST_CASE(test_scene_mesh_instance_transform_emits_instanced_draw);
    TST_CASE(test_scene_mesh_emits_depth_attachment);
    TST_CASE(test_scene_textured_mesh_emits_texture_pipeline);
    TST_CASE(test_scene_indexed_primitive_emits_draw_indexed);
    TST_CASE(test_scene_shared_index_buffer_emits_one_upload);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_mesh_glsl_executes);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_path_glsl_executes);
    TST_CASE(test_scene_path_line_width_emit_glsl);
    TST_CASE(test_scene_path_repeated_endpoint_closes_subpath);
    TST_CASE(test_scene_path_closed_star_cache_adjacency);
    TST_SCENE_GRAPH_SHARED_GPU_CASE(test_scene_image_glsl_executes);
    TST_CASE(test_scene_json);
    TST_CASE(test_scene_visual_attach_default_coord_space);
    TST_CASE(test_scene_json_includes_field_dirty_metadata);
    TST_CASE(test_scene_json_includes_buffer_binding_metadata);
    TST_CASE(test_scene_panel_full_helper);
    TST_CASE(test_scene_lifetime_local_ids);
    TST_CASE(test_scene_grid_resolve_weights_fixed_and_spans);
    TST_CASE(test_scene_grid_resolve_rejects_invalid_inputs);
    TST_CASE(test_scene_grid_panel_recomputes_before_emit);
    TST_CASE(test_scene_grid_panel_tracks_figure_resize);
    TST_CASE(test_scene_grid_destroy_detaches_panels_and_reuses_slot);
    TST_CASE(test_scene_grid_destroy_detached_panel_still_emits);
    TST_CASE(test_scene_panel_destroy_removes_grid_attachment);
    TST_CASE(test_scene_reference_grid_api_and_geometry);
    TST_CASE(test_scene_controller_mode_view_proj_strips_panel_model);
    TST_CASE(test_scene_figure_destroy_cascades_and_reuses_slot);
    TST_CASE(test_scene_z_layer_orders_emit);
    TST_CASE(test_scene_background_color_creates_fixed_quad);
    TST_CASE(test_scene_background_descriptor_gradient_and_image);
    TST_CASE(test_scene_panel_border_creates_fixed_overlay);
    TST_CASE(test_scene_panel_plot_clip_rect_metadata);
    TST_CASE(test_scene_controller_mode_fixed_emits_separate_mvp);
    TST_CASE(test_scene_visual_local_transform_bounds_and_clear);
    TST_CASE(test_scene_visual_local_transform_emits_per_visual_mvp);
    TST_CASE(test_scene_visual_data_coord_space_tracks_panel_view2d_resize);
    TST_CASE(test_scene_equal_aspect_view_and_panel_coord_spaces);
    TST_CASE(test_scene_mesh_local_transform_without_instances);
    TST_CASE(test_scene_visual_local_transform_family_audit);
    TST_CASE(test_scene_panel_one_pass_per_panel);
    TST_CASE(test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state);
    TST_CASE(test_scene_multi_panel_glsl_emits_viewport_scissor_commands);
    TST_CASE(test_scene_overlapping_depth_panels_glsl_clear_depth);
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
    TST_CASE(test_scene_visual_family_registry_coverage);
    TST_CASE(test_scene_draw_contract_resolver_matrix);
    TST_CASE(test_scene_role_work_label_mapping_complete);
    TST_CASE(test_scene_render_contract_validation_errors);
    TST_CASE(test_scene_frame_plan_missing_graph_pass_fails_contract);
    TST_CASE(test_scene_render_contract_rejects_untyped_visual_metadata);
    TST_CASE(test_scene_panel_graph_failure_reports_specific_diagnostic);
    TST_CASE(test_scene_gbuffer_runtime_lowering);
    TST_CASE(test_scene_frame_plan_node_reallocation_safe);
    TST_CASE(test_scene_msaa_runtime_lowering);
    TST_CASE(test_scene_msaa_blended_overlay_runtime_lowering);
    TST_CASE(test_scene_msaa_ssao_blended_overlay_runtime_lowering);
    TST_CASE(test_scene_msaa_runtime_capability_lowering);
    TST_CASE(test_scene_edl_runtime_lowering);
    TST_CASE(test_scene_edl_blended_overlay_runtime_lowering);
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
    TST_CASE(test_scene_visual_alpha_mode_depth_peel_blended_overlay);
    TST_CASE(test_scene_visual_alpha_mode_depth_peel_loads_prior_panel);
    TST_CASE(test_scene_visual_alpha_mode_mixed_oit_rejected);
    TST_CASE(test_scene_visual_alpha_mode_emits_depth_peel_drp2);
    TST_CASE(test_scene_visual_alpha_mode_requires_wboit_capabilities);
    TST_CASE(test_scene_visual_alpha_mode_emits_wboit_drp2);
    TST_CASE(test_scene_splat_alpha_mode_emits_wboit_drp2);
    TST_CASE(test_scene_drp2_contract_checker_rejects_pipeline_drift);
    TST_CASE(test_scene_drp2_contract_checker_rejects_raster_drift);
    TST_CASE(test_scene_alpha_mode_toggle_refreshes_drp2_contracts);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_visual_alpha_mode_wboit_glsl_executes);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_visual_alpha_mode_depth_peel_glsl_executes);
    TST_CASE(test_scene_blended_mesh_orders_after_volume_slice);
    TST_CASE(test_scene_blended_mesh_occlusion_contracts);
    TST_CASE(test_scene_visual_attr_source_and_mutability_metadata);
    TST_CASE(test_scene_visual_data_view);
    TST_CASE(test_scene_visual_scalar_color_attr_format);
    TST_CASE(test_scene_scalar_color_emits_rgba_upload);
    TST_CASE(test_scene_visual_bounds_point_and_range_update);
    TST_CASE(test_scene_visual_bounds_family_reducers);
    TST_CASE(test_scene_visual_bounds_mesh_instance_transform);
    TST_CASE(test_scene_panel_visual_bounds_and_union);
    TST_CASE(test_scene_panel_bounds_overlay_visual);
    TST_CASE(test_scene_panel_bounds_overlay_visual_panzoom_padding);
    TST_CASE(test_scene_panel_bounds_overlay_sphere_wire_padding);
    TST_CASE(test_scene_panel_bounds_overlay_emit_runtime);
    TST_CASE(test_scene_point_typed_data_upload);
    TST_CASE(test_scene_mesh_typed_data_upload);
    TST_CASE(test_scene_visual_index_data_upload);
    TST_CASE(test_scene_point_storage_position_buffer_emits_usage);
    TST_CASE(test_scene_descriptor_abi_rejects_invalid_structs);
    TST_CASE(test_scene_visual_shader_transform_future_compat);
    TST_CASE(test_scene_compute_point_position_buffer_emits_drp2);
    TST_CASE(test_scene_mesh_geometry_upload);
    TST_CASE(test_scene_polygon_composite);
    TST_CASE(test_scene_polygon_set_composite);
    TST_CASE(test_scene_graph_composite);
    TST_CASE(test_scene_additional_typed_data_uploads);
    TST_CASE(test_scene_typed_upload_rejects_wrong_family);
    TST_CASE(test_scene_point_external_position_buffer_emits_no_upload);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_point_external_position_buffer_executes);
    TST_CASE(test_scene_point_rejects_texcoords_attribute);
    TST_CASE(test_scene_primitive_rejects_size_attribute);
    TST_CASE(test_scene_path_rejects_size_attribute);
    TST_CASE(test_scene_image_rejects_size_attribute);
    TST_CASE(test_scene_emit_warns_visual_with_no_position);
    TST_CASE(test_scene_rejects_mismatched_point_attribute_counts);
    TST_CASE(test_scene_rejects_range_update_without_full_allocation);
    TST_CASE(test_scene_stream_allows_mutation_after_emit);
    TST_CASE(test_scene_stream_snapshot_freezes_upload_payloads);
    TST_CASE(test_scene_stream_survives_scene_destroy_after_emit);
    TST_CASE(test_scene_artifact_allows_mutation_after_emit);
    TST_CASE(test_scene_point_emit);
    TST_CASE(test_scene_external_unorm_target_encodes_srgb);
    TST_CASE(test_scene_path_emit);
    TST_CASE(test_scene_image_emit);
    TST_CASE(test_scene_image_multi_item_emit);
    TST_CASE(test_scene_image_pixel_anchor_emit_wgsl);
    TST_CASE(test_scene_glyph_emit_glsl);
    TST_CASE(test_scene_labels_emit_signed_glsl);
    TST_CASE(test_scene_labels_emit_unsigned_glsl);
    TST_CASE(test_scene_labels_emit_wgsl);
    TST_CASE(test_scene_image_emit_wgsl);
    TST_CASE(test_scene_image_sampling_nearest_emits_sampler_filters);
    TST_CASE(test_scene_image_linear_color_emit_wgsl);
    TST_CASE(test_scene_image_emit_uses_common_and_texture_sets);
    TST_CASE(test_scene_visual_common_binding_layout_order);
    TST_CASE(test_scene_empty_figure_emit_clear_only);
    TST_CASE(test_scene_point_emit_has_vertex_layout);
    TST_CASE(test_scene_point_visual_resizes_existing_attributes);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_indexed_primitive_material_updates_runtime);
    TST_SCENE_GRAPH_GPU_CASE(test_scene_point_large_count_executes);
    TST_CASE(test_scene_second_emit_no_uploads_when_not_dirty);
    TST_CASE(test_scene_pending_render_work_tracks_volume_state);
    TST_CASE(test_scene_pending_render_work_tracks_labels_state);
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
