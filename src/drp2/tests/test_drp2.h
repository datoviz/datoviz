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

int test_drp2_stream_empty(TstSuite* suite, TstItem* item);

int test_drp2_stream_append(TstSuite* suite, TstItem* item);

int test_drp2_stream_debug_labels(TstSuite* suite, TstItem* item);

int test_drp2_stream_json(TstSuite* suite, TstItem* item);

int test_drp2_stream_growth_json(TstSuite* suite, TstItem* item);

int test_drp2_write_buffer_bytes_uses_data_raw(TstSuite* suite, TstItem* item);

int test_drp2_write_buffer_bytes_json_encodes_data_raw(TstSuite* suite, TstItem* item);

int test_drp2_render_pipeline_step_modes_json(TstSuite* suite, TstItem* item);

int test_drp2_recording_linear_roundtrip(TstSuite* suite, TstItem* item);

int test_drp2_recording_render_jsonl_no_raw_fallback(TstSuite* suite, TstItem* item);

int test_drp2_recording_compute_copy_jsonl_no_raw_fallback(TstSuite* suite, TstItem* item);

int test_drp2_recording_reports_raw_fallback_command(TstSuite* suite, TstItem* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_write_buffer_bytes_large_payload_executes(TstSuite* suite, TstItem* item);
#endif



int test_drp2_runtime_validate_render_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_render_state_inherited_across_passes(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_dynamic_viewport_scissor(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_duplicate_id(TstSuite* suite, TstItem* item);

int test_drp2_runtime_failed_stream_does_not_commit_state(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_unknown_buffer_write(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_draw_without_vertex_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_finish_with_open_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_bad_readback_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_compute_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_dispatch_without_pipeline(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_dispatch_outside_compute_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_wrong_pipeline_type(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_finish_with_open_compute_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_indexed_render_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_draw_indexed_without_index_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_wrong_index_buffer_usage(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_write_texture(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_copy_buffer_to_texture(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_copy_texture_to_texture(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_texture_sampler_bind_group(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_generic_bind_group_slots(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_bind_group_entry_mismatch(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_bind_group_dynamic_offsets(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_bind_group_after_table_growth(TstSuite* suite, TstItem* item);

int test_drp2_runtime_reuses_submitted_transient_ids(TstSuite* suite, TstItem* item);

int test_drp2_runtime_registers_external_buffer_semantic(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_compute_storage_bind_group(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_destroy_unused_bind_group(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_compute_dispatch_without_bind_group(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_write_texture_out_of_range(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_write_texture_layout_size_overflow(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_copy_buffer_to_texture_usage(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_copy_texture_to_texture_inside_pass(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_destroy_unused_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_use_after_destroy(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_buffer_referenced_by_work(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_texture_referenced_by_work(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_submitted_render_pipeline(
    TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_destroy_live_shader_module(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_skeleton_create_destroy(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_skeleton_execute_valid_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_skeleton_execute_invalid_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_skeleton_rejects_null_runtime(TstSuite* suite, TstItem* item);

int test_drp2_runtime_frame_target_validation(TstSuite* suite, TstItem* item);

int test_drp2_runtime_frame_lifecycle_edge_cases(TstSuite* suite, TstItem* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_deferred_destroy_flush(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_trims_destroyed_tail_slots(TstSuite* suite, TstItem* item);
#endif

int test_drp2_runtime_download_buffer_rejects_out_of_range(TstSuite* suite, TstItem* item);

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_executes_resource_commands(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_writes_buffer_contents(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_copies_buffer_contents(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_uses_external_buffer(TstSuite* suite, TstItem* item);

#if DVZ_HAS_CUDA
int test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer(
    TstSuite* suite, TstItem* item);
#endif

int test_drp2_runtime_vklite_writes_texture_contents(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_copies_buffer_to_texture(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_copies_texture_to_texture(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_creates_glsl_shader_modules(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_creates_render_pipeline(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_rejects_invalid_glsl_shader(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_destroy_after_partial_failure(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_reallocates_object_table_safely(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_draws_render_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_draws_multi_color_render_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_draws_wboit_format_passes(TstSuite* suite, TstItem* item);

int test_drp2_runtime_vklite_samples_then_copies_texture(TstSuite* suite, TstItem* item);
#endif


int test_drp2_render_pipeline_color_targets_json(TstSuite* suite, TstItem* item);

int test_drp2_wboit_accumulation_resolve_stream(TstSuite* suite, TstItem* item);



int test_drp2_begin_render_pass_clear_color_stored(TstSuite* suite, TstItem* item);

int test_drp2_begin_render_pass_multi_color_attachments(TstSuite* suite, TstItem* item);

int test_drp2_begin_render_pass_attachment_ops(TstSuite* suite, TstItem* item);

int test_drp2_begin_render_pass_attachment_ops_validation(TstSuite* suite, TstItem* item);

int test_drp2_recording_preserves_attachment_ops(TstSuite* suite, TstItem* item);

int test_drp2_stream_json_preserves_clear_color(TstSuite* suite, TstItem* item);

int test_drp2_write_buffer_bytes_large_json_roundtrip(TstSuite* suite, TstItem* item);

int test_drp2(TstSuite* suite);
