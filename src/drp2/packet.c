/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 binary packets                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_stream.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_PACKET_MAGIC "DVP2PKT"
#define DVZ_DRP2_PACKET_MAGIC_SIZE 8
#define DVZ_DRP2_PACKET_HEADER_SIZE 56
#define DVZ_DRP2_PACKET_RECORD_SIZE 32
#define DVZ_DRP2_PACKET_VERSION_MAJOR 2
#define DVZ_DRP2_PACKET_VERSION_MINOR 0
#define DVZ_DRP2_PACKET_NO_PAYLOAD UINT64_MAX



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PacketWriteBufferBody
{
    uint64_t buffer_id;
    uint64_t offset;
    uint64_t size;
} PacketWriteBufferBody;


typedef struct PacketWriteTextureBody
{
    uint64_t texture_id;
    uint32_t mip_level;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t origin_z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t bytes_per_row;
    uint32_t rows_per_image;
} PacketWriteTextureBody;


typedef struct PacketShaderBody
{
    uint64_t id;
    char stage[DVZ_DRP2_LABEL_SIZE];
    char format[DVZ_DRP2_LABEL_SIZE];
    char builtin_family[DVZ_DRP2_LABEL_SIZE];
    char builtin_variant[DVZ_DRP2_LABEL_SIZE];
    uint32_t builtin_version;
    uint32_t payload_kind; /* 1=UTF-8 source, 2=SPIR-V bytes. */
    uint64_t payload_size;
} PacketShaderBody;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint64_t _align8(uint64_t x)
{
    return (x + 7u) & ~UINT64_C(7);
}



static void _put_u16(uint8_t* dst, uint16_t x)
{
    ANN(dst);
    dst[0] = (uint8_t)(x & 0xffu);
    dst[1] = (uint8_t)((x >> 8) & 0xffu);
}



static void _put_u32(uint8_t* dst, uint32_t x)
{
    ANN(dst);
    for (uint32_t i = 0; i < 4; i++)
        dst[i] = (uint8_t)((x >> (8u * i)) & 0xffu);
}



static void _put_u64(uint8_t* dst, uint64_t x)
{
    ANN(dst);
    for (uint32_t i = 0; i < 8; i++)
        dst[i] = (uint8_t)((x >> (8u * i)) & 0xffu);
}



static uint16_t _get_u16(const uint8_t* src)
{
    ANN(src);
    return (uint16_t)src[0] | (uint16_t)((uint16_t)src[1] << 8u);
}



static uint32_t _get_u32(const uint8_t* src)
{
    ANN(src);
    uint32_t out = 0;
    for (uint32_t i = 0; i < 4; i++)
        out |= (uint32_t)src[i] << (8u * i);
    return out;
}



static uint64_t _get_u64(const uint8_t* src)
{
    ANN(src);
    uint64_t out = 0;
    for (uint32_t i = 0; i < 8; i++)
        out |= (uint64_t)src[i] << (8u * i);
    return out;
}





DvzDrp2PacketKind dvz_drp2_packet_command_kind(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return DVZ_DRP2_PACKET_SETUP;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return DVZ_DRP2_PACKET_UPDATE;
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
    case DVZ_DRP2_COMMAND_DRAW:
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_RESOURCE_BARRIER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return DVZ_DRP2_PACKET_FRAME;
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return DVZ_DRP2_PACKET_NONE;
    }
}


static uint64_t _fixed_body_size(DvzDrp2CommandType type)
{
#define BODY_SIZE(name) sizeof(((DvzDrp2Command*)0)->u.name)
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        return BODY_SIZE(handshake);
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return BODY_SIZE(create_buffer);
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return BODY_SIZE(destroy_buffer);
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return BODY_SIZE(create_texture);
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return BODY_SIZE(destroy_texture);
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return BODY_SIZE(destroy_shader_module);
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return BODY_SIZE(create_render_pipeline);
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return BODY_SIZE(destroy_render_pipeline);
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return BODY_SIZE(create_compute_pipeline);
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return BODY_SIZE(destroy_compute_pipeline);
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return BODY_SIZE(create_sampler);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return BODY_SIZE(create_bind_group_layout);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return BODY_SIZE(create_bind_group);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return BODY_SIZE(destroy_bind_group_layout);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return BODY_SIZE(destroy_bind_group);
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return BODY_SIZE(begin_command_encoder);
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return BODY_SIZE(begin_render_pass);
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return BODY_SIZE(begin_compute_pass);
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return BODY_SIZE(set_viewport);
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return BODY_SIZE(set_scissor);
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return BODY_SIZE(set_pipeline);
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return BODY_SIZE(set_bind_group);
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return BODY_SIZE(set_vertex_buffer);
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return BODY_SIZE(set_index_buffer);
    case DVZ_DRP2_COMMAND_DRAW:
        return BODY_SIZE(draw);
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return BODY_SIZE(draw_indexed);
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return BODY_SIZE(end_render_pass);
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return BODY_SIZE(dispatch);
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return BODY_SIZE(end_compute_pass);
    case DVZ_DRP2_COMMAND_RESOURCE_BARRIER:
        return BODY_SIZE(resource_barrier);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return BODY_SIZE(copy_buffer_to_buffer);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return BODY_SIZE(copy_buffer_to_texture);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return BODY_SIZE(copy_texture_to_buffer);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return BODY_SIZE(copy_texture_to_texture);
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return BODY_SIZE(finish_command_encoder);
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return BODY_SIZE(queue_submit);
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return sizeof(PacketWriteBufferBody);
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return sizeof(PacketWriteTextureBody);
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return sizeof(PacketShaderBody);
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return 0;
    }
#undef BODY_SIZE
}



static bool _payload_info(const DvzDrp2Command* command, const void** out_ptr, uint64_t* out_size)
{
    ANN(command);
    if (out_ptr != NULL)
        *out_ptr = NULL;
    if (out_size != NULL)
        *out_size = 0;

    const void* ptr = NULL;
    uint64_t size = 0;
    if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
    {
        ptr = command->u.write_buffer.data_raw;
        size = command->u.write_buffer.size;
    }
    else if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
    {
        ptr = command->u.write_texture.data_raw;
        if (_dvz_mul_u64_overflows(
                (uint64_t)command->u.write_texture.depth,
                (uint64_t)command->u.write_texture.rows_per_image, &size) ||
            _dvz_mul_u64_overflows(size, (uint64_t)command->u.write_texture.bytes_per_row, &size))
            return false;
    }
    else if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
    {
        if (command->u.create_shader_module.code != NULL)
        {
            ptr = command->u.create_shader_module.code;
            size = (uint64_t)strlen(command->u.create_shader_module.code) + 1;
        }
        else
        {
            ptr = command->u.create_shader_module.spirv;
            size = command->u.create_shader_module.spirv_size;
        }
    }

    if (ptr == NULL || size == 0)
        return false;
    if (out_ptr != NULL)
        *out_ptr = ptr;
    if (out_size != NULL)
        *out_size = size;
    return true;
}



static const void* _body_ptr(const DvzDrp2Command* command, uint8_t* scratch)
{
    ANN(command);
    ANN(scratch);
    if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
    {
        PacketWriteBufferBody body = {
            command->u.write_buffer.buffer_id,
            command->u.write_buffer.offset,
            command->u.write_buffer.size,
        };
        memcpy(scratch, &body, sizeof(body));
        return scratch;
    }
    if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
    {
        PacketWriteTextureBody body = {
            command->u.write_texture.texture_id,
            command->u.write_texture.mip_level,
            command->u.write_texture.origin_x,
            command->u.write_texture.origin_y,
            command->u.write_texture.origin_z,
            command->u.write_texture.width,
            command->u.write_texture.height,
            command->u.write_texture.depth,
            command->u.write_texture.bytes_per_row,
            command->u.write_texture.rows_per_image,
        };
        memcpy(scratch, &body, sizeof(body));
        return scratch;
    }
    if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
    {
        PacketShaderBody body = {0};
        body.id = command->u.create_shader_module.id;
        memcpy(body.stage, command->u.create_shader_module.stage, sizeof(body.stage));
        memcpy(body.format, command->u.create_shader_module.format, sizeof(body.format));
        memcpy(body.builtin_family, command->u.create_shader_module.builtin_family,
               sizeof(body.builtin_family));
        memcpy(body.builtin_variant, command->u.create_shader_module.builtin_variant,
               sizeof(body.builtin_variant));
        body.builtin_version = command->u.create_shader_module.builtin_version;
        if (command->u.create_shader_module.code != NULL)
        {
            body.payload_kind = 1;
            body.payload_size = (uint64_t)strlen(command->u.create_shader_module.code) + 1;
        }
        else
        {
            body.payload_kind = 2;
            body.payload_size = command->u.create_shader_module.spirv_size;
        }
        memcpy(scratch, &body, sizeof(body));
        return scratch;
    }
    return &command->u;
}



static bool _decode_body(
    DvzDrp2Command* command, const uint8_t* body, uint64_t body_size, const uint8_t* arena,
    uint64_t payload_offset, uint64_t payload_size)
{
    ANN(command);
    ANN(body);
    const uint64_t expected = _fixed_body_size(command->type);
    if (expected == 0 || expected != body_size)
        return false;

    if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
    {
        const PacketWriteBufferBody* wb = (const PacketWriteBufferBody*)body;
        command->u.write_buffer.buffer_id = wb->buffer_id;
        command->u.write_buffer.offset = wb->offset;
        command->u.write_buffer.size = wb->size;
        if (payload_size != wb->size || payload_offset == DVZ_DRP2_PACKET_NO_PAYLOAD)
            return false;
        void* data = dvz_malloc(payload_size);
        if (data == NULL)
            return false;
        memcpy(data, arena + payload_offset, (size_t)payload_size);
        command->u.write_buffer.data_raw = data;
        command->u.write_buffer.data_raw_owned = true;
        return true;
    }

    if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
    {
        const PacketWriteTextureBody* wt = (const PacketWriteTextureBody*)body;
        command->u.write_texture.texture_id = wt->texture_id;
        command->u.write_texture.mip_level = wt->mip_level;
        command->u.write_texture.origin_x = wt->origin_x;
        command->u.write_texture.origin_y = wt->origin_y;
        command->u.write_texture.origin_z = wt->origin_z;
        command->u.write_texture.width = wt->width;
        command->u.write_texture.height = wt->height;
        command->u.write_texture.depth = wt->depth;
        command->u.write_texture.bytes_per_row = wt->bytes_per_row;
        command->u.write_texture.rows_per_image = wt->rows_per_image;
        uint64_t expected_payload_size = 0;
        if (_dvz_mul_u64_overflows((uint64_t)wt->depth, (uint64_t)wt->rows_per_image,
                                   &expected_payload_size) ||
            _dvz_mul_u64_overflows(expected_payload_size, (uint64_t)wt->bytes_per_row,
                                   &expected_payload_size) ||
            payload_offset == DVZ_DRP2_PACKET_NO_PAYLOAD || payload_size != expected_payload_size)
            return false;
        command->u.write_texture.data_raw = arena + payload_offset;
        return true;
    }

    if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
    {
        const PacketShaderBody* sh = (const PacketShaderBody*)body;
        if (payload_offset == DVZ_DRP2_PACKET_NO_PAYLOAD || payload_size != sh->payload_size)
            return false;
        command->u.create_shader_module.id = sh->id;
        memcpy(command->u.create_shader_module.stage, sh->stage,
               sizeof(command->u.create_shader_module.stage));
        memcpy(command->u.create_shader_module.format, sh->format,
               sizeof(command->u.create_shader_module.format));
        memcpy(command->u.create_shader_module.builtin_family, sh->builtin_family,
               sizeof(command->u.create_shader_module.builtin_family));
        memcpy(command->u.create_shader_module.builtin_variant, sh->builtin_variant,
               sizeof(command->u.create_shader_module.builtin_variant));
        command->u.create_shader_module.builtin_version = sh->builtin_version;
        if (sh->payload_kind == 1)
        {
            if (payload_size == 0 || arena[payload_offset + payload_size - 1] != 0)
                return false;
            char* code = (char*)dvz_malloc(payload_size);
            if (code == NULL)
                return false;
            memcpy(code, arena + payload_offset, (size_t)payload_size);
            command->u.create_shader_module.code = code;
        }
        else if (sh->payload_kind == 2)
        {
            command->u.create_shader_module.spirv = arena + payload_offset;
            command->u.create_shader_module.spirv_size = payload_size;
        }
        else
            return false;
        return true;
    }

    if (payload_offset != DVZ_DRP2_PACKET_NO_PAYLOAD || payload_size != 0)
        return false;
    memcpy(&command->u, body, (size_t)body_size);
    return true;
}



static bool _ensure_decode_capacity(DvzDrp2CommandStream* stream, uint32_t count)
{
    ANN(stream);
    if (count <= stream->capacity)
        return true;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, sizeof(DvzDrp2Command), &bytes))
        return false;
    DvzDrp2Command* commands = (DvzDrp2Command*)dvz_realloc(stream->commands, bytes);
    if (commands == NULL)
        return false;
    stream->commands = commands;
    stream->capacity = count;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Encode a DRP2 command stream as a binary packet plus payload arena.
 */
static bool _packet_encode_stream_filtered(
    const DvzDrp2CommandStream* stream, DvzDrp2PacketKind kind, uint64_t resource_version,
    uint64_t frame_index, bool filter_phase, void** packet, uint64_t* packet_size, void** arena,
    uint64_t* arena_size)
{
    if (packet != NULL)
        *packet = NULL;
    if (packet_size != NULL)
        *packet_size = 0;
    if (arena != NULL)
        *arena = NULL;
    if (arena_size != NULL)
        *arena_size = 0;
    if (stream == NULL || packet == NULL || packet_size == NULL || arena == NULL ||
        arena_size == NULL || kind == DVZ_DRP2_PACKET_NONE)
        return false;

    uint32_t selected_count = 0;
    uint64_t command_bytes = 0;
    uint64_t payload_bytes = 0;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        if (filter_phase && dvz_drp2_packet_command_kind(command->type) != kind)
            continue;
        selected_count++;
        const uint64_t body_size = _fixed_body_size(command->type);
        if (body_size == 0)
        {
            log_error("DRP2 packet encode does not support command type %d", command->type);
            return false;
        }
        uint64_t record_bytes = 0;
        if (_dvz_add_u64_overflows(DVZ_DRP2_PACKET_RECORD_SIZE, _align8(body_size), &record_bytes) ||
            _dvz_add_u64_overflows(command_bytes, record_bytes, &command_bytes))
            return false;

        const void* payload_ptr = NULL;
        uint64_t payload_size = 0;
        if (_payload_info(command, &payload_ptr, &payload_size))
        {
            payload_bytes = _align8(payload_bytes);
            if (_dvz_add_u64_overflows(payload_bytes, payload_size, &payload_bytes))
                return false;
        }
        else if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER ||
                 command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            return false;
        }
    }

    if (filter_phase && selected_count == 0)
        return true;

    uint64_t total_size = 0;
    if (_dvz_add_u64_overflows(DVZ_DRP2_PACKET_HEADER_SIZE, command_bytes, &total_size) ||
        total_size > SIZE_MAX || payload_bytes > SIZE_MAX)
        return false;

    uint8_t* bytes = (uint8_t*)dvz_calloc(1, (size_t)total_size);
    if (bytes == NULL)
        return false;
    uint8_t* payload = NULL;
    if (payload_bytes > 0)
    {
        payload = (uint8_t*)dvz_calloc(1, (size_t)payload_bytes);
        if (payload == NULL)
        {
            dvz_free(bytes);
            return false;
        }
    }

    memcpy(bytes, DVZ_DRP2_PACKET_MAGIC, 7);
    _put_u16(bytes + 8, DVZ_DRP2_PACKET_HEADER_SIZE);
    _put_u16(bytes + 10, DVZ_DRP2_PACKET_VERSION_MAJOR);
    _put_u16(bytes + 12, DVZ_DRP2_PACKET_VERSION_MINOR);
    _put_u16(bytes + 14, (uint16_t)kind);
    _put_u32(bytes + 16, 0);
    _put_u32(bytes + 20, selected_count);
    _put_u64(bytes + 24, command_bytes);
    _put_u64(bytes + 32, payload_bytes);
    _put_u64(bytes + 40, resource_version);
    _put_u64(bytes + 48, frame_index);

    uint64_t rec = DVZ_DRP2_PACKET_HEADER_SIZE;
    uint64_t payload_offset = 0;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        if (filter_phase && dvz_drp2_packet_command_kind(command->type) != kind)
            continue;
        const uint64_t body_size = _fixed_body_size(command->type);
        const uint64_t body_padded = _align8(body_size);
        const void* payload_ptr = NULL;
        uint64_t payload_size = 0;
        uint64_t command_payload_offset = DVZ_DRP2_PACKET_NO_PAYLOAD;
        if (_payload_info(command, &payload_ptr, &payload_size))
        {
            payload_offset = _align8(payload_offset);
            command_payload_offset = payload_offset;
            memcpy(payload + payload_offset, payload_ptr, (size_t)payload_size);
            payload_offset += payload_size;
        }

        _put_u32(bytes + rec + 0, (uint32_t)command->type);
        _put_u32(bytes + rec + 4, 0);
        _put_u32(bytes + rec + 8, (uint32_t)body_size);
        _put_u32(bytes + rec + 12, 0);
        _put_u64(bytes + rec + 16, command_payload_offset);
        _put_u64(bytes + rec + 24, payload_size);

        uint8_t scratch[sizeof(PacketShaderBody)] = {0};
        const void* body = _body_ptr(command, scratch);
        memcpy(bytes + rec + DVZ_DRP2_PACKET_RECORD_SIZE, body, (size_t)body_size);
        rec += DVZ_DRP2_PACKET_RECORD_SIZE + body_padded;
    }

    *packet = bytes;
    *packet_size = total_size;
    *arena = payload;
    *arena_size = payload_bytes;
    return true;
}





bool dvz_drp2_packet_encode_stream(
    const DvzDrp2CommandStream* stream, DvzDrp2PacketKind kind, uint64_t resource_version,
    uint64_t frame_index, void** packet, uint64_t* packet_size, void** arena,
    uint64_t* arena_size)
{
    return _packet_encode_stream_filtered(
        stream, kind, resource_version, frame_index, false, packet, packet_size, arena, arena_size);
}



bool dvz_drp2_packet_encode_stream_phase(
    const DvzDrp2CommandStream* stream, DvzDrp2PacketKind kind, uint64_t resource_version,
    uint64_t frame_index, void** packet, uint64_t* packet_size, void** arena,
    uint64_t* arena_size)
{
    return _packet_encode_stream_filtered(
        stream, kind, resource_version, frame_index, true, packet, packet_size, arena, arena_size);
}


/**
 * Decode a binary packet plus payload arena into a command stream.
 */
DvzDrp2CommandStream* dvz_drp2_packet_decode_stream(
    const void* packet, uint64_t packet_size, const void* arena, uint64_t arena_size,
    DvzDrp2PacketInfo* info)
{
    if (info != NULL)
        dvz_memset(info, sizeof(DvzDrp2PacketInfo), 0, sizeof(DvzDrp2PacketInfo));
    if (packet == NULL || packet_size < DVZ_DRP2_PACKET_HEADER_SIZE)
        return NULL;

    const uint8_t* bytes = (const uint8_t*)packet;
    if (memcmp(bytes, DVZ_DRP2_PACKET_MAGIC, 7) != 0 || bytes[7] != 0)
        return NULL;
    if (_get_u16(bytes + 8) != DVZ_DRP2_PACKET_HEADER_SIZE ||
        _get_u16(bytes + 10) != DVZ_DRP2_PACKET_VERSION_MAJOR || _get_u32(bytes + 16) != 0)
        return NULL;

    const DvzDrp2PacketKind kind = (DvzDrp2PacketKind)_get_u16(bytes + 14);
    if (kind == DVZ_DRP2_PACKET_NONE || kind > DVZ_DRP2_PACKET_FRAME)
        return NULL;
    const uint32_t command_count = _get_u32(bytes + 20);
    const uint64_t command_bytes = _get_u64(bytes + 24);
    const uint64_t packet_arena_size = _get_u64(bytes + 32);
    if (packet_arena_size > arena_size || (packet_arena_size > 0 && arena == NULL))
        return NULL;
    uint64_t expected_size = 0;
    if (_dvz_add_u64_overflows(DVZ_DRP2_PACKET_HEADER_SIZE, command_bytes, &expected_size) ||
        expected_size != packet_size)
        return NULL;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    if (stream == NULL)
        return NULL;
    if (!_ensure_decode_capacity(stream, command_count))
    {
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }

    const uint8_t* payload = (const uint8_t*)arena;
    uint64_t rec = DVZ_DRP2_PACKET_HEADER_SIZE;
    for (uint32_t i = 0; i < command_count; i++)
    {
        if (rec + DVZ_DRP2_PACKET_RECORD_SIZE > packet_size)
        {
            dvz_drp2_stream_destroy(stream);
            return NULL;
        }
        const DvzDrp2CommandType type = (DvzDrp2CommandType)_get_u32(bytes + rec + 0);
        const uint32_t record_flags = _get_u32(bytes + rec + 4);
        const uint32_t body_size = _get_u32(bytes + rec + 8);
        const uint64_t payload_offset = _get_u64(bytes + rec + 16);
        const uint64_t payload_size = _get_u64(bytes + rec + 24);
        const uint64_t body_padded = _align8(body_size);
        if (record_flags != 0 || body_size == 0 || rec + DVZ_DRP2_PACKET_RECORD_SIZE + body_padded >
                                                   packet_size)
        {
            dvz_drp2_stream_destroy(stream);
            return NULL;
        }
        if (payload_offset != DVZ_DRP2_PACKET_NO_PAYLOAD)
        {
            uint64_t payload_end = 0;
            if ((payload_offset & 7u) != 0 ||
                _dvz_add_u64_overflows(payload_offset, payload_size, &payload_end) ||
                payload_end > packet_arena_size)
            {
                dvz_drp2_stream_destroy(stream);
                return NULL;
            }
        }

        DvzDrp2Command* command = &stream->commands[i];
        dvz_memset(command, sizeof(DvzDrp2Command), 0, sizeof(DvzDrp2Command));
        command->type = type;
        const uint8_t* body = bytes + rec + DVZ_DRP2_PACKET_RECORD_SIZE;
        if (!_decode_body(command, body, body_size, payload, payload_offset, payload_size))
        {
            dvz_drp2_stream_destroy(stream);
            return NULL;
        }
        rec += DVZ_DRP2_PACKET_RECORD_SIZE + body_padded;
    }
    if (rec != packet_size)
    {
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    stream->count = command_count;

    if (info != NULL)
    {
        info->kind = kind;
        info->command_count = command_count;
        info->command_bytes = command_bytes;
        info->arena_size = packet_arena_size;
        info->resource_version = _get_u64(bytes + 40);
        info->frame_index = _get_u64(bytes + 48);
    }
    return stream;
}



/**
 * Destroy an encoded packet or arena buffer.
 */
void dvz_drp2_packet_destroy(void* ptr)
{
    dvz_free(ptr);
}
