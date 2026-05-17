/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_panzoom_arcball(TstSuite* suite);

int test_scene_axis(TstSuite* suite);

int test_axis_domain_and_ticks(TstSuite* suite, TstItem* item);

int test_panel_data_to_visual_positions(TstSuite* suite, TstItem* item);

int test_panel_visible_domain(TstSuite* suite, TstItem* item);

int test_axis_panzoom_visible_domain(TstSuite* suite, TstItem* item);

int test_axis_dynamic_segment_draw_count(TstSuite* suite, TstItem* item);

int test_scene_fly(TstSuite* suite);

int test_scene_turntable(TstSuite* suite);

int test_fly_create_default(TstSuite* suite, TstItem* item);

int test_fly_free_and_plane_movement(TstSuite* suite, TstItem* item);

int test_fly_set_mode(TstSuite* suite, TstItem* item);

int test_fly_keyboard_arrows_update(TstSuite* suite, TstItem* item);

int test_fly_ctrl_and_space_use_same_vertical_speed(TstSuite* suite, TstItem* item);

int test_fly_right_drag_moves_vertical_plane(TstSuite* suite, TstItem* item);

int test_fly_pivot_preserves_eye_and_orbits(TstSuite* suite, TstItem* item);

int test_panel_fly_getter(TstSuite* suite, TstItem* item);

int test_figure_fly_update_advances_panel_camera(TstSuite* suite, TstItem* item);

int test_turntable_create_default(TstSuite* suite, TstItem* item);

int test_turntable_orbit_preserves_distance(TstSuite* suite, TstItem* item);

int test_turntable_pivot_preserves_eye(TstSuite* suite, TstItem* item);

int test_turntable_pan_moves_pivot_and_eye(TstSuite* suite, TstItem* item);

int test_panel_turntable_getter(TstSuite* suite, TstItem* item);

int test_scene_animation(TstSuite* suite);

int test_scene_animation_offline_timer_every_frame(TstSuite* suite, TstItem* item);

int test_scene_animation_timer_period_and_stop(TstSuite* suite, TstItem* item);

int test_scene_animation_realtime_delta_clamp(TstSuite* suite, TstItem* item);

int test_scene_animation_destroy_reuses_slot(TstSuite* suite, TstItem* item);

int test_scene_animation_active_query(TstSuite* suite, TstItem* item);

int test_scene_animation_arcball_spin(TstSuite* suite, TstItem* item);

int test_scene_frame_plan(TstSuite* suite);

int test_scene_frame_plan_emit(TstSuite* suite);

int test_scene_fields(TstSuite* suite);

int test_scene_interaction(TstSuite* suite);

int test_scene_graph(TstSuite* suite);

int test_scene_pick_probe(TstSuite* suite);

int test_scene_poll_pick_probe_clears_consumed_slots(TstSuite* suite, TstItem* item);

int test_scene_app(TstSuite* suite);

int test_panzoom_create_reset(TstSuite* suite, TstItem* item);

int test_panzoom_pan_shift(TstSuite* suite, TstItem* item);

int test_panzoom_zoom_wheel(TstSuite* suite, TstItem* item);

int test_panzoom_zoom_limits(TstSuite* suite, TstItem* item);

int test_panzoom_viewport_filters_pointer_events(TstSuite* suite, TstItem* item);

int test_panzoom_double_click_resets(TstSuite* suite, TstItem* item);

int test_panzoom_mvp_identity(TstSuite* suite, TstItem* item);

int test_panel_panzoom_getter(TstSuite* suite, TstItem* item);

int test_arcball_create_reset(TstSuite* suite, TstItem* item);

int test_arcball_rotate_produces_nonidentity_model(TstSuite* suite, TstItem* item);

int test_arcball_end_commits_rotation(TstSuite* suite, TstItem* item);

int test_arcball_rotate_axis_is_incremental(TstSuite* suite, TstItem* item);

int test_arcball_zoom_wheel(TstSuite* suite, TstItem* item);

int test_arcball_pan_right_drag(TstSuite* suite, TstItem* item);

int test_arcball_interaction_state(TstSuite* suite, TstItem* item);

int test_arcball_double_click_resets(TstSuite* suite, TstItem* item);

int test_scene_capabilities_diagnostics(TstSuite* suite, TstItem* item);

int test_frame_plan_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_render_pass_roles(TstSuite* suite, TstItem* item);

int test_frame_plan_clear(TstSuite* suite, TstItem* item);

int test_frame_plan_growth_json(TstSuite* suite, TstItem* item);

int test_frame_plan_json_escapes_labels(TstSuite* suite, TstItem* item);

int test_scene_resource_keys(TstSuite* suite, TstItem* item);

int test_frame_plan_render_visual_metadata(TstSuite* suite, TstItem* item);
int test_frame_plan_render_visual_metadata_diagnostic(TstSuite* suite, TstItem* item);

int test_frame_plan_dynamic_update(TstSuite* suite, TstItem* item);

int test_frame_plan_texture_upload_json_includes_region(TstSuite* suite, TstItem* item);

int test_frame_plan_readbacks(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_static_multipass(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_dependencies_dump(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_depth_peeling_shape(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_gbuffer_shape(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_read_before_write(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_ambiguous_producer(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_missing_usage(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_attachment_kind(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_attachment_extent(TstSuite* suite, TstItem* item);

int test_frame_plan_graph_validation_pass_kind(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render_glsl(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_small_caps(TstSuite* suite, TstItem* item);

int test_scene_camera_arcball_mvp_composition(TstSuite* suite, TstItem* item);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_frame_plan_emit_drp2_static_render_glsl_executes(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_readback_glsl_executes(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_two_frames_glsl_executes(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_depth_peeling_graph_executes(TstSuite* suite, TstItem* item);

int test_scene_drp2_offscreen_canvas_frame(TstSuite* suite, TstItem* item);

int test_scene_point_emit_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_sphere_emit_glsl_executes(TstSuite* suite, TstItem* item);
int test_scene_sphere_mode(TstSuite* suite, TstItem* item);

int test_scene_segment_emit_glsl(TstSuite* suite, TstItem* item);

int test_scene_segment_caps(TstSuite* suite, TstItem* item);

int test_scene_point_like_lowering_policy(TstSuite* suite, TstItem* item);

int test_scene_point_emit_glsl_native_points(TstSuite* suite, TstItem* item);

int test_scene_point_style_emits_glsl_and_wgsl(TstSuite* suite, TstItem* item);

int test_scene_marker_api_and_emit_glsl(TstSuite* suite, TstItem* item);

int test_scene_point_emit_wgsl_instanced_quads(TstSuite* suite, TstItem* item);

int test_scene_pixel_emit_glsl_native_square_points(TstSuite* suite, TstItem* item);

int test_scene_pixel_emit_wgsl_instanced_quads(TstSuite* suite, TstItem* item);

int test_scene_primitive_triangle_list_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_primitive_line_strip_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_primitive_triangle_list_emit_wgsl(TstSuite* suite, TstItem* item);

int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstSuite* suite, TstItem* item);

int test_scene_mesh_instance_transform_emits_instanced_draw(TstSuite* suite, TstItem* item);

int test_scene_mesh_emits_depth_attachment(TstSuite* suite, TstItem* item);

int test_scene_mesh_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_indexed_primitive_emits_draw_indexed(TstSuite* suite, TstItem* item);

int test_scene_shared_index_buffer_emits_one_upload(TstSuite* suite, TstItem* item);

int test_scene_indexed_primitive_shading_updates_runtime(TstSuite* suite, TstItem* item);

int test_scene_path_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_path_line_width_emit_glsl(TstSuite* suite, TstItem* item);

int test_scene_image_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_image_emit_wgsl(TstSuite* suite, TstItem* item);

int test_scene_glyph_emit_glsl(TstSuite* suite, TstItem* item);

int test_scene_image_emit_uses_common_and_texture_sets(TstSuite* suite, TstItem* item);

int test_scene_visual_common_binding_layout_order(TstSuite* suite, TstItem* item);

int test_scene_process_pick_probe_requests(TstSuite* suite, TstItem* item);

int test_scene_point_pick_quadrants(TstSuite* suite, TstItem* item);

int test_scene_point_pick_rejects_disc_corner(TstSuite* suite, TstItem* item);

int test_scene_pixel_pick_accepts_square_corner(TstSuite* suite, TstItem* item);

int test_scene_marker_pick_accepts_bbox_corner(TstSuite* suite, TstItem* item);

int test_scene_process_requests_preserves_caller_runtime(TstSuite* suite, TstItem* item);

int test_scene_image_probe_reuses_retained_request_executor(TstSuite* suite, TstItem* item);

int test_scene_image_probe_respects_panel_request_position(TstSuite* suite, TstItem* item);

int test_scene_image_probe_segment_rgba_hidden_visual(TstSuite* suite, TstItem* item);

int test_scene_image_probe_transparent_pixel_misses(TstSuite* suite, TstItem* item);

int test_scene_image_probe_gpu_readback_failure_misses(TstSuite* suite, TstItem* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen(TstSuite* suite, TstItem* item);

int test_app_offscreen_timer_advances_in_app_run(TstSuite* suite, TstItem* item);

int test_app_offscreen_timer_advances_in_render_once(TstSuite* suite, TstItem* item);

int test_app_offscreen_render_enabled_gate(TstSuite* suite, TstItem* item);

#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
int test_app_external_surface_release_waits(TstSuite* suite, TstItem* item);
#endif

int test_app_offscreen_lit_primitive_depth_orders_overlap(TstSuite* suite, TstItem* item);

int test_app_offscreen_lit_primitive_depth_cue_darkens_far(TstSuite* suite, TstItem* item);

int test_app_offscreen_mesh_renders_nonblank(TstSuite* suite, TstItem* item);

int test_app_offscreen_rotated_mesh_depth_orders_faces(TstSuite* suite, TstItem* item);

int test_app_offscreen_camera_arcball_mesh_renders_cube(TstSuite* suite, TstItem* item);
#endif
#endif

int test_frame_plan_emit_drp2_readback(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_dynamic_uploads(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_texture_sampling(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_compute_assisted(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_two_frames(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_dynamic_two_frames(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_texture_extent_changes(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_object_map_grows(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_texture_two_frames(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_compute_two_frames(TstSuite* suite, TstItem* item);



int test_scene_json(TstSuite* suite, TstItem* item);

int test_scene_json_includes_field_dirty_metadata(TstSuite* suite, TstItem* item);

int test_scene_json_includes_buffer_binding_metadata(TstSuite* suite, TstItem* item);

int test_scene_rejects_cross_scene_visual(TstSuite* suite, TstItem* item);

int test_scene_z_layer_orders_emit(TstSuite* suite, TstItem* item);

int test_scene_controller_mode_fixed_emits_separate_mvp(TstSuite* suite, TstItem* item);

int test_scene_panel_one_pass_per_panel(TstSuite* suite, TstItem* item);

int test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state(
    TstSuite* suite, TstItem* item);

int test_scene_multi_panel_glsl_emits_viewport_scissor_commands(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_panel_three_visuals_all_drawn(TstSuite* suite, TstItem* item);

int test_app_offscreen_point_depth_orders_overlap(TstSuite* suite, TstItem* item);

int test_app_offscreen_wboit_mesh_order_independent_layers(TstSuite* suite, TstItem* item);

int test_app_offscreen_source_over_mesh_depth_and_blend(TstSuite* suite, TstItem* item);

int test_app_offscreen_depth_peel_mesh_two_layers(TstSuite* suite, TstItem* item);

int test_app_offscreen_scene_occlusion_hidden_alpha(TstSuite* suite, TstItem* item);

int test_app_offscreen_source_over_scene_occlusion_matrix(TstSuite* suite, TstItem* item);

int test_app_offscreen_point_depth_cue_darkens_far(TstSuite* suite, TstItem* item);

int test_app_offscreen_records_dvzr_frames(TstSuite* suite, TstItem* item);

int test_scene_background_color_creates_fixed_quad(TstSuite* suite, TstItem* item);

int test_scene_scale_colormap_colorbar_core(TstSuite* suite, TstItem* item);

int test_scene_colorbar_rejects_cross_scene_scale(TstSuite* suite, TstItem* item);

int test_scene_image_visual_binds_colormap_scale(TstSuite* suite, TstItem* item);

int test_scene_visual_scale_rejects_cross_scene_scale(TstSuite* suite, TstItem* item);

int test_scene_visual_buffer_rejects_cross_scene_buffer(TstSuite* suite, TstItem* item);

int test_scene_image_scalar_texture_uses_bound_scale(TstSuite* suite, TstItem* item);

int test_scene_image_r16_float_field_uses_bound_scale(TstSuite* suite, TstItem* item);

int test_scene_image_r16_snorm_field_uses_bound_scale(TstSuite* suite, TstItem* item);

int test_scene_visual_field_rejects_cross_scene_field(TstSuite* suite, TstItem* item);

int test_scene_sampled_field_update_region(TstSuite* suite, TstItem* item);

int test_scene_sampled_field_rejects_unsupported_format(TstSuite* suite, TstItem* item);

int test_scene_image_visual_rejects_3d_field(TstSuite* suite, TstItem* item);

int test_scene_volume_visual_binds_3d_field(TstSuite* suite, TstItem* item);

int test_scene_volume_field_emit_realizes_3d_texture(TstSuite* suite, TstItem* item);

int test_scene_volume_retained_controls(TstSuite* suite, TstItem* item);

int test_scene_volume_rgba_field_no_transfer(TstSuite* suite, TstItem* item);

int test_scene_volume_visual_metadata_lowering(TstSuite* suite, TstItem* item);

int test_scene_volume_scalar_transfer_function_uploads_rgba(
    TstSuite* suite, TstItem* item);

int test_scene_sampled_field_3d_emits_runtime_texture_upload(
    TstSuite* suite, TstItem* item);

int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstSuite* suite, TstItem* item);

int test_scene_sampled_field_destroy_clears_visual_binding(TstSuite* suite, TstItem* item);

int test_scene_shared_field_update_marks_two_visuals_dirty(TstSuite* suite, TstItem* item);

int test_scene_image_field_partial_update_emits_texture_subregion(
    TstSuite* suite, TstItem* item);

int test_scene_image_field_resize_emits_texture_reallocation(TstSuite* suite, TstItem* item);

int test_scene_shared_field_mixed_full_and_partial_uploads(TstSuite* suite, TstItem* item);

int test_scene_rejects_unsupported_point_attribute(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode(TstSuite* suite, TstItem* item);

int test_scene_visual_depth_test(TstSuite* suite, TstItem* item);

int test_scene_visual_scene_occlusion_flags(TstSuite* suite, TstItem* item);

int test_scene_visual_scene_occlusion_frame_plan(TstSuite* suite, TstItem* item);

int test_scene_visual_scene_occlusion_emits_drp2(TstSuite* suite, TstItem* item);

int test_scene_volume_slice_uses_volume_occlusion(TstSuite* suite, TstItem* item);

int test_scene_volume_slice_uses_generic_scene_occlusion(TstSuite* suite, TstItem* item);

int test_scene_visual_internal_material_state(TstSuite* suite, TstItem* item);

int test_scene_visual_material_setter(TstSuite* suite, TstItem* item);

int test_scene_pixel_depth_cue_toggle_switches_pipeline(TstSuite* suite, TstItem* item);

int test_scene_visual_pass_capabilities(TstSuite* suite, TstItem* item);

int test_scene_draw_contract_resolver_matrix(TstSuite* suite, TstItem* item);

int test_scene_role_work_label_mapping_complete(TstSuite* suite, TstItem* item);

int test_scene_render_contract_validation_errors(TstSuite* suite, TstItem* item);

int test_scene_frame_plan_missing_graph_pass_fails_contract(TstSuite* suite, TstItem* item);

int test_scene_gbuffer_runtime_lowering(TstSuite* suite, TstItem* item);

int test_scene_frame_plan_node_reallocation_safe(TstSuite* suite, TstItem* item);

int test_scene_msaa_runtime_lowering(TstSuite* suite, TstItem* item);

int test_scene_msaa_runtime_capability_lowering(TstSuite* suite, TstItem* item);

int test_scene_edl_runtime_lowering(TstSuite* suite, TstItem* item);

int test_scene_edl_depth_producer_capabilities(TstSuite* suite, TstItem* item);

int test_scene_edl_ignores_ineligible_passes(TstSuite* suite, TstItem* item);

int test_scene_ssao_graph_foundation(TstSuite* suite, TstItem* item);

int test_scene_ssao_runtime_lowering(TstSuite* suite, TstItem* item);

int test_scene_ssao_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_sphere_ssao_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_ssao_ignores_ineligible_visuals(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_standard_blend(TstSuite* suite, TstItem* item);

int test_scene_blended_mesh_orders_after_volume_slice(TstSuite* suite, TstItem* item);

int test_scene_blended_mesh_occlusion_contracts(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_splits_frame_plan_passes(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_wboit_transparent_only_depth(
    TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_depth_peel_frame_plan(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_mixed_oit_rejected(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_emits_depth_peel_drp2(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_requires_wboit_capabilities(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_emits_wboit_drp2(TstSuite* suite, TstItem* item);

int test_scene_drp2_contract_checker_rejects_pipeline_drift(TstSuite* suite, TstItem* item);

int test_scene_alpha_mode_toggle_refreshes_drp2_contracts(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_wboit_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_visual_alpha_mode_depth_peel_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_visual_attr_source_and_mutability_metadata(TstSuite* suite, TstItem* item);

int test_scene_point_external_position_buffer_emits_no_upload(TstSuite* suite, TstItem* item);

int test_scene_point_external_position_buffer_executes(TstSuite* suite, TstItem* item);

int test_scene_point_rejects_texcoords_attribute(TstSuite* suite, TstItem* item);

int test_scene_primitive_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_path_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_image_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_emit_warns_visual_with_no_position(TstSuite* suite, TstItem* item);

int test_scene_rejects_mismatched_point_attribute_counts(TstSuite* suite, TstItem* item);

int test_scene_point_visual_resizes_existing_attributes(TstSuite* suite, TstItem* item);

int test_scene_rejects_range_update_without_full_allocation(TstSuite* suite, TstItem* item);

int test_scene_rejects_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item);

int test_scene_pick_request_distinct_ids_keep_independent_pending_and_results(
    TstSuite* suite, TstItem* item);

int test_scene_pick_request_same_id_rejects_late_result_after_newer_poll(
    TstSuite* suite, TstItem* item);

int test_scene_probe_request_zero_id_rejects_late_result_after_newer_poll(
    TstSuite* suite, TstItem* item);

int test_scene_process_requests_coalesces_pending_picks_before_execution(
    TstSuite* suite, TstItem* item);

int test_scene_process_requests_coalesces_pending_probes_before_execution(
    TstSuite* suite, TstItem* item);

int test_scene_rejects_scale_binding_while_emitted_stream_is_live(TstSuite* suite, TstItem* item);

int test_scene_rejects_range_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item);

int test_scene_rejects_destroy_while_emitted_stream_is_live(TstSuite* suite, TstItem* item);

int test_scene_live_stream_count_tracks_multiple_emits(TstSuite* suite, TstItem* item);

int test_scene_point_emit(TstSuite* suite, TstItem* item);

int test_scene_path_emit(TstSuite* suite, TstItem* item);

int test_scene_image_emit(TstSuite* suite, TstItem* item);

int test_scene_image_multi_item_emit(TstSuite* suite, TstItem* item);

int test_scene_empty_figure_emit_clear_only(TstSuite* suite, TstItem* item);

int test_scene_point_emit_has_vertex_layout(TstSuite* suite, TstItem* item);

int test_scene_second_emit_no_uploads_when_not_dirty(TstSuite* suite, TstItem* item);

int test_scene_hidden_visual_first_visible_later_uploads(TstSuite* suite, TstItem* item);

int test_scene_hidden_indexed_mesh_first_visible_later_uploads(TstSuite* suite, TstItem* item);

int test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item);

int test_scene_partial_update_uploads_only_range(TstSuite* suite, TstItem* item);

int test_scene_repeated_partial_updates_across_frames(TstSuite* suite, TstItem* item);

int test_scene_partial_update_merges_ranges_before_emit(TstSuite* suite, TstItem* item);

int test_scene_multiple_panels_multiple_point_visuals_emit(TstSuite* suite, TstItem* item);

int test_scene_render_pass_scope_excludes_resource_commands(TstSuite* suite, TstItem* item);

int test_scene_interaction_core(TstSuite* suite, TstItem* item);

int test_scene_selection_apply_pick_and_link_keys(TstSuite* suite, TstItem* item);

int test_scene_pick_probe_queues_and_pinned_readout(TstSuite* suite, TstItem* item);

int test_scene_pick_request_same_id_supersedes_older_unresolved(
    TstSuite* suite, TstItem* item);

int test_scene_probe_request_zero_id_keeps_newest_unresolved(
    TstSuite* suite, TstItem* item);

int test_scene_image_probe_plan_rejects_size_overflow(TstSuite* suite, TstItem* item);

int test_scene_text_annotation_bookkeeping(TstSuite* suite, TstItem* item);

int test_scene_text_bitmap_visual_realization(TstSuite* suite, TstItem* item);

int test_scene_text_many_labels_render_plan(TstSuite* suite, TstItem* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_pixel_square_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_points_edl_renders(TstSuite* suite, TstItem* item);

int test_app_offscreen_points_edl_changes_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_mesh_ssao_changes_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_sphere_ssao_darkens_contact(TstSuite* suite, TstItem* item);

int test_app_offscreen_image_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_text_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_image_field_partial_update_changes_region(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_shared_field_mixed_runtime_updates(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_image_retained_render_second_frame(TstSuite* suite, TstItem* item);

int test_app_offscreen_retained_render_second_frame(TstSuite* suite, TstItem* item);

int test_app_offscreen_resize_reuses_runtime_with_mesh_and_image(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_pick_probe_request_steady_state(TstSuite* suite, TstItem* item);

int test_app_offscreen_two_panel_points_light_both_halves(TstSuite* suite, TstItem* item);

int test_app_offscreen_clear_color(TstSuite* suite, TstItem* item);

int test_app_offscreen_volume_slice_renders_field(TstSuite* suite, TstItem* item);

int test_app_offscreen_volume_mip_renders_bright_slice(TstSuite* suite, TstItem* item);

int test_app_offscreen_volume_composite_renders_field(TstSuite* suite, TstItem* item);

int test_app_offscreen_volume_occlusion_slice_renders(TstSuite* suite, TstItem* item);

int test_app_offscreen_volume_depth_occluded_by_primitive(TstSuite* suite, TstItem* item);

int test_app_capture_rejects_wrong_dimensions(TstSuite* suite, TstItem* item);

int test_app_capture_rejects_undersized_buffer(TstSuite* suite, TstItem* item);
#endif

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_scene_point_large_count_executes(TstSuite* suite, TstItem* item);
#endif

int test_scene(TstSuite* suite);
