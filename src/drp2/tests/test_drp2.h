/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing DRP2                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_stream_empty(TstContext* suite, const TstCase* item);

int test_drp2_stream_append(TstContext* suite, const TstCase* item);

int test_drp2_stream_debug_labels(TstContext* suite, const TstCase* item);

int test_drp2_stream_json(TstContext* suite, const TstCase* item);

int test_drp2_stream_growth_json(TstContext* suite, const TstCase* item);

int test_drp2_write_buffer_bytes_uses_data_raw(TstContext* suite, const TstCase* item);

int test_drp2_write_buffer_bytes_json_encodes_data_raw(TstContext* suite, const TstCase* item);

int test_drp2_render_pipeline_step_modes_json(TstContext* suite, const TstCase* item);

int test_drp2_recording_linear_roundtrip(TstContext* suite, const TstCase* item);

int test_drp2_recording_render_jsonl_no_raw_fallback(TstContext* suite, const TstCase* item);

int test_drp2_recording_compute_copy_jsonl_no_raw_fallback(TstContext* suite, const TstCase* item);

int test_drp2_recording_reports_raw_fallback_command(TstContext* suite, const TstCase* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_write_buffer_bytes_large_payload_executes(TstContext* suite, const TstCase* item);
#endif



int test_drp2_runtime_validate_render_stream(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_render_state_inherited_across_passes(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_dynamic_viewport_scissor(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_duplicate_id(TstContext* suite, const TstCase* item);

int test_drp2_runtime_failed_stream_does_not_commit_state(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_unknown_buffer_write(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_draw_without_vertex_buffer(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_finish_with_open_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_bad_readback_buffer(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_compute_stream(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_dispatch_without_pipeline(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_dispatch_outside_compute_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_wrong_pipeline_type(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_finish_with_open_compute_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_indexed_render_stream(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_draw_indexed_without_index_buffer(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_wrong_index_buffer_usage(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_write_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_write_texture_3d_formats(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_write_texture_format_row_layout(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_copy_buffer_to_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_copy_texture_to_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_texture_sampler_bind_group(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_generic_bind_group_slots(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_bind_group_entry_mismatch(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_bind_group_dynamic_offsets(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_bind_group_after_table_growth(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_recreate_bind_group_resources(TstContext* suite, const TstCase* item);

int test_drp2_runtime_reuses_submitted_transient_ids(TstContext* suite, const TstCase* item);

int test_drp2_runtime_registers_external_buffer_semantic(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_compute_storage_bind_group(TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_destroy_unused_bind_group(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_compute_dispatch_without_bind_group(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_write_texture_out_of_range(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_write_texture_layout_size_overflow(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_copy_buffer_to_texture_usage(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_copy_texture_to_texture_inside_pass(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_validate_destroy_unused_buffer(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_use_after_destroy(TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_buffer_referenced_by_work(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_texture_referenced_by_work(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_submitted_render_pipeline(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_rejects_destroy_live_shader_module(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_skeleton_create_destroy(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_skeleton_execute_valid_stream(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_skeleton_execute_invalid_stream(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_skeleton_rejects_null_runtime(TstContext* suite, const TstCase* item);

int test_drp2_runtime_frame_target_validation(TstContext* suite, const TstCase* item);

int test_drp2_runtime_frame_lifecycle_edge_cases(TstContext* suite, const TstCase* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_deferred_destroy_flush(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_trims_destroyed_tail_slots(TstContext* suite, const TstCase* item);
#endif

int test_drp2_runtime_download_buffer_rejects_out_of_range(TstContext* suite, const TstCase* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_executes_resource_commands(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_writes_buffer_contents(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_copies_buffer_contents(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_uses_external_buffer(TstContext* suite, const TstCase* item);

#if DVZ_HAS_CUDA
int test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer(
    TstContext* suite, const TstCase* item);
#endif

int test_drp2_runtime_vklite_writes_texture_contents(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_copies_buffer_to_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_copies_texture_to_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_creates_glsl_shader_modules(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_creates_render_pipeline(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_rejects_invalid_glsl_shader(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_destroy_after_partial_failure(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_reallocates_object_table_safely(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_render_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_named_depth_render_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_msaa_resolve_render_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_multi_color_render_pass(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_wboit_format_passes(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_draws_depth_peeling_shape(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_samples_read_only_active_depth(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_samples_3d_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_samples_then_copies_texture(TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_refreshes_bind_group_after_texture_recreate(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_refreshes_bind_group_after_buffer_sampler_recreate(
    TstContext* suite, const TstCase* item);

int test_drp2_runtime_vklite_refresh_defers_retired_descriptors(
    TstContext* suite, const TstCase* item);
#endif


int test_drp2_render_pipeline_color_targets_json(TstContext* suite, const TstCase* item);

int test_drp2_render_pipeline_raster_state(TstContext* suite, const TstCase* item);

int test_drp2_wboit_accumulation_resolve_stream(TstContext* suite, const TstCase* item);



int test_drp2_begin_render_pass_clear_color_stored(TstContext* suite, const TstCase* item);

int test_drp2_begin_render_pass_multi_color_attachments(TstContext* suite, const TstCase* item);

int test_drp2_begin_render_pass_attachment_ops(TstContext* suite, const TstCase* item);

int test_drp2_begin_render_pass_attachment_ops_validation(TstContext* suite, const TstCase* item);

int test_drp2_begin_render_pass_named_depth_validation(TstContext* suite, const TstCase* item);

int test_drp2_render_pipeline_rejects_depth_color_target(TstContext* suite, const TstCase* item);

int test_drp2_render_pass_rejects_attachment_format_classes(TstContext* suite, const TstCase* item);

int test_drp2_render_pipeline_attachment_validation(TstContext* suite, const TstCase* item);

int test_drp2_recording_preserves_attachment_ops(TstContext* suite, const TstCase* item);

int test_drp2_recording_preserves_named_depth(TstContext* suite, const TstCase* item);

int test_drp2_stream_json_preserves_clear_color(TstContext* suite, const TstCase* item);

int test_drp2_write_buffer_bytes_large_json_roundtrip(TstContext* suite, const TstCase* item);

int test_drp2(TstSuite* suite);
