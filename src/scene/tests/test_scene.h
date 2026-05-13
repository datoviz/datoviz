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

int test_panzoom_create_reset(TstSuite* suite, TstItem* item);

int test_panzoom_pan_shift(TstSuite* suite, TstItem* item);

int test_panzoom_zoom_wheel(TstSuite* suite, TstItem* item);

int test_panzoom_double_click_resets(TstSuite* suite, TstItem* item);

int test_panzoom_mvp_identity(TstSuite* suite, TstItem* item);

int test_arcball_create_reset(TstSuite* suite, TstItem* item);

int test_arcball_rotate_produces_nonidentity_model(TstSuite* suite, TstItem* item);

int test_arcball_end_commits_rotation(TstSuite* suite, TstItem* item);

int test_arcball_double_click_resets(TstSuite* suite, TstItem* item);

int test_scene_capabilities_diagnostics(TstSuite* suite, TstItem* item);

int test_frame_plan_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_clear(TstSuite* suite, TstItem* item);

int test_frame_plan_growth_json(TstSuite* suite, TstItem* item);

int test_frame_plan_json_escapes_labels(TstSuite* suite, TstItem* item);

int test_frame_plan_dynamic_update(TstSuite* suite, TstItem* item);

int test_frame_plan_texture_upload_json_includes_region(TstSuite* suite, TstItem* item);

int test_frame_plan_readbacks(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render_glsl(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_small_caps(TstSuite* suite, TstItem* item);

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

int test_scene_drp2_offscreen_canvas_frame(TstSuite* suite, TstItem* item);

int test_scene_point_emit_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_primitive_triangle_list_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_primitive_line_strip_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstSuite* suite, TstItem* item);

int test_scene_mesh_emits_depth_attachment(TstSuite* suite, TstItem* item);

int test_scene_mesh_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_indexed_primitive_emits_draw_indexed(TstSuite* suite, TstItem* item);

int test_scene_shared_index_buffer_emits_one_upload(TstSuite* suite, TstItem* item);

int test_scene_indexed_primitive_shading_updates_runtime(TstSuite* suite, TstItem* item);

int test_scene_path_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_image_glsl_executes(TstSuite* suite, TstItem* item);

int test_scene_process_pick_probe_requests(TstSuite* suite, TstItem* item);

int test_scene_point_pick_quadrants(TstSuite* suite, TstItem* item);

int test_scene_process_requests_preserves_caller_runtime(TstSuite* suite, TstItem* item);

int test_scene_image_probe_respects_panel_request_position(TstSuite* suite, TstItem* item);

int test_scene_image_probe_transparent_pixel_misses(TstSuite* suite, TstItem* item);

int test_scene_image_probe_gpu_readback_failure_misses(TstSuite* suite, TstItem* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen(TstSuite* suite, TstItem* item);

int test_app_offscreen_lit_primitive_depth_orders_overlap(TstSuite* suite, TstItem* item);

int test_app_offscreen_mesh_renders_nonblank(TstSuite* suite, TstItem* item);

int test_app_offscreen_rotated_mesh_depth_orders_faces(TstSuite* suite, TstItem* item);
#endif
#endif

int test_frame_plan_emit_drp2_readback(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_dynamic_uploads(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_texture_sampling(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_compute_assisted(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_two_frames(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_dynamic_two_frames(TstSuite* suite, TstItem* item);

int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstSuite* suite, TstItem* item);

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

int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstSuite* suite, TstItem* item);

int test_scene_sampled_field_destroy_clears_visual_binding(TstSuite* suite, TstItem* item);

int test_scene_shared_field_update_marks_two_visuals_dirty(TstSuite* suite, TstItem* item);

int test_scene_image_field_partial_update_emits_texture_subregion(
    TstSuite* suite, TstItem* item);

int test_scene_shared_field_mixed_full_and_partial_uploads(TstSuite* suite, TstItem* item);

int test_scene_rejects_unsupported_point_attribute(TstSuite* suite, TstItem* item);

int test_scene_point_rejects_texcoords_attribute(TstSuite* suite, TstItem* item);

int test_scene_primitive_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_path_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_image_rejects_size_attribute(TstSuite* suite, TstItem* item);

int test_scene_emit_warns_visual_with_no_position(TstSuite* suite, TstItem* item);

int test_scene_rejects_mismatched_point_attribute_counts(TstSuite* suite, TstItem* item);

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

int test_scene_empty_figure_emit_clear_only(TstSuite* suite, TstItem* item);

int test_scene_point_emit_has_vertex_layout(TstSuite* suite, TstItem* item);

int test_scene_second_emit_no_uploads_when_not_dirty(TstSuite* suite, TstItem* item);

int test_scene_partial_update_uploads_only_range(TstSuite* suite, TstItem* item);

int test_scene_repeated_partial_updates_across_frames(TstSuite* suite, TstItem* item);

int test_scene_partial_update_merges_ranges_before_emit(TstSuite* suite, TstItem* item);

int test_scene_multiple_panels_multiple_point_visuals_emit(TstSuite* suite, TstItem* item);

int test_scene_interaction_core(TstSuite* suite, TstItem* item);

int test_scene_selection_apply_pick_and_link_keys(TstSuite* suite, TstItem* item);

int test_scene_pick_probe_queues_and_pinned_readout(TstSuite* suite, TstItem* item);

int test_scene_pick_request_same_id_supersedes_older_unresolved(
    TstSuite* suite, TstItem* item);

int test_scene_probe_request_zero_id_keeps_newest_unresolved(
    TstSuite* suite, TstItem* item);

int test_scene_text_annotation_bookkeeping(TstSuite* suite, TstItem* item);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_image_has_nonblank_pixels(TstSuite* suite, TstItem* item);

int test_app_offscreen_image_field_partial_update_changes_region(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_shared_field_mixed_runtime_updates(
    TstSuite* suite, TstItem* item);

int test_app_offscreen_image_retained_render_second_frame(TstSuite* suite, TstItem* item);

int test_app_offscreen_retained_render_second_frame(TstSuite* suite, TstItem* item);

int test_app_offscreen_two_panel_points_light_both_halves(TstSuite* suite, TstItem* item);

int test_app_offscreen_clear_color(TstSuite* suite, TstItem* item);

int test_app_capture_rejects_wrong_dimensions(TstSuite* suite, TstItem* item);

int test_app_capture_rejects_undersized_buffer(TstSuite* suite, TstItem* item);
#endif

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_scene_point_large_count_executes(TstSuite* suite, TstItem* item);
#endif

int test_scene(TstSuite* suite);
