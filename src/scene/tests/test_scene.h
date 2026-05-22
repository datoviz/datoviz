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

int test_axis_domain_and_ticks(TstContext* suite, const TstCase* item);

int test_panel_data_to_visual_positions(TstContext* suite, const TstCase* item);

int test_panel_visible_domain(TstContext* suite, const TstCase* item);

int test_axis_panzoom_visible_domain(TstContext* suite, const TstCase* item);

int test_axis_dynamic_segment_draw_count(TstContext* suite, const TstCase* item);

int test_scene_fly(TstSuite* suite);

int test_scene_turntable(TstSuite* suite);

int test_fly_create_default(TstContext* suite, const TstCase* item);

int test_fly_lookat_initialization(TstContext* suite, const TstCase* item);

int test_fly_pitch_clamp_and_reset(TstContext* suite, const TstCase* item);

int test_fly_free_and_plane_movement(TstContext* suite, const TstCase* item);

int test_fly_set_mode(TstContext* suite, const TstCase* item);

int test_fly_keyboard_arrows_update(TstContext* suite, const TstCase* item);

int test_fly_wasd_and_arrows_equivalent(TstContext* suite, const TstCase* item);

int test_fly_shift_changes_speed(TstContext* suite, const TstCase* item);

int test_fly_left_drag_updates_view(TstContext* suite, const TstCase* item);

int test_fly_router_keyboard_updates_key_state(TstContext* suite, const TstCase* item);

int test_fly_ctrl_and_space_use_same_vertical_speed(TstContext* suite, const TstCase* item);

int test_fly_right_drag_moves_vertical_plane(TstContext* suite, const TstCase* item);

int test_fly_pivot_preserves_eye_and_orbits(TstContext* suite, const TstCase* item);

int test_fly_pivot_marker_visual_tracks_visibility(TstContext* suite, const TstCase* item);

int test_panel_fly_getter(TstContext* suite, const TstCase* item);

int test_fly_scene_controller_binding(TstContext* suite, const TstCase* item);

int test_fly_controller_rejects_partial_dims(TstContext* suite, const TstCase* item);

int test_fly_controller_survives_panel_destroy(TstContext* suite, const TstCase* item);

int test_shared_fly_updates_once_for_two_panels(TstContext* suite, const TstCase* item);

int test_figure_fly_update_advances_panel_camera(TstContext* suite, const TstCase* item);

int test_figure_fly_update_clamps_dt(TstContext* suite, const TstCase* item);

int test_fly_state_is_panel_scoped(TstContext* suite, const TstCase* item);

int test_turntable_create_default(TstContext* suite, const TstCase* item);

int test_turntable_orbit_preserves_distance(TstContext* suite, const TstCase* item);

int test_turntable_pivot_preserves_eye(TstContext* suite, const TstCase* item);

int test_turntable_pan_moves_pivot_and_eye(TstContext* suite, const TstCase* item);

int test_panel_turntable_getter(TstContext* suite, const TstCase* item);

int test_turntable_pitch_and_distance_clamps(TstContext* suite, const TstCase* item);

int test_turntable_double_click_resets(TstContext* suite, const TstCase* item);

int test_turntable_scene_binding_uses_panel_input(TstContext* suite, const TstCase* item);

int test_scene_animation(TstSuite* suite);

int test_scene_animation_offline_timer_every_frame(TstContext* suite, const TstCase* item);

int test_scene_animation_timer_period_and_stop(TstContext* suite, const TstCase* item);

int test_scene_animation_realtime_delta_clamp(TstContext* suite, const TstCase* item);

int test_scene_animation_destroy_reuses_slot(TstContext* suite, const TstCase* item);

int test_scene_animation_active_query(TstContext* suite, const TstCase* item);

int test_scene_animation_arcball_spin(TstContext* suite, const TstCase* item);

int test_scene_frame_plan(TstSuite* suite);

int test_scene_frame_plan_emit(TstSuite* suite);

int test_scene_fields(TstSuite* suite);

int test_scene_interaction(TstSuite* suite);

int test_scene_graph(TstSuite* suite);

int test_scene_pick_probe(TstSuite* suite);

int test_scene_text_atlas(TstSuite* suite);

int test_scene_poll_pick_probe_clears_consumed_slots(TstContext* suite, const TstCase* item);

int test_scene_app(TstSuite* suite);

int test_panzoom_create_reset(TstContext* suite, const TstCase* item);

int test_panzoom_pan_shift(TstContext* suite, const TstCase* item);

int test_panzoom_zoom_wheel(TstContext* suite, const TstCase* item);

int test_panzoom_zoom_limits(TstContext* suite, const TstCase* item);

int test_panzoom_viewport_filters_pointer_events(TstContext* suite, const TstCase* item);

int test_panzoom_double_click_resets(TstContext* suite, const TstCase* item);

int test_panzoom_mvp_identity(TstContext* suite, const TstCase* item);

int test_panel_panzoom_getter(TstContext* suite, const TstCase* item);

int test_shared_panzoom_xy_visible_domains(TstContext* suite, const TstCase* item);

int test_split_panzoom_x_y_bindings(TstContext* suite, const TstCase* item);

int test_arcball_create_reset(TstContext* suite, const TstCase* item);

int test_arcball_rotate_produces_nonidentity_model(TstContext* suite, const TstCase* item);

int test_arcball_end_commits_rotation(TstContext* suite, const TstCase* item);

int test_arcball_rotate_axis_is_incremental(TstContext* suite, const TstCase* item);

int test_arcball_camera_view_preserves_drag_anchor(TstContext* suite, const TstCase* item);

int test_arcball_zoom_wheel(TstContext* suite, const TstCase* item);

int test_arcball_pan_right_drag(TstContext* suite, const TstCase* item);

int test_arcball_interaction_state(TstContext* suite, const TstCase* item);

int test_arcball_double_click_resets(TstContext* suite, const TstCase* item);

int test_arcball_scene_binding_uses_panel_input(TstContext* suite, const TstCase* item);

int test_arcball_panel_input_uses_hidpi_figure_coordinates(
    TstContext* suite, const TstCase* item);

int test_scene_capabilities_diagnostics(TstContext* suite, const TstCase* item);

int test_frame_plan_static_render(TstContext* suite, const TstCase* item);

int test_frame_plan_render_pass_roles(TstContext* suite, const TstCase* item);

int test_frame_plan_clear(TstContext* suite, const TstCase* item);

int test_frame_plan_growth_json(TstContext* suite, const TstCase* item);

int test_frame_plan_json_escapes_labels(TstContext* suite, const TstCase* item);

int test_scene_resource_keys(TstContext* suite, const TstCase* item);

int test_frame_plan_render_visual_metadata(TstContext* suite, const TstCase* item);
int test_frame_plan_render_visual_metadata_diagnostic(TstContext* suite, const TstCase* item);

int test_frame_plan_dynamic_update(TstContext* suite, const TstCase* item);

int test_frame_plan_texture_upload_json_includes_region(TstContext* suite, const TstCase* item);

int test_frame_plan_readbacks(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_static_multipass(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_dependencies_dump(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_depth_peeling_shape(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_gbuffer_shape(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_read_before_write(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_topological_order(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_ambiguous_producer(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_missing_usage(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_attachment_kind(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_attachment_extent(TstContext* suite, const TstCase* item);

int test_frame_plan_graph_validation_pass_kind(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_static_render(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_static_render_glsl(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(
    TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_rejects_small_caps(TstContext* suite, const TstCase* item);

int test_scene_camera_arcball_mvp_composition(TstContext* suite, const TstCase* item);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_frame_plan_emit_drp2_static_render_glsl_executes(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_readback_glsl_executes(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_two_frames_glsl_executes(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_depth_peeling_graph_executes(TstContext* suite, const TstCase* item);

int test_scene_drp2_offscreen_canvas_frame(TstContext* suite, const TstCase* item);

int test_scene_point_emit_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_sphere_emit_glsl_executes(TstContext* suite, const TstCase* item);
int test_scene_sphere_mode(TstContext* suite, const TstCase* item);

int test_scene_segment_emit_glsl(TstContext* suite, const TstCase* item);

int test_scene_segment_caps(TstContext* suite, const TstCase* item);

int test_scene_path_stroke_style(TstContext* suite, const TstCase* item);

int test_scene_point_like_lowering_policy(TstContext* suite, const TstCase* item);

int test_scene_point_emit_glsl_native_points(TstContext* suite, const TstCase* item);

int test_scene_point_style_emits_glsl_and_wgsl(TstContext* suite, const TstCase* item);

int test_scene_marker_api_and_emit_glsl(TstContext* suite, const TstCase* item);

int test_scene_point_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item);

int test_scene_pixel_emit_glsl_native_square_points(TstContext* suite, const TstCase* item);

int test_scene_pixel_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item);

int test_scene_primitive_triangle_list_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_primitive_line_strip_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_primitive_triangle_list_emit_wgsl(TstContext* suite, const TstCase* item);

int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstContext* suite, const TstCase* item);

int test_scene_mesh_instance_transform_emits_instanced_draw(TstContext* suite, const TstCase* item);

int test_scene_mesh_emits_depth_attachment(TstContext* suite, const TstCase* item);

int test_scene_mesh_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_indexed_primitive_emits_draw_indexed(TstContext* suite, const TstCase* item);

int test_scene_shared_index_buffer_emits_one_upload(TstContext* suite, const TstCase* item);

int test_scene_indexed_primitive_shading_updates_runtime(TstContext* suite, const TstCase* item);

int test_scene_path_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_path_line_width_emit_glsl(TstContext* suite, const TstCase* item);

int test_scene_image_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_image_emit_wgsl(TstContext* suite, const TstCase* item);

int test_scene_glyph_emit_glsl(TstContext* suite, const TstCase* item);

int test_scene_image_emit_uses_common_and_texture_sets(TstContext* suite, const TstCase* item);

int test_scene_visual_common_binding_layout_order(TstContext* suite, const TstCase* item);

int test_scene_process_pick_probe_requests(TstContext* suite, const TstCase* item);

int test_scene_point_pick_quadrants(TstContext* suite, const TstCase* item);

int test_scene_point_pick_rejects_disc_corner(TstContext* suite, const TstCase* item);

int test_scene_pixel_pick_accepts_square_corner(TstContext* suite, const TstCase* item);

int test_scene_marker_pick_accepts_bbox_corner(TstContext* suite, const TstCase* item);

int test_scene_sphere_pick_resolves_item(TstContext* suite, const TstCase* item);

int test_scene_stroke_pick_resolves_item(TstContext* suite, const TstCase* item);

int test_scene_primitive_pick_resolves_item(TstContext* suite, const TstCase* item);

int test_scene_image_pick_resolves_item(TstContext* suite, const TstCase* item);

int test_scene_pick_respects_visual_order_across_families(
    TstContext* suite, const TstCase* item);

int test_scene_volume_pick_resolves_item(TstContext* suite, const TstCase* item);

int test_scene_process_requests_preserves_caller_runtime(TstContext* suite, const TstCase* item);

int test_scene_image_probe_reuses_retained_request_executor(TstContext* suite, const TstCase* item);

int test_scene_image_probe_respects_panel_request_position(TstContext* suite, const TstCase* item);

int test_scene_image_probe_segment_rgba_hidden_visual(TstContext* suite, const TstCase* item);

int test_scene_image_probe_transparent_pixel_misses(TstContext* suite, const TstCase* item);

int test_scene_image_probe_gpu_readback_failure_misses(TstContext* suite, const TstCase* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen(TstContext* suite, const TstCase* item);

int test_app_offscreen_scheduler_sees_scene_dirty_without_request(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_frame_callback_enables_continuous_scheduler(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_pick_probe_requests_notify_hosted_callback(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_shared_scene_request_frame_subscribers(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_timer_advances_in_app_run(TstContext* suite, const TstCase* item);

int test_app_offscreen_timer_advances_in_render_once(TstContext* suite, const TstCase* item);

int test_app_offscreen_render_enabled_gate(TstContext* suite, const TstCase* item);

int test_app_window_panel_panzoom_helper(TstContext* suite, const TstCase* item);

#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
int test_app_external_surface_release_waits(TstContext* suite, const TstCase* item);
#endif

int test_app_offscreen_lit_primitive_depth_orders_overlap(TstContext* suite, const TstCase* item);

int test_app_offscreen_lit_primitive_depth_cue_darkens_far(TstContext* suite, const TstCase* item);

int test_app_offscreen_mesh_renders_nonblank(TstContext* suite, const TstCase* item);

int test_app_offscreen_rotated_mesh_depth_orders_faces(TstContext* suite, const TstCase* item);

int test_app_offscreen_camera_arcball_mesh_renders_cube(TstContext* suite, const TstCase* item);
#endif
#endif

int test_frame_plan_emit_drp2_readback(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_dynamic_uploads(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_texture_sampling(TstContext* suite, const TstCase* item);

int test_frame_plan_emit_drp2_compute_assisted(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_two_frames(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_dynamic_two_frames(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_texture_extent_changes(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_object_map_grows(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_texture_two_frames(TstContext* suite, const TstCase* item);

int test_frame_plan_emitter_runtime_compute_two_frames(TstContext* suite, const TstCase* item);



int test_scene_json(TstContext* suite, const TstCase* item);

int test_scene_json_includes_field_dirty_metadata(TstContext* suite, const TstCase* item);

int test_scene_json_includes_buffer_binding_metadata(TstContext* suite, const TstCase* item);

int test_scene_panel_full_helper(TstContext* suite, const TstCase* item);

int test_scene_rejects_cross_scene_visual(TstContext* suite, const TstCase* item);

int test_scene_z_layer_orders_emit(TstContext* suite, const TstCase* item);

int test_scene_controller_mode_fixed_emits_separate_mvp(TstContext* suite, const TstCase* item);

int test_scene_panel_one_pass_per_panel(TstContext* suite, const TstCase* item);

int test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state(
    TstContext* suite, const TstCase* item);

int test_scene_multi_panel_glsl_emits_viewport_scissor_commands(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_panel_three_visuals_all_drawn(TstContext* suite, const TstCase* item);

int test_app_offscreen_point_depth_orders_overlap(TstContext* suite, const TstCase* item);

int test_app_offscreen_wboit_mesh_order_independent_layers(TstContext* suite, const TstCase* item);

int test_app_offscreen_source_over_mesh_depth_and_blend(TstContext* suite, const TstCase* item);

int test_app_offscreen_depth_peel_mesh_two_layers(TstContext* suite, const TstCase* item);

int test_app_offscreen_depth_peel_mesh_three_layers(TstContext* suite, const TstCase* item);

int test_app_offscreen_scene_occlusion_hidden_alpha(TstContext* suite, const TstCase* item);

int test_app_offscreen_source_over_scene_occlusion_matrix(TstContext* suite, const TstCase* item);

int test_app_offscreen_point_depth_cue_darkens_far(TstContext* suite, const TstCase* item);

int test_app_offscreen_records_dvzr_frames(TstContext* suite, const TstCase* item);

int test_scene_background_color_creates_fixed_quad(TstContext* suite, const TstCase* item);

int test_scene_panel_plot_clip_rect_metadata(TstContext* suite, const TstCase* item);

int test_scene_scale_colormap_colorbar_core(TstContext* suite, const TstCase* item);

int test_scene_colorbar_auto_reserve_and_visuals(TstContext* suite, const TstCase* item);

int test_scene_colorbar_prepare_is_idempotent(TstContext* suite, const TstCase* item);

int test_scene_colorbar_auto_reserve_tracks_resize(TstContext* suite, const TstCase* item);

int test_scene_colorbar_detached_placement(TstContext* suite, const TstCase* item);

int test_scene_colorbar_updates_retained_visuals(TstContext* suite, const TstCase* item);

int test_scene_colorbar_emit_stream_contains_derived_visuals(
    TstContext* suite, const TstCase* item);

int test_scene_colorbar_invalid_domain_reports_diagnostic(
    TstContext* suite, const TstCase* item);

int test_scene_colorbar_rejects_unsupported_requests(TstContext* suite, const TstCase* item);

int test_scene_colorbar_rejects_cross_scene_scale(TstContext* suite, const TstCase* item);

int test_scene_image_visual_binds_colormap_scale(TstContext* suite, const TstCase* item);

int test_scene_visual_scale_rejects_cross_scene_scale(TstContext* suite, const TstCase* item);

int test_scene_visual_buffer_rejects_cross_scene_buffer(TstContext* suite, const TstCase* item);

int test_scene_image_scalar_texture_uses_bound_scale(TstContext* suite, const TstCase* item);

int test_scene_image_r16_float_field_uses_bound_scale(TstContext* suite, const TstCase* item);

int test_scene_image_r16_snorm_field_uses_bound_scale(TstContext* suite, const TstCase* item);

int test_scene_visual_field_rejects_cross_scene_field(TstContext* suite, const TstCase* item);

int test_scene_sampled_field_update_region(TstContext* suite, const TstCase* item);

int test_scene_sampled_field_rejects_unsupported_format(TstContext* suite, const TstCase* item);

int test_scene_image_visual_rejects_3d_field(TstContext* suite, const TstCase* item);

int test_scene_volume_visual_binds_3d_field(TstContext* suite, const TstCase* item);

int test_scene_volume_field_emit_realizes_3d_texture(TstContext* suite, const TstCase* item);

int test_scene_volume_retained_controls(TstContext* suite, const TstCase* item);

int test_scene_volume_rgba_field_no_transfer(TstContext* suite, const TstCase* item);

int test_scene_volume_visual_metadata_lowering(TstContext* suite, const TstCase* item);

int test_scene_volume_scalar_transfer_function_uploads_rgba(
    TstContext* suite, const TstCase* item);

int test_scene_sampled_field_3d_emits_runtime_texture_upload(
    TstContext* suite, const TstCase* item);

int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstContext* suite, const TstCase* item);

int test_scene_sampled_field_destroy_clears_visual_binding(TstContext* suite, const TstCase* item);

int test_scene_shared_field_update_marks_two_visuals_dirty(TstContext* suite, const TstCase* item);

int test_scene_image_field_partial_update_emits_texture_subregion(
    TstContext* suite, const TstCase* item);

int test_scene_image_field_resize_emits_texture_reallocation(TstContext* suite, const TstCase* item);

int test_scene_shared_field_mixed_full_and_partial_uploads(TstContext* suite, const TstCase* item);

int test_scene_rejects_unsupported_point_attribute(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode(TstContext* suite, const TstCase* item);

int test_scene_visual_depth_test(TstContext* suite, const TstCase* item);

int test_scene_visual_scene_occlusion_flags(TstContext* suite, const TstCase* item);

int test_scene_visual_scene_occlusion_frame_plan(TstContext* suite, const TstCase* item);

int test_scene_visual_scene_occlusion_emits_drp2(TstContext* suite, const TstCase* item);

int test_scene_volume_slice_uses_volume_occlusion(TstContext* suite, const TstCase* item);

int test_scene_volume_slice_uses_generic_scene_occlusion(TstContext* suite, const TstCase* item);

int test_scene_visual_internal_material_state(TstContext* suite, const TstCase* item);

int test_scene_visual_material_setter(TstContext* suite, const TstCase* item);

int test_scene_pixel_depth_cue_toggle_switches_pipeline(TstContext* suite, const TstCase* item);

int test_scene_visual_pass_capabilities(TstContext* suite, const TstCase* item);

int test_scene_draw_contract_resolver_matrix(TstContext* suite, const TstCase* item);

int test_scene_role_work_label_mapping_complete(TstContext* suite, const TstCase* item);

int test_scene_render_contract_validation_errors(TstContext* suite, const TstCase* item);

int test_scene_frame_plan_missing_graph_pass_fails_contract(TstContext* suite, const TstCase* item);

int test_scene_panel_graph_failure_reports_specific_diagnostic(TstContext* suite, const TstCase* item);

int test_scene_gbuffer_runtime_lowering(TstContext* suite, const TstCase* item);

int test_scene_frame_plan_node_reallocation_safe(TstContext* suite, const TstCase* item);

int test_scene_msaa_runtime_lowering(TstContext* suite, const TstCase* item);

int test_scene_msaa_runtime_capability_lowering(TstContext* suite, const TstCase* item);

int test_scene_edl_runtime_lowering(TstContext* suite, const TstCase* item);

int test_scene_edl_depth_producer_capabilities(TstContext* suite, const TstCase* item);

int test_scene_edl_ignores_ineligible_passes(TstContext* suite, const TstCase* item);

int test_scene_ssao_graph_foundation(TstContext* suite, const TstCase* item);

int test_scene_ssao_runtime_lowering(TstContext* suite, const TstCase* item);

int test_scene_ssao_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_sphere_ssao_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_ssao_ignores_ineligible_visuals(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_standard_blend(TstContext* suite, const TstCase* item);

int test_scene_blended_mesh_orders_after_volume_slice(TstContext* suite, const TstCase* item);

int test_scene_blended_mesh_occlusion_contracts(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_splits_frame_plan_passes(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_wboit_transparent_only_depth(
    TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_depth_peel_frame_plan(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_mixed_oit_rejected(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_emits_depth_peel_drp2(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_requires_wboit_capabilities(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_emits_wboit_drp2(TstContext* suite, const TstCase* item);

int test_scene_drp2_contract_checker_rejects_pipeline_drift(TstContext* suite, const TstCase* item);

int test_scene_drp2_contract_checker_rejects_raster_drift(TstContext* suite, const TstCase* item);

int test_scene_alpha_mode_toggle_refreshes_drp2_contracts(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_wboit_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_visual_alpha_mode_depth_peel_glsl_executes(TstContext* suite, const TstCase* item);

int test_scene_visual_attr_source_and_mutability_metadata(TstContext* suite, const TstCase* item);

int test_scene_visual_data_view(TstContext* suite, const TstCase* item);

int test_scene_point_typed_data_upload(TstContext* suite, const TstCase* item);

int test_scene_mesh_typed_data_upload(TstContext* suite, const TstCase* item);

int test_scene_mesh_geometry_upload(TstContext* suite, const TstCase* item);

int test_scene_additional_typed_data_uploads(TstContext* suite, const TstCase* item);

int test_scene_typed_upload_rejects_wrong_family(TstContext* suite, const TstCase* item);

int test_scene_point_external_position_buffer_emits_no_upload(TstContext* suite, const TstCase* item);

int test_scene_point_external_position_buffer_executes(TstContext* suite, const TstCase* item);

int test_scene_point_rejects_texcoords_attribute(TstContext* suite, const TstCase* item);

int test_scene_primitive_rejects_size_attribute(TstContext* suite, const TstCase* item);

int test_scene_path_rejects_size_attribute(TstContext* suite, const TstCase* item);

int test_scene_image_rejects_size_attribute(TstContext* suite, const TstCase* item);

int test_scene_emit_warns_visual_with_no_position(TstContext* suite, const TstCase* item);

int test_scene_rejects_mismatched_point_attribute_counts(TstContext* suite, const TstCase* item);

int test_scene_point_visual_resizes_existing_attributes(TstContext* suite, const TstCase* item);

int test_scene_rejects_range_update_without_full_allocation(TstContext* suite, const TstCase* item);

int test_scene_rejects_mutation_while_emitted_stream_is_live(TstContext* suite, const TstCase* item);

int test_scene_pick_request_distinct_ids_keep_independent_pending_and_results(
    TstContext* suite, const TstCase* item);

int test_scene_pick_request_same_id_rejects_late_result_after_newer_poll(
    TstContext* suite, const TstCase* item);

int test_scene_probe_request_zero_id_rejects_late_result_after_newer_poll(
    TstContext* suite, const TstCase* item);

int test_scene_process_requests_coalesces_pending_picks_before_execution(
    TstContext* suite, const TstCase* item);

int test_scene_process_requests_coalesces_pending_probes_before_execution(
    TstContext* suite, const TstCase* item);

int test_scene_pick_probe_unsupported_targets(TstContext* suite, const TstCase* item);

int test_scene_rejects_scale_binding_while_emitted_stream_is_live(TstContext* suite, const TstCase* item);

int test_scene_rejects_range_mutation_while_emitted_stream_is_live(TstContext* suite, const TstCase* item);

int test_scene_rejects_destroy_while_emitted_stream_is_live(TstContext* suite, const TstCase* item);

int test_scene_rejects_visual_destroy_while_emitted_stream_is_live(
    TstContext* suite, const TstCase* item);

int test_scene_live_stream_count_tracks_multiple_emits(TstContext* suite, const TstCase* item);

int test_scene_point_emit(TstContext* suite, const TstCase* item);

int test_scene_path_emit(TstContext* suite, const TstCase* item);

int test_scene_image_emit(TstContext* suite, const TstCase* item);

int test_scene_image_multi_item_emit(TstContext* suite, const TstCase* item);

int test_scene_empty_figure_emit_clear_only(TstContext* suite, const TstCase* item);

int test_scene_point_emit_has_vertex_layout(TstContext* suite, const TstCase* item);

int test_scene_second_emit_no_uploads_when_not_dirty(TstContext* suite, const TstCase* item);

int test_scene_pending_render_work_tracks_volume_state(TstContext* suite, const TstCase* item);

int test_scene_pending_render_work_clears_unlit_background(
    TstContext* suite, const TstCase* item);

int test_scene_hidden_visual_first_visible_later_uploads(TstContext* suite, const TstCase* item);

int test_scene_hidden_indexed_mesh_first_visible_later_uploads(TstContext* suite, const TstCase* item);

int test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item);

int test_scene_partial_update_uploads_only_range(TstContext* suite, const TstCase* item);

int test_scene_repeated_partial_updates_across_frames(TstContext* suite, const TstCase* item);

int test_scene_partial_update_merges_ranges_before_emit(TstContext* suite, const TstCase* item);

int test_scene_multiple_panels_multiple_point_visuals_emit(TstContext* suite, const TstCase* item);

int test_scene_render_pass_scope_excludes_resource_commands(TstContext* suite, const TstCase* item);

int test_scene_interaction_core(TstContext* suite, const TstCase* item);

int test_scene_selection_apply_pick_and_link_keys(TstContext* suite, const TstCase* item);

int test_scene_selection_apply_pick_updates_visual_masks(TstContext* suite, const TstCase* item);

int test_scene_pick_probe_queues_and_pinned_readout(TstContext* suite, const TstCase* item);

int test_scene_pick_request_same_id_supersedes_older_unresolved(
    TstContext* suite, const TstCase* item);

int test_scene_probe_request_zero_id_keeps_newest_unresolved(
    TstContext* suite, const TstCase* item);

int test_scene_image_probe_plan_rejects_size_overflow(TstContext* suite, const TstCase* item);

int test_scene_text_annotation_bookkeeping(TstContext* suite, const TstCase* item);

int test_scene_text_semantic_object_realization(TstContext* suite, const TstCase* item);

int test_scene_text_bitmap_visual_realization(TstContext* suite, const TstCase* item);

int test_scene_text_sdf_visual_realization(TstContext* suite, const TstCase* item);

int test_scene_text_auto_renderer_selection(TstContext* suite, const TstCase* item);

int test_scene_text_msdf_atlas_spec_scales_range(TstContext* suite, const TstCase* item);

int test_scene_text_font_atlas_expands_for_utf8(TstContext* suite, const TstCase* item);

int test_scene_text_font_atlas_missing_glyph_fallback(TstContext* suite, const TstCase* item);

int test_scene_text_msdf_shader_uses_rgb_distance(TstContext* suite, const TstCase* item);

int test_scene_text_atlas_utf8_runtime_readback(TstContext* suite, const TstCase* item);

int test_scene_text_many_labels_render_plan(TstContext* suite, const TstCase* item);

int test_scene_text_panzoom_glyph_anchor_coordinates(TstContext* suite, const TstCase* item);

int test_scene_text_attach_mode_change_regenerates_glyphs(TstContext* suite, const TstCase* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_has_nonblank_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_path_join_has_no_center_gap(TstContext* suite, const TstCase* item);

int test_app_offscreen_pixel_square_has_nonblank_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_points_edl_renders(TstContext* suite, const TstCase* item);

int test_app_offscreen_points_edl_changes_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_mesh_ssao_changes_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_sphere_ssao_darkens_contact(TstContext* suite, const TstCase* item);

int test_app_offscreen_image_has_nonblank_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_text_has_nonblank_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_sdf_text_has_nonblank_pixels(TstContext* suite, const TstCase* item);

int test_app_offscreen_image_field_partial_update_changes_region(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_shared_field_mixed_runtime_updates(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_image_retained_render_second_frame(TstContext* suite, const TstCase* item);

int test_app_offscreen_retained_render_second_frame(TstContext* suite, const TstCase* item);

int test_app_offscreen_resize_reuses_runtime_with_mesh_and_image(
    TstContext* suite, const TstCase* item);

int test_app_offscreen_pick_probe_request_steady_state(TstContext* suite, const TstCase* item);

int test_app_offscreen_two_panel_points_light_both_halves(TstContext* suite, const TstCase* item);

int test_app_offscreen_clear_color(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_slice_renders_field(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_mip_renders_bright_slice(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_composite_renders_field(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_occlusion_slice_renders(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_occlusion_region_delta(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_occlusion_perspective_camera(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_slice_scene_occlusion_dimming(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle(TstContext* suite, const TstCase* item);

int test_app_offscreen_volume_depth_occluded_by_primitive(TstContext* suite, const TstCase* item);

int test_app_capture_rejects_wrong_dimensions(TstContext* suite, const TstCase* item);

int test_app_capture_rejects_undersized_buffer(TstContext* suite, const TstCase* item);
#endif

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_scene_point_large_count_executes(TstContext* suite, const TstCase* item);
#endif

int test_scene(TstSuite* suite);
