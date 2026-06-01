/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 stream tests                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_stream_empty(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_count(stream) == 0);
    AT(dvz_drp2_stream_get(stream, 0) == NULL);
    AT(dvz_drp2_command_type(NULL) == DVZ_DRP2_COMMAND_NONE);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_stream_destroy(NULL);
    return 0;
}



int test_drp2_stream_append(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 2, 3));
    AT(dvz_drp2_stream_queue_submit_readback(stream, 3, 4, 1, 0, 16));

    AT(dvz_drp2_stream_count(stream) == 7);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);
    AT(dvz_drp2_stream_get(stream, 7) == NULL);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_debug_labels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_label(stream, 1) == NULL);
    AT(dvz_drp2_stream_set_label(stream, 1, "visual.0.position"));
    AT(strcmp(dvz_drp2_stream_label(stream, 1), "visual.0.position") == 0);
    AT(dvz_drp2_stream_label_id(stream, "visual.0.position") == 1);
    AT(dvz_drp2_stream_set_label(stream, 1, "visual.0.color"));
    AT(strcmp(dvz_drp2_stream_label(stream, 1), "visual.0.color") == 0);
    AT(dvz_drp2_stream_label_id(stream, "visual.0.color") == 1);
    AT(dvz_drp2_stream_label_id(stream, "visual.0.position") == 0);
    AT(!dvz_drp2_stream_set_label(NULL, 1, "ignored"));
    AT(!dvz_drp2_stream_set_label(stream, 0, "ignored"));
    AT(dvz_drp2_stream_label_id(NULL, "ignored") == 0);
    AT(dvz_drp2_stream_label_id(stream, NULL) == 0);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_json(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "fixture-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "fixture-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 4, 4, "AQIDBA=="));

    char* json = dvz_drp2_stream_json(stream, "write_buffer_basic_from_c");
    ANN(json);

    AT(strstr(json, "\"name\": \"write_buffer_basic_from_c\"") != NULL);
    AT(strstr(json, "\"cmd\": \"HelloRenderer\"") != NULL);
    AT(strstr(json, "\"cmd\": \"RendererHelloReply\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\"]") != NULL);
    AT(strstr(json, "\"cmd\": \"WriteBuffer\"") != NULL);
    AT(strstr(json, "\"data\": \"AQIDBA==\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_growth_json(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    for (uint32_t i = 0; i < 160; i++)
        AT(dvz_drp2_stream_create_buffer(stream, i + 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    AT(dvz_drp2_stream_count(stream) == 160);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 159)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_stream_get(stream, 160) == NULL);

    char* json = dvz_drp2_stream_json(stream, "stream_growth");
    ANN(json);
    AT(strstr(json, "\"name\": \"stream_growth\"") != NULL);
    AT(strstr(json, "\"id\": 160") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_write_buffer_bytes_uses_data_raw(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));
    payload[0] = 42;
    AT(dvz_drp2_stream_count(stream) == 1);

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    /* The in-process path keeps copied raw bytes and still avoids eager base64 encoding. */
    AT(cmd->u.write_buffer.data_raw != (void*)payload);
    const uint8_t expected[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(memcmp(cmd->u.write_buffer.data_raw, expected, sizeof(expected)) == 0);
    AT(cmd->u.write_buffer.data_base64 == NULL);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_write_buffer_bytes_json_encodes_data_raw(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    /* {1,2,3,4} -> base64 "AQIDBA==" */
    static const uint8_t payload[4] = {1, 2, 3, 4};
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));

    char* json = dvz_drp2_stream_json(stream, "write_buffer_bytes_json_test");
    ANN(json);
    /* Verify the JSON serializer correctly encodes data_raw on the fly. */
    AT(strstr(json, "\"data\": \"AQIDBA==\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_json_payload_refs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    static const uint8_t buffer_payload[4] = {1, 2, 3, 4};
    static const uint8_t texture_payload[16] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 0,   255,
    };
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(buffer_payload), buffer_payload));
    AT(dvz_drp2_stream_write_texture_2d_bytes(stream, 2, 0, 2, 2, 8, 2, texture_payload));

    AT(dvz_drp2_stream_payload_count(stream) == 2);
    AT(dvz_drp2_stream_payload_command_index(stream, 0) == 0);
    AT(dvz_drp2_stream_payload_command_index(stream, 1) == 1);
    AT(dvz_drp2_stream_payload_size(stream, 0) == sizeof(buffer_payload));
    AT(dvz_drp2_stream_payload_size(stream, 1) == sizeof(texture_payload));
    AT(memcmp(dvz_drp2_stream_payload_ptr(stream, 0), buffer_payload, sizeof(buffer_payload)) == 0);
    AT(memcmp(dvz_drp2_stream_payload_ptr(stream, 1), texture_payload, sizeof(texture_payload)) == 0);

    char* json = dvz_drp2_stream_json_payload_refs(stream, "payload_refs");
    ANN(json);
    AT(strstr(json, "\"data_ref\": 0") != NULL);
    AT(strstr(json, "\"data_ref\": 1") != NULL);
    AT(strstr(json, "\"data_encoding\": \"wasm-memory\"") != NULL);
    AT(strstr(json, "\"data\":") == NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_packet_roundtrip_payload_arena(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    static const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(dvz_drp2_stream_create_buffer(stream, 7, sizeof(payload), DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 7, 0, sizeof(payload), payload));

    void* packet = NULL;
    uint64_t packet_size = 0;
    void* arena = NULL;
    uint64_t arena_size = 0;
    AT(dvz_drp2_packet_encode_stream(
        stream, DVZ_DRP2_PACKET_UPDATE, 42, 9, &packet, &packet_size, &arena, &arena_size));
    ANN(packet);
    ANN(arena);
    AT(packet_size > 56);
    AT(arena_size == sizeof(payload));
    AT(memcmp(arena, payload, sizeof(payload)) == 0);

    DvzDrp2PacketInfo info = {0};
    DvzDrp2CommandStream* decoded =
        dvz_drp2_packet_decode_stream(packet, packet_size, arena, arena_size, &info);
    ANN(decoded);
    AT(info.kind == DVZ_DRP2_PACKET_UPDATE);
    AT(info.resource_version == 42);
    AT(info.frame_index == 9);
    AT(info.command_count == 2);
    AT(dvz_drp2_stream_count(decoded) == 2);

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(decoded, 1);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(cmd->u.write_buffer.buffer_id == 7);
    AT(cmd->u.write_buffer.size == sizeof(payload));
    AT(cmd->u.write_buffer.data_raw_owned);
    AT(memcmp(cmd->u.write_buffer.data_raw, payload, sizeof(payload)) == 0);

    dvz_drp2_stream_destroy(decoded);
    dvz_drp2_packet_destroy(arena);
    dvz_drp2_packet_destroy(packet);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_packet_roundtrip_frame_metadata(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_begin_command_encoder(stream, 100));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 100, 1, 0, 2, 4, 16));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 100, 101));
    AT(dvz_drp2_stream_queue_submit(stream, 101, 102));

    void* packet = NULL;
    uint64_t packet_size = 0;
    void* arena = NULL;
    uint64_t arena_size = 0;
    AT(dvz_drp2_packet_encode_stream(
        stream, DVZ_DRP2_PACKET_FRAME, 12, 34, &packet, &packet_size, &arena, &arena_size));
    ANN(packet);
    AT(arena == NULL);
    AT(arena_size == 0);

    DvzDrp2PacketInfo info = {0};
    DvzDrp2CommandStream* decoded =
        dvz_drp2_packet_decode_stream(packet, packet_size, NULL, 0, &info);
    ANN(decoded);
    AT(info.kind == DVZ_DRP2_PACKET_FRAME);
    AT(info.resource_version == 12);
    AT(info.frame_index == 34);
    AT(dvz_drp2_stream_count(decoded) == 4);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(decoded, 3)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    const DvzDrp2Command* copy = dvz_drp2_stream_get(decoded, 1);
    ANN(copy);
    AT(copy->u.copy_buffer_to_buffer.src_buffer_id == 1);
    AT(copy->u.copy_buffer_to_buffer.dst_buffer_id == 2);
    AT(copy->u.copy_buffer_to_buffer.dst_offset == 4);
    AT(copy->u.copy_buffer_to_buffer.size == 16);

    dvz_drp2_stream_destroy(decoded);
    dvz_drp2_packet_destroy(packet);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_packet_rejects_base64_payloads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 4, "AQIDBA=="));

    void* packet = NULL;
    uint64_t packet_size = 0;
    void* arena = NULL;
    uint64_t arena_size = 0;
    AT(!dvz_drp2_packet_encode_stream(
        stream, DVZ_DRP2_PACKET_UPDATE, 1, 1, &packet, &packet_size, &arena, &arena_size));
    AT(packet == NULL);
    AT(packet_size == 0);
    AT(arena == NULL);
    AT(arena_size == 0);

    dvz_drp2_stream_destroy(stream);
    return 0;
}






int test_drp2_packet_phase_split_roundtrip(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    static const uint8_t payload[4] = {9, 8, 7, 6};
    AT(dvz_drp2_stream_hello_renderer(stream, "split"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, sizeof(payload), DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    void* setup_packet = NULL;
    uint64_t setup_packet_size = 0;
    void* setup_arena = NULL;
    uint64_t setup_arena_size = 0;
    AT(dvz_drp2_packet_encode_stream_phase(
        stream, DVZ_DRP2_PACKET_SETUP, 3, 4, &setup_packet, &setup_packet_size, &setup_arena,
        &setup_arena_size));
    ANN(setup_packet);
    AT(setup_arena == NULL);
    AT(setup_arena_size == 0);

    DvzDrp2PacketInfo info = {0};
    DvzDrp2CommandStream* setup =
        dvz_drp2_packet_decode_stream(setup_packet, setup_packet_size, NULL, 0, &info);
    ANN(setup);
    AT(info.kind == DVZ_DRP2_PACKET_SETUP);
    AT(info.command_count == 2);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(setup, 1)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);

    void* update_packet = NULL;
    uint64_t update_packet_size = 0;
    void* update_arena = NULL;
    uint64_t update_arena_size = 0;
    AT(dvz_drp2_packet_encode_stream_phase(
        stream, DVZ_DRP2_PACKET_UPDATE, 3, 4, &update_packet, &update_packet_size, &update_arena,
        &update_arena_size));
    ANN(update_packet);
    ANN(update_arena);
    AT(update_arena_size == sizeof(payload));

    DvzDrp2CommandStream* update = dvz_drp2_packet_decode_stream(
        update_packet, update_packet_size, update_arena, update_arena_size, &info);
    ANN(update);
    AT(info.kind == DVZ_DRP2_PACKET_UPDATE);
    AT(info.command_count == 1);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(update, 0)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);

    void* frame_packet = NULL;
    uint64_t frame_packet_size = 0;
    void* frame_arena = NULL;
    uint64_t frame_arena_size = 0;
    AT(dvz_drp2_packet_encode_stream_phase(
        stream, DVZ_DRP2_PACKET_FRAME, 3, 4, &frame_packet, &frame_packet_size, &frame_arena,
        &frame_arena_size));
    ANN(frame_packet);
    AT(frame_arena == NULL);
    AT(frame_arena_size == 0);

    DvzDrp2CommandStream* frame =
        dvz_drp2_packet_decode_stream(frame_packet, frame_packet_size, NULL, 0, &info);
    ANN(frame);
    AT(info.kind == DVZ_DRP2_PACKET_FRAME);
    AT(info.command_count == 3);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(frame, 2)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    dvz_drp2_stream_destroy(frame);
    dvz_drp2_packet_destroy(frame_packet);
    dvz_drp2_stream_destroy(update);
    dvz_drp2_packet_destroy(update_arena);
    dvz_drp2_packet_destroy(update_packet);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_packet_destroy(setup_packet);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_packet_shader_module_roundtrip(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    const char* code = "@vertex fn main() -> @builtin(position) vec4f { return vec4f(); }";
    AT(dvz_drp2_stream_create_shader_module_format(stream, 77, "VERTEX", "wgsl", code));
    AT(dvz_drp2_stream_shader_set_builtin_identity(stream, 77, "point", "main", 2));

    void* packet = NULL;
    uint64_t packet_size = 0;
    void* arena = NULL;
    uint64_t arena_size = 0;
    AT(dvz_drp2_packet_encode_stream_phase(
        stream, DVZ_DRP2_PACKET_SETUP, 8, 9, &packet, &packet_size, &arena, &arena_size));
    ANN(packet);
    ANN(arena);
    AT(arena_size == strlen(code) + 1);

    DvzDrp2PacketInfo info = {0};
    DvzDrp2CommandStream* decoded =
        dvz_drp2_packet_decode_stream(packet, packet_size, arena, arena_size, &info);
    ANN(decoded);
    AT(info.kind == DVZ_DRP2_PACKET_SETUP);
    AT(info.command_count == 1);
    const DvzDrp2Command* cmd = dvz_drp2_stream_get(decoded, 0);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(cmd->u.create_shader_module.id == 77);
    AT(strcmp(cmd->u.create_shader_module.format, "wgsl") == 0);
    AT(strcmp(cmd->u.create_shader_module.builtin_family, "point") == 0);
    AT(strcmp(cmd->u.create_shader_module.builtin_variant, "main") == 0);
    AT(cmd->u.create_shader_module.builtin_version == 2);
    AT(strcmp(cmd->u.create_shader_module.code, code) == 0);

    dvz_drp2_stream_destroy(decoded);
    dvz_drp2_packet_destroy(arena);
    dvz_drp2_packet_destroy(packet);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_write_buffer_bytes_large_json_roundtrip(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    /* 3000 floats = 12000 bytes, verifying JSON encodes the full data_raw payload. */
    const uint32_t N    = 3000;
    const uint64_t SIZE = N * sizeof(float);
    float* data = (float*)dvz_malloc(SIZE);
    ANN(data);
    for (uint32_t i = 0; i < N; i++)
        data[i] = (float)i * 0.5f;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, SIZE, data));

    char* json = dvz_drp2_stream_json(stream, "large_json_roundtrip");
    ANN(json);
    /* JSON must contain a "data" field with non-empty base64 encoding of the full payload. */
    AT(strstr(json, "\"data\": \"") != NULL);
    /* The base64 of 12000 bytes is 16000 chars, so verify the JSON is large enough. */
    AT(strlen(json) > 16000);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_free(data);
    return 0;
}



int test_drp2_render_pipeline_step_modes_json(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint32_t strides[2] = {3 * sizeof(float), 4 * sizeof(uint8_t)};
    uint32_t step_modes[2] = {
        DVZ_DRP2_VERTEX_STEP_MODE_VERTEX,
        DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE,
    };
    uint32_t bindings[2] = {0, 1};
    uint32_t locations[2] = {0, 1};
    uint32_t formats[2] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
    uint32_t offsets[2] = {0, 0};

    AT(dvz_drp2_stream_create_render_pipeline_ex2(
        stream, 10, 9000, 9001, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 2, strides,
        step_modes, 2, bindings, locations, formats, offsets));

    char* json = dvz_drp2_stream_json(stream, "pipeline_step_modes");
    ANN(json);
    AT(strstr(json, "\"step_mode\": \"vertex\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_render_pipeline_color_targets_json(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_create_render_pipeline(stream, 10, 9000, 9001, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, VK_FORMAT_R16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 1, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT));

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->u.create_render_pipeline.color_target_count == 2);
    AT(cmd->u.create_render_pipeline.color_targets[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(cmd->u.create_render_pipeline.color_targets[1].format == VK_FORMAT_R16_SFLOAT);
    AT(cmd->u.create_render_pipeline.color_targets[0].blend_enabled);
    AT(cmd->u.create_render_pipeline.color_targets[1].blend_enabled);

    char* json = dvz_drp2_stream_json(stream, "pipeline_color_targets");
    ANN(json);
    AT(strstr(json, "\"format\": \"rgba16float\"") != NULL);
    AT(strstr(json, "\"format\": \"r16float\"") != NULL);
    AT(strstr(json, "\"write_mask\": [\"red\", \"green\", \"blue\", \"alpha\"]") != NULL);
    AT(strstr(json, "\"blend\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_render_pipeline_raster_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_create_render_pipeline(stream, 10, 9000, 9001, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE));

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->u.create_render_pipeline.has_raster_state);
    AT(cmd->u.create_render_pipeline.cull_mode == VK_CULL_MODE_BACK_BIT);
    AT(cmd->u.create_render_pipeline.front_face == VK_FRONT_FACE_CLOCKWISE);

    char* json = dvz_drp2_stream_json(stream, "pipeline_raster_state");
    ANN(json);
    AT(strstr(json, "\"cull_mode\": \"back\"") != NULL);
    AT(strstr(json, "\"front_face\": \"clockwise\"") != NULL);
    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE));
    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    DvzDrp2RecordingInfo info = {
        .width = 8,
        .height = 8,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    const char* path = "/tmp/dvz_drp2_recording_raster_state.dvzr";
    AT(dvz_drp2_recording_write_stream(path, stream, &info));
    DvzDrp2CommandStream* replay = dvz_drp2_recording_read_stream(path);
    ANN(replay);
    const DvzDrp2Command* replay_pipeline = dvz_drp2_stream_get(replay, 4);
    ANN(replay_pipeline);
    AT(replay_pipeline->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    AT(replay_pipeline->u.create_render_pipeline.has_raster_state);
    AT(replay_pipeline->u.create_render_pipeline.cull_mode == VK_CULL_MODE_BACK_BIT);
    AT(replay_pipeline->u.create_render_pipeline.front_face == VK_FRONT_FACE_CLOCKWISE);
    dvz_drp2_stream_destroy(replay);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(stream, 0x80000000u, VK_FRONT_FACE_CLOCKWISE));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(stream, VK_CULL_MODE_BACK_BIT, 99));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_wboit_accumulation_resolve_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 36, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_write_buffer(
        stream, 1, 0, 36, "AAAAAAAAAAAAAAAAAAAAAAAA" "AAAAAAAAAAAAAAAAAAAAAAAA"));
    AT(dvz_drp2_stream_create_sampler(stream, 2));

    DvzDrp2BindGroupLayoutEntry layout_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 3, 3, layout_entries));

    AT(dvz_drp2_stream_create_shader_module(
        stream, 10, "VERTEX", "@vertex fn main() -> @builtin(position) vec4f { return vec4f(); }"));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 11, "FRAGMENT",
        "@fragment fn main() -> @location(0) vec4f { return vec4f(1.0); }"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 12, 10, 11, 1));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, VK_FORMAT_R16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 1, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, VK_COMPARE_OP_LESS_OR_EQUAL));

    AT(dvz_drp2_stream_create_shader_module(
        stream, 20, "VERTEX", "@vertex fn main() -> @builtin(position) vec4f { return vec4f(); }"));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 21, "FRAGMENT",
        "@group(0) @binding(0) var accum: texture_2d<f32>; "
        "@group(0) @binding(1) var reveal: texture_2d<f32>; "
        "@group(0) @binding(2) var samp: sampler; "
        "@fragment fn main() -> @location(0) vec4f { "
        "return textureSample(accum, samp, vec2f(0.5)); }"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, 22, 20, 21, 0, 3));

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 30, 4, 4, VK_FORMAT_R16G16B16A16_SFLOAT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 31, 4, 4, VK_FORMAT_R16_SFLOAT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 32, 4, 4, VK_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 33, 4, 4, VK_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));

    const DvzDrp2Command* accum_texture = dvz_drp2_stream_get(stream, 12);
    const DvzDrp2Command* reveal_texture = dvz_drp2_stream_get(stream, 13);
    ANN(accum_texture);
    ANN(reveal_texture);
    AT(accum_texture->u.create_texture.format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(reveal_texture->u.create_texture.format == VK_FORMAT_R16_SFLOAT);

    DvzDrp2BindGroupEntry bind_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 30,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 31,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 2,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 34, 3, 3, bind_entries));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 40));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 41, 40, 30, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 31, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 41, 12));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 41, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 41, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 41));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 42, 40, 32, 0, 0, 0, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 42, 22));
    AT(dvz_drp2_stream_set_bind_group(stream, 42, 0, 34));
    AT(dvz_drp2_stream_draw(stream, 42, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 42));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 40, 43));
    AT(dvz_drp2_stream_queue_submit(stream, 43, 44));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(stream, "wboit_accumulation_resolve_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"rgba16float\"") != NULL);
    AT(strstr(json, "\"format\": \"r16float\"") != NULL);
    AT(strstr(json, "\"blend\"") != NULL);
    AT(strstr(json, "\"texture_id\": 30") != NULL);
    AT(strstr(json, "\"texture_id\": 31") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}
