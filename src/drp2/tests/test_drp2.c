/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing DRP2                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

#define TST_DRP2_CASE_EX(test, resources_, isolation_)                                            \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = (resources_);                                                       \
        _tst_desc.isolation = (isolation_);                                                       \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_DRP2_GPU_CASE(test)                                                                   \
    TST_DRP2_CASE_EX(test, TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN, TST_ISOLATION_PROCESS)

#define TST_DRP2_SHARED_GPU_CASE(test)                                                            \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_SERIAL;                                               \
        _tst_desc.fixture = TST_DRP2_VKLITE_FIXTURE;                                              \
        _tst_desc.fixture_scope = TST_FIXTURE_SCOPE_PROCESS;                                      \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

int test_drp2(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "drp2";

    TST_MODULE(suite, tags);

#if DVZ_DRP2_HAS_VKLITE
    tst_suite_register_fixture(
        suite, TST_DRP2_VKLITE_FIXTURE, TST_FIXTURE_SCOPE_PROCESS,
        drp2_test_vklite_fixture_create, drp2_test_vklite_fixture_destroy);
#endif

    TST_GROUP("stream");
    TST_CASE(test_drp2_stream_empty);
    TST_CASE(test_drp2_stream_append);
    TST_CASE(test_drp2_stream_debug_labels);
    TST_CASE(test_drp2_stream_json);
    TST_CASE(test_drp2_stream_texture_color_role_json);
    TST_CASE(test_drp2_stream_growth_json);
    TST_CASE(test_drp2_write_buffer_bytes_uses_data_raw);
    TST_CASE(test_drp2_write_buffer_bytes_json_encodes_data_raw);
    TST_CASE(test_drp2_stream_json_payload_refs);
    TST_CASE(test_drp2_packet_roundtrip_payload_arena);
    TST_CASE(test_drp2_packet_roundtrip_frame_metadata);
    TST_CASE(test_drp2_packet_rejects_base64_payloads);
    TST_CASE(test_drp2_packet_phase_split_roundtrip);
    TST_CASE(test_drp2_packet_shader_module_roundtrip);
    TST_CASE(test_drp2_packet_rejects_empty_shader_module_fields);
    TST_CASE(test_drp2_write_buffer_bytes_large_json_roundtrip);
    TST_CASE(test_drp2_render_pipeline_step_modes_json);
    TST_CASE(test_drp2_render_pipeline_rejects_vertex_layout_overflow);
    TST_CASE(test_drp2_render_pipeline_color_targets_json);
    TST_CASE(test_drp2_render_pipeline_raster_state);
    TST_CASE(test_drp2_wboit_accumulation_resolve_stream);

    TST_GROUP("recording");
    TST_DRP2_CASE_EX(
        test_drp2_recording_linear_roundtrip, TST_RES_CPU | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS);
    TST_DRP2_CASE_EX(
        test_drp2_recording_render_jsonl_no_raw_fallback, TST_RES_CPU | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS);
    TST_DRP2_CASE_EX(
        test_drp2_recording_compute_copy_jsonl_no_raw_fallback, TST_RES_CPU | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS);
    TST_DRP2_CASE_EX(
        test_drp2_recording_reports_raw_fallback_command, TST_RES_CPU | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS);

    TST_GROUP("render-pass");
    TST_CASE(test_drp2_begin_render_pass_clear_color_stored);
    TST_CASE(test_drp2_begin_render_pass_multi_color_attachments);
    TST_CASE(test_drp2_begin_render_pass_attachment_ops);
    TST_CASE(test_drp2_begin_render_pass_attachment_ops_validation);
    TST_CASE(test_drp2_begin_render_pass_named_depth_validation);
    TST_CASE(test_drp2_render_pipeline_rejects_depth_color_target);
    TST_CASE(test_drp2_render_pass_rejects_attachment_format_classes);
    TST_CASE(test_drp2_render_pipeline_attachment_validation);
    TST_CASE(test_drp2_recording_preserves_attachment_ops);
    TST_CASE(test_drp2_recording_preserves_named_depth);
    TST_CASE(test_drp2_stream_json_preserves_clear_color);

    TST_GROUP("runtime-validation");
    TST_CASE(test_drp2_runtime_validate_render_stream);
    TST_CASE(test_drp2_runtime_validate_render_state_inherited_across_passes);
    TST_CASE(test_drp2_runtime_validate_dynamic_viewport_scissor);
    TST_CASE(test_drp2_runtime_rejects_draw_past_vertex_buffer);
    TST_CASE(test_drp2_runtime_rejects_draw_indexed_past_index_buffer);
    TST_CASE(test_drp2_runtime_rejects_duplicate_id);
    TST_CASE(test_drp2_runtime_failed_stream_does_not_commit_state);
    TST_CASE(test_drp2_runtime_rejects_unknown_buffer_write);
    TST_CASE(test_drp2_runtime_rejects_draw_without_vertex_buffer);
    TST_CASE(test_drp2_runtime_rejects_finish_with_open_pass);
    TST_CASE(test_drp2_runtime_rejects_bad_readback_buffer);
    TST_CASE(test_drp2_runtime_validate_compute_stream);
    TST_CASE(test_drp2_runtime_rejects_dispatch_without_pipeline);
    TST_CASE(test_drp2_runtime_rejects_dispatch_outside_compute_pass);
    TST_CASE(test_drp2_runtime_rejects_wrong_pipeline_type);
    TST_CASE(test_drp2_runtime_rejects_finish_with_open_compute_pass);
    TST_CASE(test_drp2_runtime_validate_indexed_render_stream);
    TST_CASE(test_drp2_runtime_rejects_draw_indexed_without_index_buffer);
    TST_CASE(test_drp2_runtime_rejects_wrong_index_buffer_usage);
    TST_CASE(test_drp2_runtime_validate_write_texture);
    TST_CASE(test_drp2_runtime_validate_write_texture_3d_formats);
    TST_CASE(test_drp2_runtime_rejects_write_texture_format_row_layout);
    TST_CASE(test_drp2_runtime_validate_copy_buffer_to_texture);
    TST_CASE(test_drp2_runtime_validate_copy_texture_to_texture);
    TST_CASE(test_drp2_runtime_validate_texture_sampler_bind_group);
    TST_CASE(test_drp2_runtime_validate_generic_bind_group_slots);
    TST_CASE(test_drp2_runtime_rejects_bind_group_entry_mismatch);
    TST_CASE(test_drp2_runtime_validate_bind_group_dynamic_offsets);
    TST_CASE(test_drp2_runtime_validate_bind_group_after_table_growth);
    TST_CASE(test_drp2_runtime_validate_recreate_bind_group_resources);
    TST_CASE(test_drp2_runtime_reuses_submitted_transient_ids);
    TST_CASE(test_drp2_runtime_registers_external_buffer_semantic);
    TST_CASE(test_drp2_runtime_validate_compute_storage_bind_group);
    TST_CASE(test_drp2_runtime_validate_destroy_unused_bind_group);
    TST_CASE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group);
    TST_CASE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline);
    TST_CASE(test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work);
    TST_CASE(test_drp2_runtime_rejects_compute_dispatch_without_bind_group);
    TST_CASE(test_drp2_runtime_rejects_write_texture_out_of_range);
    TST_CASE(test_drp2_runtime_rejects_write_texture_layout_size_overflow);
    TST_CASE(test_drp2_runtime_rejects_copy_buffer_to_texture_usage);
    TST_CASE(test_drp2_runtime_rejects_copy_texture_to_texture_inside_pass);
    TST_CASE(test_drp2_runtime_validate_destroy_unused_buffer);
    TST_CASE(test_drp2_runtime_rejects_use_after_destroy);
    TST_CASE(test_drp2_runtime_rejects_destroy_buffer_referenced_by_work);
    TST_CASE(test_drp2_runtime_rejects_destroy_texture_referenced_by_work);
    TST_CASE(test_drp2_runtime_rejects_destroy_submitted_render_pipeline);
    TST_CASE(test_drp2_runtime_rejects_destroy_live_shader_module);

    TST_GROUP("runtime-lifecycle");
    TST_CASE(test_drp2_runtime_vklite_skeleton_create_destroy);
    TST_CASE(test_drp2_runtime_vklite_skeleton_execute_valid_stream);
    TST_CASE(test_drp2_runtime_vklite_skeleton_execute_invalid_stream);
    TST_CASE(test_drp2_runtime_vklite_skeleton_rejects_null_runtime);
    TST_CASE(test_drp2_runtime_frame_target_validation);
    TST_CASE(test_drp2_runtime_frame_lifecycle_edge_cases);
#if DVZ_DRP2_HAS_VKLITE
    TST_CASE(test_drp2_runtime_vklite_deferred_destroy_flush);
    TST_CASE(test_drp2_runtime_vklite_trims_destroyed_tail_slots);
#endif
    TST_DRP2_GPU_CASE(test_drp2_runtime_download_buffer_rejects_out_of_range);
#if DVZ_DRP2_HAS_VKLITE
    TST_GROUP("vklite-runtime");
    TST_DRP2_SHARED_GPU_CASE(test_drp2_write_buffer_bytes_large_payload_executes);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_executes_resource_commands);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_writes_buffer_contents);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_copies_buffer_contents);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_uses_external_buffer);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_writes_texture_contents);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_copies_buffer_to_texture);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_copies_texture_to_texture);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_creates_glsl_shader_modules);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_rejects_invalid_glsl_shader);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_destroy_after_partial_failure);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_creates_render_pipeline);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_reallocates_object_table_safely);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_draws_render_pass);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_draws_named_depth_render_pass);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_draws_msaa_resolve_render_pass);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_draws_rg32uint_readback);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_draws_multi_color_render_pass);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_draws_wboit_format_passes);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_draws_depth_peeling_shape);
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_samples_read_only_active_depth);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_ignores_unused_render_pass_bind_groups);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_samples_3d_texture);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_samples_then_copies_texture);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_refreshes_bind_group_after_texture_recreate);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_refreshes_bind_group_after_buffer_sampler_recreate);
    TST_DRP2_SHARED_GPU_CASE(test_drp2_runtime_vklite_refresh_defers_retired_descriptors);
#if DVZ_HAS_CUDA
    TST_DRP2_GPU_CASE(test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer);
#endif
#endif

    return 0;
}

#undef TST_DRP2_CASE_EX
#undef TST_DRP2_GPU_CASE
#undef TST_DRP2_SHARED_GPU_CASE
