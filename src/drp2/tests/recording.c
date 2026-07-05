/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 recording tests                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_assertions.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_recording_preserves_attachment_ops(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 8, 8, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 3, 2, 1, 0.0f, 0.0f, 0.0f, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        stream, 0, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
        stream, 0, DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 8,
        .height = 8,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_attachment_ops.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_attachment_ops.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[8192] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, "\"ca0_load_op\":1") != NULL);
    AT(strstr(stream_jsonl, "\"ca0_store_op\":1") != NULL);
    AT(strstr(stream_jsonl, "\"ca0_access\":2") != NULL);
    AT(strstr(stream_jsonl, "\"depth_load_op\":1") != NULL);
    AT(strstr(stream_jsonl, "\"depth_store_op\":1") != NULL);
    AT(strstr(stream_jsonl, "\"depth_access\":1") != NULL);

    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    const DvzDrp2Command* pass = dvz_drp2_stream_get(replay, 4);
    ANN(pass);
    AT(pass->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AT(pass->u.begin_render_pass.color_attachments[0].load_op == DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
    AT(
        pass->u.begin_render_pass.color_attachments[0].store_op ==
        DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE);
    AT(
        pass->u.begin_render_pass.color_attachments[0].access ==
        DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE);
    AT(pass->u.begin_render_pass.depth_load_op == DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
    AT(pass->u.begin_render_pass.depth_store_op == DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE);
    AT(pass->u.begin_render_pass.depth_access == DVZ_DRP2_ATTACHMENT_ACCESS_READ);
    AT(pass->u.begin_render_pass.depth_ops_explicit);

    dvz_drp2_stream_destroy(replay);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_recording_preserves_named_depth(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 8, 8, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 8, 8, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 4, 3, 1, 0.0f, 0.0f, 0.0f, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 0.25f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_CLEAR, DVZ_DRP2_ATTACHMENT_STORE_STORE));

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 8,
        .height = 8,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_named_depth.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_named_depth.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[8192] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, "\"depth_texture_id\":2") != NULL);

    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    const DvzDrp2Command* pass = dvz_drp2_stream_get(replay, 5);
    ANN(pass);
    AT(pass->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AT(pass->u.begin_render_pass.has_depth_attachment);
    AT(pass->u.begin_render_pass.depth_texture_id == 2);
    AC(pass->u.begin_render_pass.clear_depth, 0.25f, 1e-6f);

    char* json = dvz_drp2_stream_json(replay, "named_depth");
    ANN(json);
    AT(strstr(json, "\"depth_stencil_attachment\"") != NULL);
    AT(strstr(json, "\"texture_id\": 2") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(replay);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



/**
 * Ensure a linear DRP2 recording round-trips portable commands and payload blobs.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_recording_linear_roundtrip(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    uint8_t buffer_payload[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };
    uint8_t texture_payload[16] = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255,
    };

    AT(dvz_drp2_stream_create_buffer(
        stream, 1, sizeof(buffer_payload), DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(
        stream, 1, 0, sizeof(buffer_payload), buffer_payload));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d_region_borrowed(
        stream, 2, 0, 0, 0, 2, 2, 8, 2, texture_payload));

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    DvzDrp2CommandStream* setup_stream = dvz_drp2_stream();
    DvzDrp2CommandStream* update_stream = dvz_drp2_stream();
    ANN(setup_stream);
    ANN(update_stream);
    AT(dvz_drp2_stream_hello_renderer(setup_stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup_stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup_stream, 1, sizeof(buffer_payload), DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(
        update_stream, 1, 0, sizeof(buffer_payload), buffer_payload));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        update_stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d_region_borrowed(
        update_stream, 2, 0, 0, 0, 2, 2, 8, 2, texture_payload));

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 64,
        .height = 64,
        .duration_s = 0.016,
        .t_present = 0.016,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_linear.dvzr";
    DvzDrp2Recorder* recorder = dvz_drp2_recorder_open(path, &info);
    ANN(recorder);
    AT(dvz_drp2_recorder_write_stream(recorder, 0.0, setup_stream));
    AT(dvz_drp2_recorder_write_stream(recorder, 0.016, update_stream));
    AT(dvz_drp2_recorder_close(recorder));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_linear.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[4096] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, "\"op\":\"WriteBuffer\"") != NULL);
    AT(strstr(stream_jsonl, ".cmd") == NULL);

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    ANN(recording);
    AT(dvz_drp2_recording_frame_count(recording) == 2);
    const DvzDrp2CommandStream* loaded_stream = dvz_drp2_recording_stream(recording);
    ANN(loaded_stream);
    AT(dvz_drp2_stream_count(loaded_stream) == dvz_drp2_stream_count(stream));
    const DvzDrp2RecordedFrame* frame0 = dvz_drp2_recording_frame(recording, 0);
    const DvzDrp2RecordedFrame* frame1 = dvz_drp2_recording_frame(recording, 1);
    ANN(frame0);
    ANN(frame1);
    AT(frame0->t_present == 0.0);
    AT(frame0->first_command == 0);
    AT(frame0->command_count == 3);
    AT(frame1->t_present > 0.015 && frame1->t_present < 0.017);
    AT(frame1->first_command == 3);
    AT(frame1->command_count == 3);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_recording_execute_frame(recording, runtime, 0);
    AT(result.ok);
    result = dvz_drp2_recording_execute_frame(recording, runtime, 1);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_recording_execute_all(recording, runtime);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_recording_playback(recording, runtime, false);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    DvzDrp2CommandStream* replay_setup_frame = dvz_drp2_recording_frame_stream(recording, 0);
    DvzDrp2CommandStream* replay_update_frame = dvz_drp2_recording_frame_stream(recording, 1);
    dvz_drp2_recording_close(recording);
    ANN(replay_setup_frame);
    ANN(replay_update_frame);
    AT(dvz_drp2_stream_count(replay_setup_frame) == 3);
    AT(dvz_drp2_stream_count(replay_update_frame) == 3);

    const DvzDrp2Command* frame_write_buffer = dvz_drp2_stream_get(replay_update_frame, 0);
    ANN(frame_write_buffer);
    AT(frame_write_buffer->u.write_buffer.data_raw != NULL);
    AT(memcmp(
           frame_write_buffer->u.write_buffer.data_raw, buffer_payload,
           sizeof(buffer_payload)) == 0);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_runtime_execute(runtime, replay_setup_frame);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, replay_update_frame);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(replay_update_frame);
    dvz_drp2_stream_destroy(replay_setup_frame);

    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    AT(dvz_drp2_stream_count(replay) == dvz_drp2_stream_count(stream));

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* a = dvz_drp2_stream_get(stream, i);
        const DvzDrp2Command* b = dvz_drp2_stream_get(replay, i);
        ANN(a);
        ANN(b);
        AT(a->type == b->type);
    }

    const DvzDrp2Command* write_buffer = dvz_drp2_stream_get(replay, 3);
    ANN(write_buffer);
    AT(write_buffer->u.write_buffer.size == sizeof(buffer_payload));
    AT(write_buffer->u.write_buffer.data_raw != NULL);
    AT(memcmp(
           write_buffer->u.write_buffer.data_raw, buffer_payload, sizeof(buffer_payload)) == 0);

    const DvzDrp2Command* write_texture = dvz_drp2_stream_get(replay, 5);
    ANN(write_texture);
    AT(write_texture->u.write_texture.width == 2);
    AT(write_texture->u.write_texture.height == 2);
    AT(write_texture->u.write_texture.data_raw != NULL);
    AT(memcmp(
           write_texture->u.write_texture.data_raw, texture_payload, sizeof(texture_payload)) == 0);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_runtime_execute(runtime, replay);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(replay);
    dvz_drp2_stream_destroy(update_stream);
    dvz_drp2_stream_destroy(setup_stream);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_recording_render_jsonl_no_raw_fallback(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 8, 8, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module_format(stream, 2, "vertex", "wgsl", "vertex-main"));
    AT(dvz_drp2_stream_create_shader_module_format(stream, 3, "fragment", "wgsl", "fragment-main"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 0));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 5));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 6, 5, 1, 0.25f, 0.5f, 0.75f, 1.0f));
    AT(dvz_drp2_stream_set_viewport(stream, 6, 0.0f, 0.0f, 4.0f, 4.0f));
    AT(dvz_drp2_stream_set_scissor(stream, 6, 0.0f, 0.0f, 4.0f, 4.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 6, 4));
    AT(dvz_drp2_stream_draw(stream, 6, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 6));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 5, 7));
    AT(dvz_drp2_stream_queue_submit(stream, 7, 8));

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 8,
        .height = 8,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_render_jsonl.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_render_jsonl.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[16384] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, ".cmd") == NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CreateShaderModule\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CreateRenderPipeline\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"BeginRenderPass\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"QueueSubmit\"") != NULL);

    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    AT(dvz_drp2_stream_count(replay) == dvz_drp2_stream_count(stream));
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* a = dvz_drp2_stream_get(stream, i);
        const DvzDrp2Command* b = dvz_drp2_stream_get(replay, i);
        ANN(a);
        ANN(b);
        AT(a->type == b->type);
    }

    const DvzDrp2Command* shader = dvz_drp2_stream_get(replay, 3);
    ANN(shader);
    AT(shader->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(shader->u.create_shader_module.code != NULL);
    AT(strcmp(shader->u.create_shader_module.code, "vertex-main") == 0);

    const DvzDrp2Command* pass = dvz_drp2_stream_get(replay, 7);
    ANN(pass);
    AT(pass->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AC(pass->u.begin_render_pass.clear_color[0], 0.25f, 1e-6f);
    AC(pass->u.begin_render_pass.clear_color[1], 0.5f, 1e-6f);
    AC(pass->u.begin_render_pass.clear_color[2], 0.75f, 1e-6f);
    AT(pass->u.begin_render_pass.clear);

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_runtime_execute(runtime, replay);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(replay);

    const uint32_t spirv_words[4] = {
        0x07230203u,
        0x00010000u,
        0x000d000bu,
        0x00000001u,
    };
    DvzDrp2CommandStream* spirv_stream = dvz_drp2_stream();
    ANN(spirv_stream);
    AT(dvz_drp2_stream_create_shader_module_spirv(
        spirv_stream, 20, "vertex", (const unsigned char*)spirv_words, sizeof(spirv_words)));

    const char* spirv_path = "/tmp/dvz_drp2_recording_spirv_payload.dvzr";
    AT(dvz_drp2_recording_write_stream(spirv_path, spirv_stream, &info));

    DvzDrp2CommandStream* spirv_replay = dvz_drp2_recording_read_stream(spirv_path);
    ANN(spirv_replay);
    const DvzDrp2Command* spirv_shader = dvz_drp2_stream_get(spirv_replay, 0);
    ANN(spirv_shader);
    AT(spirv_shader->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(spirv_shader->u.create_shader_module.spirv != NULL);
    AT(spirv_shader->u.create_shader_module.spirv_size == sizeof(spirv_words));
    AT(memcmp(
           spirv_shader->u.create_shader_module.spirv, spirv_words, sizeof(spirv_words)) == 0);

    dvz_drp2_stream_destroy(spirv_replay);
    dvz_drp2_stream_destroy(spirv_stream);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_recording_compute_copy_jsonl_no_raw_fallback(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint8_t payload[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, sizeof(payload),
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, sizeof(payload),
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE));
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));

    DvzDrp2BindGroupLayoutEntry layout_entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE,
        .access = DVZ_DRP2_BINDING_ACCESS_READ_WRITE,
        .has_dynamic_offset = false,
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 3, 1, &layout_entry));

    DvzDrp2BindGroupEntry bind_entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 2,
        .offset = 0,
        .size = sizeof(payload),
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 4, 3, 1, &bind_entry));
    AT(dvz_drp2_stream_create_shader_module_format(stream, 5, "compute", "wgsl", "compute-main"));
    AT(dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(stream, 6, 5, 3));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 7));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 7, 1, 0, 2, 0, sizeof(payload)));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 8, 7));
    AT(dvz_drp2_stream_set_pipeline(stream, 8, 6));
    AT(dvz_drp2_stream_set_bind_group(stream, 8, 0, 4));
    AT(dvz_drp2_stream_dispatch_workgroups(stream, 8, 1, 1, 1));
    AT(dvz_drp2_stream_end_compute_pass(stream, 8));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 7, 9));
    AT(dvz_drp2_stream_queue_submit(stream, 9, 10));

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 0,
        .height = 0,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_compute_copy_jsonl.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_compute_copy_jsonl.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[32768] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, ".cmd") == NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CreateBindGroupLayout\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CreateBindGroup\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CreateComputePipeline\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"CopyBufferToBuffer\"") != NULL);
    AT(strstr(stream_jsonl, "\"op\":\"DispatchWorkgroups\"") != NULL);

    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    AT(dvz_drp2_stream_count(replay) == dvz_drp2_stream_count(stream));
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* a = dvz_drp2_stream_get(stream, i);
        const DvzDrp2Command* b = dvz_drp2_stream_get(replay, i);
        ANN(a);
        ANN(b);
        AT(a->type == b->type);
    }

    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    result = dvz_drp2_runtime_execute(runtime, replay);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(replay);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



/**
 * Ensure loaded recordings report ABI-local raw fallback commands.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_recording_reports_raw_fallback_command(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));

    DvzDrp2RecordingInfo info = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo),
        .width = 0,
        .height = 0,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_raw_blob.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    FILE* stream_file = fopen("/tmp/dvz_drp2_recording_raw_blob.dvzr/stream.jsonl", "rb");
    ANN(stream_file);
    char stream_jsonl[8192] = {0};
    size_t stream_jsonl_size = fread(stream_jsonl, 1, sizeof(stream_jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(stream_jsonl_size > 0);
    AT(strstr(stream_jsonl, "\"op\":\"CreateBuffer\"") != NULL);
    AT(strstr(stream_jsonl, ".cmd") != NULL);
    AT(strstr(stream_jsonl, "\"command_blob\":\"") != NULL);
    char fallback_type[64] = {0};
    snprintf(
        fallback_type, sizeof(fallback_type), "\"cmd_type\":%d",
        (int)DVZ_DRP2_COMMAND_DESTROY_BUFFER);
    AT(strstr(stream_jsonl, fallback_type) != NULL);

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    ANN(recording);
    const DvzDrp2CommandStream* loaded = dvz_drp2_recording_stream(recording);
    ANN(loaded);
    AT(dvz_drp2_stream_count(loaded) == dvz_drp2_stream_count(stream));
    const DvzDrp2Command* fallback = dvz_drp2_stream_get(loaded, 3);
    ANN(fallback);
    AT(fallback->type == DVZ_DRP2_COMMAND_DESTROY_BUFFER);
    dvz_drp2_recording_close(recording);

    dvz_drp2_stream_destroy(stream);
    return 0;
}
