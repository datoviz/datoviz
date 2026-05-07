/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command stream                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_stream.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_JSON_INITIAL_CAPACITY 4096



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct JsonBuilder JsonBuilder;

struct JsonBuilder
{
    char* data;
    uint64_t count;
    uint64_t capacity;
    bool failed;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _ensure_stream_capacity(DvzDrp2CommandStream* stream)
{
    ANN(stream);
    if (stream->commands == NULL || stream->capacity == 0)
    {
        stream->capacity = DVZ_DRP2_INITIAL_COMMAND_CAPACITY;
        stream->commands = (DvzDrp2Command*)dvz_calloc(stream->capacity, sizeof(DvzDrp2Command));
        return stream->commands != NULL;
    }

    if (stream->count < stream->capacity)
        return true;

    if (stream->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = stream->capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzDrp2Command), &bytes))
        return false;

    DvzDrp2Command* commands = (DvzDrp2Command*)dvz_realloc(stream->commands, bytes);
    if (commands == NULL)
        return false;

    stream->capacity = capacity;
    stream->commands = commands;
    return stream->commands != NULL;
}



static DvzDrp2Command* _append_command(DvzDrp2CommandStream* stream, DvzDrp2CommandType type)
{
    if (stream == NULL)
    {
        log_error("cannot append DRP2 command to a null stream");
        return NULL;
    }
    if (!_ensure_stream_capacity(stream))
    {
        log_error("cannot grow DRP2 command stream");
        return NULL;
    }

    DvzDrp2Command* command = &stream->commands[stream->count++];
    dvz_memset(command, sizeof(DvzDrp2Command), 0, sizeof(DvzDrp2Command));
    command->type = type;
    return command;
}



static void _copy_label(char* dst, uint64_t dst_size, const char* src)
{
    ANN(dst);
    ANN(src);
    dvz_strlcpy(dst, src, (size_t)dst_size);
}



static const char* _command_name(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        return "HelloRenderer";
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        return "RendererHelloReply";
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return "CreateBuffer";
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return "DestroyBuffer";
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return "CreateTexture";
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return "DestroyTexture";
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return "CreateShaderModule";
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return "DestroyShaderModule";
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return "CreateRenderPipeline";
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return "DestroyRenderPipeline";
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return "CreateComputePipeline";
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return "DestroyComputePipeline";
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return "CreateSampler";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return "CreateBindGroupLayout";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return "CreateBindGroup";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return "DestroyBindGroupLayout";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return "DestroyBindGroup";
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return "WriteBuffer";
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return "WriteTexture";
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return "BeginCommandEncoder";
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return "BeginRenderPass";
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return "BeginComputePass";
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return "SetPipeline";
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return "SetBindGroup";
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return "SetVertexBuffer";
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return "SetIndexBuffer";
    case DVZ_DRP2_COMMAND_DRAW:
        return "Draw";
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return "DrawIndexed";
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return "EndRenderPass";
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return "DispatchWorkgroups";
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return "EndComputePass";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return "CopyBufferToBuffer";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return "CopyBufferToTexture";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return "CopyTextureToBuffer";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return "CopyTextureToTexture";
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return "FinishCommandEncoder";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        return "QueueSubmit";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return "QueueSubmitReply";
    case DVZ_DRP2_COMMAND_NONE:
        return "None";
    default:
        return "None";
    }
}



static bool _json_init(JsonBuilder* builder)
{
    ANN(builder);
    builder->capacity = DVZ_DRP2_JSON_INITIAL_CAPACITY;
    builder->count = 0;
    builder->failed = false;
    builder->data = (char*)dvz_calloc(builder->capacity, sizeof(char));
    if (builder->data == NULL)
    {
        builder->failed = true;
        return false;
    }
    return true;
}



static bool _json_ensure(JsonBuilder* builder, uint64_t count)
{
    ANN(builder);
    if (builder->failed)
        return false;
    if (builder->data == NULL)
    {
        builder->failed = true;
        return false;
    }

    uint64_t required = 0;
    if (_dvz_add3_u64_overflows(builder->count, count, 1, &required))
    {
        builder->failed = true;
        return false;
    }

    if (required <= builder->capacity)
        return true;

    uint64_t capacity = builder->capacity;
    while (required > capacity)
    {
        if (capacity > UINT64_MAX / 2)
        {
            builder->failed = true;
            return false;
        }
        capacity *= 2;
    }

    char* data = (char*)dvz_realloc(builder->data, capacity);
    if (data == NULL)
    {
        builder->failed = true;
        return false;
    }
    builder->capacity = capacity;
    builder->data = data;
    return true;
}



static void _json_append(JsonBuilder* builder, const char* format, ...)
{
    ANN(builder);
    ANN(format);
    if (builder->failed)
        return;

    while (true)
    {
        if (builder->data == NULL || builder->count >= builder->capacity)
        {
            builder->failed = true;
            return;
        }

        uint64_t available = builder->capacity - builder->count;
        va_list args;
        va_start(args, format);
        int written = dvz_vsnprintf(
            builder->data + builder->count, (size_t)available, format, args);
        va_end(args);

        if (written < 0)
        {
            if (!_json_ensure(builder, builder->capacity))
                return;
            continue;
        }

        uint64_t written_u = (uint64_t)written;
        if (written_u < available)
        {
            builder->count += written_u;
            return;
        }
        if (!_json_ensure(builder, written_u))
            return;
    }
}


/**
 * Append a JSON string literal with minimal escaping.
 *
 * @param builder the JSON builder
 * @param string the string to append
 */
static void _json_append_escaped_string(JsonBuilder* builder, const char* string)
{
    ANN(builder);
    if (string == NULL)
        string = "";

    _json_append(builder, "\"");
    for (uint32_t i = 0; string[i] != '\0'; i++)
    {
        char c = string[i];
        if (c == '"' || c == '\\')
            _json_append(builder, "\\%c", c);
        else if (c == '\n')
            _json_append(builder, "\\n");
        else if (c == '\r')
            _json_append(builder, "\\r");
        else if (c == '\t')
            _json_append(builder, "\\t");
        else
            _json_append(builder, "%c", c);
    }
    _json_append(builder, "\"");
}



static void _json_append_usage(JsonBuilder* builder, uint32_t usage)
{
    ANN(builder);
    bool needs_comma = false;
    _json_append(builder, "[");
#define APPEND_USAGE(flag, label)                                                                \
    do                                                                                            \
    {                                                                                             \
        if ((usage & (flag)) != 0)                                                                \
        {                                                                                         \
            _json_append(builder, "%s\"%s\"", needs_comma ? ", " : "", (label));              \
            needs_comma = true;                                                                   \
        }                                                                                         \
    } while (0)

    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_COPY_SRC, "COPY_SRC");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_COPY_DST, "COPY_DST");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_MAP_READ, "MAP_READ");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_MAP_WRITE, "MAP_WRITE");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_VERTEX, "VERTEX");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_INDEX, "INDEX");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_UNIFORM, "UNIFORM");
    APPEND_USAGE(DVZ_DRP2_BUFFER_USAGE_STORAGE, "STORAGE");

#undef APPEND_USAGE
    _json_append(builder, "]");
}



static void _json_append_texture_usage(JsonBuilder* builder, uint32_t usage)
{
    ANN(builder);
    bool needs_comma = false;
    _json_append(builder, "[");
#define APPEND_TEXTURE_USAGE(flag, label)                                                         \
    do                                                                                            \
    {                                                                                             \
        if ((usage & (flag)) != 0)                                                                \
        {                                                                                         \
            _json_append(builder, "%s\"%s\"", needs_comma ? ", " : "", (label));              \
            needs_comma = true;                                                                   \
        }                                                                                         \
    } while (0)

    APPEND_TEXTURE_USAGE(DVZ_DRP2_TEXTURE_USAGE_COPY_SRC, "COPY_SRC");
    APPEND_TEXTURE_USAGE(DVZ_DRP2_TEXTURE_USAGE_COPY_DST, "COPY_DST");
    APPEND_TEXTURE_USAGE(DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING, "TEXTURE_BINDING");
    APPEND_TEXTURE_USAGE(DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING, "STORAGE_BINDING");
    APPEND_TEXTURE_USAGE(DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT, "RENDER_ATTACHMENT");

#undef APPEND_TEXTURE_USAGE
    _json_append(builder, "]");
}



static void _json_append_command(JsonBuilder* builder, const DvzDrp2Command* command)
{
    ANN(builder);
    ANN(command);

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"version\": { \"major\": 2, \"minor\": 0 }, "
            "\"client_name\": \"%s\" }",
            _command_name(command->type), command->u.handshake.name);
        break;
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"version\": { \"major\": 2, \"minor\": 0 }, "
            "\"status\": \"ok\", \"renderer_name\": \"%s\" }",
            _command_name(command->type), command->u.handshake.name);
        break;
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"size\": %" PRIu64 ", \"usage\": ",
            _command_name(command->type), command->u.create_buffer.id,
            command->u.create_buffer.size);
        _json_append_usage(builder, command->u.create_buffer.usage);
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"buffer_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.destroy_buffer.buffer_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64
            ", \"dimension\": \"%s\", \"width\": %" PRIu32 ", \"height\": %" PRIu32
            ", \"depth\": %" PRIu32 ", \"format\": \"rgba8unorm\", \"usage\": ",
            _command_name(command->type), command->u.create_texture.id,
            command->u.create_texture.depth > 1 ? "3d" : "2d",
            command->u.create_texture.width, command->u.create_texture.height,
            command->u.create_texture.depth > 0 ? command->u.create_texture.depth : 1);
        _json_append_texture_usage(builder, command->u.create_texture.usage);
        _json_append(builder, ", \"mip_level_count\": 1, \"sample_count\": 1 }");
        break;
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"texture_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.destroy_texture.texture_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"stage\": \"%s\", \"format\": \"%s\", "
            "\"entry_point\": \"main\", \"code\": ",
            _command_name(command->type), command->u.create_shader_module.id,
            command->u.create_shader_module.stage,
            command->u.create_shader_module.format[0] != '\0' ? command->u.create_shader_module.format
                                                               : "wgsl");
        _json_append_escaped_string(builder, command->u.create_shader_module.code);
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"shader_module_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.destroy_shader_module.shader_module_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"vertex_buffer_slots\": %" PRIu32
            ", \"vertex_shader_module_id\": %" PRIu64
            ", \"fragment_shader_module_id\": %" PRIu64,
            _command_name(command->type), command->u.create_render_pipeline.id,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.vertex_shader_module_id,
            command->u.create_render_pipeline.fragment_shader_module_id);
        if (command->u.create_render_pipeline.bind_group_layout_id != 0)
        {
            _json_append(
                builder, ", \"bind_group_layout_ids\": [%" PRIu64 "]",
                command->u.create_render_pipeline.bind_group_layout_id);
        }
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"render_pipeline_id\": %" PRIu64 " }",
            _command_name(command->type),
            command->u.destroy_render_pipeline.render_pipeline_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"compute_shader_module_id\": %" PRIu64,
            _command_name(command->type), command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id);
        if (command->u.create_compute_pipeline.bind_group_layout_id != 0)
        {
            _json_append(
                builder, ", \"bind_group_layout_ids\": [%" PRIu64 "]",
                command->u.create_compute_pipeline.bind_group_layout_id);
        }
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"compute_pipeline_id\": %" PRIu64 " }",
            _command_name(command->type),
            command->u.destroy_compute_pipeline.compute_pipeline_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64
            ", \"mag_filter\": \"linear\", \"min_filter\": \"linear\", "
            "\"mipmap_filter\": \"nearest\", \"address_mode_u\": \"clamp-to-edge\", "
            "\"address_mode_v\": \"clamp-to-edge\" }",
            _command_name(command->type), command->u.create_sampler.id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        if (command->u.create_bind_group_layout.storage_buffers)
        {
            _json_append(
                builder,
                "{ \"cmd\": \"%s\", \"id\": %" PRIu64
                ", \"entries\": [ { \"binding\": 0, \"binding_type\": \"storage_buffer\" }, "
                "{ \"binding\": 1, \"binding_type\": \"storage_buffer\" } ] }",
                _command_name(command->type), command->u.create_bind_group_layout.id);
        }
        else
        {
            _json_append(
                builder,
                "{ \"cmd\": \"%s\", \"id\": %" PRIu64
                ", \"entries\": [ { \"binding\": 0, \"binding_type\": \"sampled_texture\" }, "
                "{ \"binding\": 1, \"binding_type\": \"sampler\" } ] }",
                _command_name(command->type), command->u.create_bind_group_layout.id);
        }
        break;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        if (command->u.create_bind_group.buffer0_id != 0 ||
            command->u.create_bind_group.buffer1_id != 0)
        {
            _json_append(
                builder,
                "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"bind_group_layout_id\": %" PRIu64
                ", \"entries\": [ { \"binding\": 0, \"binding_type\": \"storage_buffer\", "
                "\"resource_kind\": \"buffer\", \"resource_id\": %" PRIu64
                ", \"offset\": 0, \"size\": %" PRIu64
                " }, { \"binding\": 1, \"binding_type\": \"storage_buffer\", "
                "\"resource_kind\": \"buffer\", \"resource_id\": %" PRIu64
                ", \"offset\": 0, \"size\": %" PRIu64 " } ] }",
                _command_name(command->type), command->u.create_bind_group.id,
                command->u.create_bind_group.bind_group_layout_id,
                command->u.create_bind_group.buffer0_id, command->u.create_bind_group.buffer_size,
                command->u.create_bind_group.buffer1_id, command->u.create_bind_group.buffer_size);
        }
        else
        {
            _json_append(
                builder,
                "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"bind_group_layout_id\": %" PRIu64
                ", \"entries\": [ { \"binding\": 0, \"binding_type\": \"sampled_texture\", "
                "\"resource_kind\": \"texture\", \"resource_id\": %" PRIu64
                " }, { \"binding\": 1, \"binding_type\": \"sampler\", "
                "\"resource_kind\": \"sampler\", \"resource_id\": %" PRIu64 " } ] }",
                _command_name(command->type), command->u.create_bind_group.id,
                command->u.create_bind_group.bind_group_layout_id,
                command->u.create_bind_group.texture_id, command->u.create_bind_group.sampler_id);
        }
        break;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"bind_group_layout_id\": %" PRIu64 " }",
            _command_name(command->type),
            command->u.destroy_bind_group_layout.bind_group_layout_id);
        break;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"bind_group_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.destroy_bind_group.bind_group_id);
        break;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    {
        const char* b64_wb = command->u.write_buffer.data_base64;
        char* b64_wb_tmp   = NULL;
        if (b64_wb == NULL && command->u.write_buffer.data_raw != NULL)
        {
            uint64_t enc_len = _dvz_b64_encoded_len(command->u.write_buffer.size) + 1;
            b64_wb_tmp = (char*)dvz_calloc((size_t)enc_len, 1);
            if (b64_wb_tmp)
                _dvz_b64_encode(
                    (const uint8_t*)command->u.write_buffer.data_raw,
                    command->u.write_buffer.size, b64_wb_tmp, enc_len);
            b64_wb = b64_wb_tmp;
        }
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"buffer_id\": %" PRIu64 ", \"offset\": %" PRIu64
            ", \"size\": %" PRIu64 ", \"data\": \"%s\" }",
            _command_name(command->type), command->u.write_buffer.buffer_id,
            command->u.write_buffer.offset, command->u.write_buffer.size,
            b64_wb ? b64_wb : "");
        dvz_free(b64_wb_tmp);
        break;
    }
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    {
        uint64_t tex_size = (uint64_t)command->u.write_texture.depth *
                            command->u.write_texture.rows_per_image *
                            command->u.write_texture.bytes_per_row;
        const char* b64_wt = command->u.write_texture.data_base64;
        char* b64_wt_tmp   = NULL;
        if (b64_wt == NULL && command->u.write_texture.data_raw != NULL)
        {
            uint64_t enc_len = _dvz_b64_encoded_len(tex_size) + 1;
            b64_wt_tmp = (char*)dvz_calloc((size_t)enc_len, 1);
            if (b64_wt_tmp)
                _dvz_b64_encode(
                    (const uint8_t*)command->u.write_texture.data_raw, tex_size, b64_wt_tmp,
                    enc_len);
            b64_wt = b64_wt_tmp;
        }
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"texture_id\": %" PRIu64 ", \"mip_level\": %" PRIu32
            ", \"origin\": { \"x\": %" PRIu32 ", \"y\": %" PRIu32 ", \"z\": %" PRIu32
            " }, \"size\": { \"width\": %" PRIu32 ", \"height\": %" PRIu32
            ", \"depth\": %" PRIu32 " }, \"bytes_per_row\": %" PRIu32
            ", \"rows_per_image\": %" PRIu32 ", \"data\": \"%s\" }",
            _command_name(command->type), command->u.write_texture.texture_id,
            command->u.write_texture.mip_level, command->u.write_texture.origin_x,
            command->u.write_texture.origin_y, command->u.write_texture.origin_z,
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
            command->u.write_texture.rows_per_image, b64_wt ? b64_wt : "");
        dvz_free(b64_wt_tmp);
        break;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"id\": %" PRIu64 " }", _command_name(command->type),
            command->u.begin_command_encoder.id);
        break;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"encoder_id\": %" PRIu64
            ", \"color_attachments\": [ { \"texture_id\": %" PRIu64
            ", \"load_op\": \"clear\", \"store_op\": \"store\", "
            "\"clear_value\": { \"r\": %g, \"g\": %g, \"b\": %g, \"a\": %g } } ] }",
            _command_name(command->type), command->u.begin_render_pass.id,
            command->u.begin_render_pass.encoder_id, command->u.begin_render_pass.texture_id,
            (double)command->u.begin_render_pass.clear_color[0],
            (double)command->u.begin_render_pass.clear_color[1],
            (double)command->u.begin_render_pass.clear_color[2],
            (double)command->u.begin_render_pass.clear_color[3]);
        break;
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"encoder_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.begin_compute_pass.id,
            command->u.begin_compute_pass.encoder_id);
        break;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"pipeline_id\": %" PRIu64
                     " }",
            _command_name(command->type), command->u.set_pipeline.pass_id,
            command->u.set_pipeline.pipeline_id);
        break;
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"slot\": %" PRIu32
            ", \"bind_group_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.set_bind_group.pass_id,
            command->u.set_bind_group.slot, command->u.set_bind_group.bind_group_id);
        break;
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"slot\": %" PRIu32
            ", \"buffer_id\": %" PRIu64 ", \"offset\": %" PRIu64 " }",
            _command_name(command->type), command->u.set_vertex_buffer.pass_id,
            command->u.set_vertex_buffer.slot, command->u.set_vertex_buffer.buffer_id,
            command->u.set_vertex_buffer.offset);
        break;
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"buffer_id\": %" PRIu64
            ", \"index_format\": \"%s\", \"offset\": %" PRIu64 " }",
            _command_name(command->type), command->u.set_index_buffer.pass_id,
            command->u.set_index_buffer.buffer_id, command->u.set_index_buffer.index_format,
            command->u.set_index_buffer.offset);
        break;
    case DVZ_DRP2_COMMAND_DRAW:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"vertex_count\": %" PRIu32
            ", \"instance_count\": %" PRIu32 ", \"first_vertex\": %" PRIu32
            ", \"first_instance\": %" PRIu32 " }",
            _command_name(command->type), command->u.draw.pass_id, command->u.draw.vertex_count,
            command->u.draw.instance_count, command->u.draw.first_vertex,
            command->u.draw.first_instance);
        break;
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"index_count\": %" PRIu32
            ", \"instance_count\": %" PRIu32 ", \"first_index\": %" PRIu32
            ", \"base_vertex\": %" PRId32 ", \"first_instance\": %" PRIu32 " }",
            _command_name(command->type), command->u.draw_indexed.pass_id,
            command->u.draw_indexed.index_count, command->u.draw_indexed.instance_count,
            command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex,
            command->u.draw_indexed.first_instance);
        break;
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.end_render_pass.pass_id);
        break;
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"x\": %" PRIu32
            ", \"y\": %" PRIu32 ", \"z\": %" PRIu32 " }",
            _command_name(command->type), command->u.dispatch.pass_id, command->u.dispatch.x,
            command->u.dispatch.y, command->u.dispatch.z);
        break;
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.end_compute_pass.pass_id);
        break;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"encoder_id\": %" PRIu64 ", \"src_buffer_id\": %" PRIu64
            ", \"src_offset\": %" PRIu64 ", \"dst_buffer_id\": %" PRIu64
            ", \"dst_offset\": %" PRIu64 ", \"size\": %" PRIu64 " }",
            _command_name(command->type), command->u.copy_buffer_to_buffer.encoder_id,
            command->u.copy_buffer_to_buffer.src_buffer_id,
            command->u.copy_buffer_to_buffer.src_offset,
            command->u.copy_buffer_to_buffer.dst_buffer_id,
            command->u.copy_buffer_to_buffer.dst_offset, command->u.copy_buffer_to_buffer.size);
        break;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"encoder_id\": %" PRIu64 ", \"src_buffer_id\": %" PRIu64
            ", \"src_offset\": %" PRIu64 ", \"bytes_per_row\": %" PRIu32
            ", \"rows_per_image\": %" PRIu32 ", \"dst_texture_id\": %" PRIu64
            ", \"dst_mip_level\": %" PRIu32 ", \"dst_origin\": { \"x\": %" PRIu32
            ", \"y\": %" PRIu32 ", \"z\": %" PRIu32 " }, \"size\": { \"width\": %" PRIu32
            ", \"height\": %" PRIu32 ", \"depth\": %" PRIu32 " } }",
            _command_name(command->type), command->u.copy_buffer_to_texture.encoder_id,
            command->u.copy_buffer_to_texture.src_buffer_id,
            command->u.copy_buffer_to_texture.src_offset,
            command->u.copy_buffer_to_texture.bytes_per_row,
            command->u.copy_buffer_to_texture.rows_per_image,
            command->u.copy_buffer_to_texture.dst_texture_id,
            command->u.copy_buffer_to_texture.dst_mip_level,
            command->u.copy_buffer_to_texture.dst_origin_x,
            command->u.copy_buffer_to_texture.dst_origin_y,
            command->u.copy_buffer_to_texture.dst_origin_z,
            command->u.copy_buffer_to_texture.width, command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth);
        break;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"encoder_id\": %" PRIu64 ", \"src_texture_id\": %" PRIu64
            ", \"src_mip_level\": 0, \"src_origin\": { \"x\": 0, \"y\": 0, \"z\": 0 }, "
            "\"size\": { \"width\": %" PRIu32 ", \"height\": %" PRIu32
            ", \"depth\": 1 }, \"dst_buffer_id\": %" PRIu64 ", \"dst_offset\": %" PRIu64
            ", \"bytes_per_row\": %" PRIu32 ", \"rows_per_image\": %" PRIu32 " }",
            _command_name(command->type), command->u.copy_texture_to_buffer.encoder_id,
            command->u.copy_texture_to_buffer.src_texture_id,
            command->u.copy_texture_to_buffer.width, command->u.copy_texture_to_buffer.height,
            command->u.copy_texture_to_buffer.dst_buffer_id,
            command->u.copy_texture_to_buffer.dst_offset,
            command->u.copy_texture_to_buffer.bytes_per_row,
            command->u.copy_texture_to_buffer.rows_per_image);
        break;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"encoder_id\": %" PRIu64 ", \"src_texture_id\": %" PRIu64
            ", \"src_mip_level\": %" PRIu32 ", \"src_origin\": { \"x\": %" PRIu32
            ", \"y\": %" PRIu32 ", \"z\": %" PRIu32 " }, \"dst_texture_id\": %" PRIu64
            ", \"dst_mip_level\": %" PRIu32 ", \"dst_origin\": { \"x\": %" PRIu32
            ", \"y\": %" PRIu32 ", \"z\": %" PRIu32 " }, \"size\": { \"width\": %" PRIu32
            ", \"height\": %" PRIu32 ", \"depth\": %" PRIu32 " } }",
            _command_name(command->type), command->u.copy_texture_to_texture.encoder_id,
            command->u.copy_texture_to_texture.src_texture_id,
            command->u.copy_texture_to_texture.src_mip_level,
            command->u.copy_texture_to_texture.src_origin_x,
            command->u.copy_texture_to_texture.src_origin_y,
            command->u.copy_texture_to_texture.src_origin_z,
            command->u.copy_texture_to_texture.dst_texture_id,
            command->u.copy_texture_to_texture.dst_mip_level,
            command->u.copy_texture_to_texture.dst_origin_x,
            command->u.copy_texture_to_texture.dst_origin_y,
            command->u.copy_texture_to_texture.dst_origin_z,
            command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth);
        break;
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"encoder_id\": %" PRIu64
                     ", \"command_buffer_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.finish_command_encoder.encoder_id,
            command->u.finish_command_encoder.command_buffer_id);
        break;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"command_buffer_ids\": [%" PRIu64
                     "], \"submission_id\": %" PRIu64,
            _command_name(command->type), command->u.queue_submit.command_buffer_id,
            command->u.queue_submit.submission_id);
        if (command->u.queue_submit.has_readback)
        {
            _json_append(
                builder,
                ", \"readbacks\": [ { \"buffer_id\": %" PRIu64 ", \"offset\": %" PRIu64
                ", \"size\": %" PRIu64 " } ]",
                command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
                command->u.queue_submit.size);
        }
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"submission_id\": %" PRIu64 ", \"readbacks\": [ { "
            "\"buffer_id\": %" PRIu64 ", \"offset\": %" PRIu64 ", \"size\": %" PRIu64
            ", \"data\": \"%s\" } ] }",
            _command_name(command->type), command->u.queue_submit.submission_id,
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size, command->u.queue_submit.data_base64);
        break;
    case DVZ_DRP2_COMMAND_NONE:
        _json_append(builder, "{ \"cmd\": \"None\" }");
        break;
    default:
        _json_append(builder, "{ \"cmd\": \"None\" }");
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an empty DRP2 command stream.
 *
 * @return the command stream
 */
DvzDrp2CommandStream* dvz_drp2_stream(void)
{
    DvzDrp2CommandStream* stream = (DvzDrp2CommandStream*)dvz_calloc(
        1, sizeof(DvzDrp2CommandStream));
    if (stream == NULL)
        return NULL;
    stream->capacity = DVZ_DRP2_INITIAL_COMMAND_CAPACITY;
    stream->commands = (DvzDrp2Command*)dvz_calloc(stream->capacity, sizeof(DvzDrp2Command));
    if (stream->commands == NULL)
    {
        dvz_free(stream);
        return NULL;
    }
    return stream;
}



/**
 * Destroy a DRP2 command stream.
 *
 * @param stream the command stream
 */
void dvz_drp2_stream_destroy(DvzDrp2CommandStream* stream)
{
    if (stream == NULL)
        return;
    if (stream->owner_release != NULL && !stream->owner_released)
    {
        stream->owner_release(stream->owner);
        stream->owner_released = true;
    }
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* cmd = &stream->commands[i];
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
            dvz_free(cmd->u.write_buffer.data_base64);
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            dvz_free(cmd->u.write_texture.data_base64);
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
            dvz_free(cmd->u.create_shader_module.code);
    }
    dvz_free(stream->commands);
    dvz_free(stream);
}



/**
 * Return the number of commands in a DRP2 command stream.
 *
 * @param stream the command stream
 * @return the number of commands
 */
uint32_t dvz_drp2_stream_count(const DvzDrp2CommandStream* stream)
{
    if (stream == NULL)
        return 0;
    return stream->count;
}



/**
 * Return a command from a DRP2 command stream.
 *
 * @param stream the command stream
 * @param index the command index
 * @return the command, or NULL when index is out of bounds
 */
const DvzDrp2Command* dvz_drp2_stream_get(const DvzDrp2CommandStream* stream, uint32_t index)
{
    if (stream == NULL || index >= stream->count)
        return NULL;
    return &stream->commands[index];
}



/**
 * Return a command type.
 *
 * @param command the command
 * @return the command type
 */
DvzDrp2CommandType dvz_drp2_command_type(const DvzDrp2Command* command)
{
    if (command == NULL)
        return DVZ_DRP2_COMMAND_NONE;
    return command->type;
}



/**
 * Append a HelloRenderer command.
 *
 * @param stream the command stream
 * @param client_name the client name
 * @return whether the command was appended
 */
bool dvz_drp2_stream_hello_renderer(DvzDrp2CommandStream* stream, const char* client_name)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_HELLO_RENDERER);
    if (command == NULL)
        return false;
    _copy_label(command->u.handshake.name, DVZ_DRP2_LABEL_SIZE, client_name ? client_name : "");
    return true;
}



/**
 * Append a RendererHelloReply command.
 *
 * @param stream the command stream
 * @param renderer_name the renderer name
 * @return whether the command was appended
 */
bool dvz_drp2_stream_renderer_hello_reply(DvzDrp2CommandStream* stream, const char* renderer_name)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY);
    if (command == NULL)
        return false;
    _copy_label(command->u.handshake.name, DVZ_DRP2_LABEL_SIZE, renderer_name ? renderer_name : "");
    return true;
}



/**
 * Append a CreateBuffer command.
 *
 * @param stream the command stream
 * @param id the buffer id
 * @param size the buffer size in bytes
 * @param usage buffer usage flags
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_buffer(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t size, uint32_t usage)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BUFFER);
    if (command == NULL)
        return false;
    command->u.create_buffer.id = id;
    command->u.create_buffer.size = size;
    command->u.create_buffer.usage = usage;
    return true;
}



/**
 * Append a DestroyBuffer command.
 *
 * @param stream the command stream
 * @param buffer_id the buffer id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_buffer(DvzDrp2CommandStream* stream, uint64_t buffer_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_BUFFER);
    if (command == NULL)
        return false;
    command->u.destroy_buffer.buffer_id = buffer_id;
    return true;
}



/**
 * Append a CreateTexture command for a 2D render attachment.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_2d(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height)
{
    return dvz_drp2_stream_create_texture_2d_usage(
        stream, id, width, height,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC |
            DVZ_DRP2_TEXTURE_USAGE_COPY_DST);
}



/**
 * Append a CreateTexture command for a 2D texture with explicit usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param usage texture usage flags
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_2d_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t usage)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    if (command == NULL)
        return false;
    command->u.create_texture.id = id;
    command->u.create_texture.width = width;
    command->u.create_texture.height = height;
    command->u.create_texture.depth = 1;
    command->u.create_texture.usage = usage;
    return true;
}



/**
 * Append a DestroyTexture command.
 *
 * @param stream the command stream
 * @param texture_id the texture id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_texture(DvzDrp2CommandStream* stream, uint64_t texture_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_TEXTURE);
    if (command == NULL)
        return false;
    command->u.destroy_texture.texture_id = texture_id;
    return true;
}



/**
 * Append a CreateShaderModule command.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage
 * @param code the WGSL shader source
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_shader_module(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* code)
{
    return dvz_drp2_stream_create_shader_module_format(stream, id, stage, "wgsl", code);
}



/**
 * Append a CreateShaderModule command with an explicit shader source format.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage
 * @param format the shader source format
 * @param code the shader source
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_shader_module_format(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* format,
    const char* code)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    if (command == NULL)
        return false;

    const char* src = code ? code : "";
    size_t n = strlen(src) + 1;
    char* code_copy = (char*)dvz_malloc(n);
    if (code_copy == NULL)
    {
        stream->count--;
        return false;
    }
    memcpy(code_copy, src, n);

    command->u.create_shader_module.id = id;
    _copy_label(command->u.create_shader_module.stage, DVZ_DRP2_LABEL_SIZE, stage ? stage : "");
    _copy_label(
        command->u.create_shader_module.format, DVZ_DRP2_LABEL_SIZE,
        format != NULL && format[0] != '\0' ? format : "wgsl");
    command->u.create_shader_module.code = code_copy;
    return true;
}



/**
 * Append a DestroyShaderModule command.
 *
 * @param stream the command stream
 * @param shader_module_id the shader module id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_shader_module(
    DvzDrp2CommandStream* stream, uint64_t shader_module_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE);
    if (command == NULL)
        return false;
    command->u.destroy_shader_module.shader_module_id = shader_module_id;
    return true;
}



/**
 * Append a CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_render_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots)
{
    return dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, id, vertex_shader_module_id, fragment_shader_module_id, vertex_buffer_slots, 0);
}



/**
 * Append a CreateRenderPipeline command with one bind-group layout.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @param bind_group_layout_id the bind-group layout id for slot 0
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots, uint64_t bind_group_layout_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    if (command == NULL)
        return false;
    command->u.create_render_pipeline.id = id;
    command->u.create_render_pipeline.vertex_shader_module_id = vertex_shader_module_id;
    command->u.create_render_pipeline.fragment_shader_module_id = fragment_shader_module_id;
    command->u.create_render_pipeline.vertex_buffer_slots = vertex_buffer_slots;
    command->u.create_render_pipeline.bind_group_layout_id = bind_group_layout_id;
    return true;
}



/**
 * Append a CreateRenderPipeline command with explicit vertex input layout and topology.
 */
bool dvz_drp2_stream_create_render_pipeline_ex(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint32_t topology,
    uint32_t binding_count, const uint32_t* binding_strides,
    uint32_t attr_count, const uint32_t* attr_bindings, const uint32_t* attr_locations,
    const uint32_t* attr_formats, const uint32_t* attr_offsets)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    if (command == NULL)
        return false;
    command->u.create_render_pipeline.id = id;
    command->u.create_render_pipeline.vertex_shader_module_id = vertex_shader_module_id;
    command->u.create_render_pipeline.fragment_shader_module_id = fragment_shader_module_id;
    command->u.create_render_pipeline.vertex_buffer_slots = vertex_buffer_slots;
    command->u.create_render_pipeline.bind_group_layout_id = 0;
    command->u.create_render_pipeline.topology = topology;
    uint32_t nb = binding_count < 16 ? binding_count : 16;
    command->u.create_render_pipeline.binding_count = nb;
    for (uint32_t i = 0; i < nb; i++)
        command->u.create_render_pipeline.binding_strides[i] = binding_strides[i];
    uint32_t na = attr_count < 16 ? attr_count : 16;
    command->u.create_render_pipeline.attr_count = na;
    for (uint32_t i = 0; i < na; i++)
    {
        command->u.create_render_pipeline.attr_bindings[i]  = attr_bindings[i];
        command->u.create_render_pipeline.attr_locations[i] = attr_locations[i];
        command->u.create_render_pipeline.attr_formats[i]   = attr_formats[i];
        command->u.create_render_pipeline.attr_offsets[i]   = attr_offsets[i];
    }
    return true;
}



/**
 * Append a DestroyRenderPipeline command.
 *
 * @param stream the command stream
 * @param render_pipeline_id the render pipeline id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_render_pipeline(
    DvzDrp2CommandStream* stream, uint64_t render_pipeline_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE);
    if (command == NULL)
        return false;
    command->u.destroy_render_pipeline.render_pipeline_id = render_pipeline_id;
    return true;
}



/**
 * Append a CreateComputePipeline command.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param compute_shader_module_id the compute shader module id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_compute_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t compute_shader_module_id)
{
    return dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
        stream, id, compute_shader_module_id, 0);
}



/**
 * Append a CreateComputePipeline command with one bind-group layout.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param compute_shader_module_id the compute shader module id
 * @param bind_group_layout_id the bind-group layout id for slot 0
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t compute_shader_module_id,
    uint64_t bind_group_layout_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    if (command == NULL)
        return false;
    command->u.create_compute_pipeline.id = id;
    command->u.create_compute_pipeline.compute_shader_module_id = compute_shader_module_id;
    command->u.create_compute_pipeline.bind_group_layout_id = bind_group_layout_id;
    return true;
}



/**
 * Append a DestroyComputePipeline command.
 *
 * @param stream the command stream
 * @param compute_pipeline_id the compute pipeline id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_compute_pipeline(
    DvzDrp2CommandStream* stream, uint64_t compute_pipeline_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE);
    if (command == NULL)
        return false;
    command->u.destroy_compute_pipeline.compute_pipeline_id = compute_pipeline_id;
    return true;
}



/**
 * Append a CreateSampler command.
 *
 * @param stream the command stream
 * @param id the sampler id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_sampler(DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    if (command == NULL)
        return false;
    command->u.create_sampler.id = id;
    return true;
}



/**
 * Append a CreateBindGroupLayout command for one sampled texture and one sampler.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_sampler_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    if (command == NULL)
        return false;
    command->u.create_bind_group_layout.id = id;
    return true;
}



/**
 * Append a CreateBindGroupLayout command for two storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_storage_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    if (command == NULL)
        return false;
    command->u.create_bind_group_layout.id = id;
    command->u.create_bind_group_layout.storage_buffers = true;
    return true;
}



/**
 * Append a CreateBindGroup command for one sampled texture and one sampler.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param texture_id the sampled texture id
 * @param sampler_id the sampler id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_sampler_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t texture_id,
    uint64_t sampler_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.create_bind_group.id = id;
    command->u.create_bind_group.bind_group_layout_id = bind_group_layout_id;
    command->u.create_bind_group.texture_id = texture_id;
    command->u.create_bind_group.sampler_id = sampler_id;
    return true;
}



/**
 * Append a CreateBindGroup command for two storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param buffer0_id the first storage buffer id
 * @param buffer1_id the second storage buffer id
 * @param buffer_size the bound range size for each buffer
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_storage_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t buffer0_id,
    uint64_t buffer1_id, uint64_t buffer_size)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.create_bind_group.id = id;
    command->u.create_bind_group.bind_group_layout_id = bind_group_layout_id;
    command->u.create_bind_group.buffer0_id = buffer0_id;
    command->u.create_bind_group.buffer1_id = buffer1_id;
    command->u.create_bind_group.buffer_size = buffer_size;
    return true;
}



/**
 * Append a DestroyBindGroupLayout command.
 *
 * @param stream the command stream
 * @param bind_group_layout_id the bind-group layout id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT);
    if (command == NULL)
        return false;
    command->u.destroy_bind_group_layout.bind_group_layout_id = bind_group_layout_id;
    return true;
}



/**
 * Append a DestroyBindGroup command.
 *
 * @param stream the command stream
 * @param bind_group_id the bind-group id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_destroy_bind_group(DvzDrp2CommandStream* stream, uint64_t bind_group_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.destroy_bind_group.bind_group_id = bind_group_id;
    return true;
}



/**
 * Append a WriteBuffer command.
 *
 * @param stream the command stream
 * @param buffer_id the buffer id
 * @param offset the byte offset
 * @param size the payload size in bytes
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
bool dvz_drp2_stream_write_buffer(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const char* data_base64)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_BUFFER);
    if (command == NULL)
        return false;

    const char* src = data_base64 ? data_base64 : "";
    size_t n = strlen(src) + 1;
    char* buf = (char*)dvz_malloc(n);
    if (buf == NULL)
    {
        /* Roll back: leaving a half-built WriteBuffer command in the stream would
           crash the runtime when both data_raw and data_base64 are NULL. */
        stream->count--;
        return false;
    }
    memcpy(buf, src, n);

    command->u.write_buffer.buffer_id   = buffer_id;
    command->u.write_buffer.offset      = offset;
    command->u.write_buffer.size        = size;
    command->u.write_buffer.data_base64 = buf;
    return true;
}



/**
 * Append a WriteTexture command.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param width the written width
 * @param height the written height
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
bool dvz_drp2_stream_write_texture_2d(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    if (command == NULL)
        return false;

    const char* src = data_base64 ? data_base64 : "";
    size_t n = strlen(src) + 1;
    char* buf = (char*)dvz_malloc(n);
    if (buf == NULL)
    {
        stream->count--;
        return false;
    }
    memcpy(buf, src, n);

    command->u.write_texture.texture_id     = texture_id;
    command->u.write_texture.mip_level      = mip_level;
    command->u.write_texture.width          = width;
    command->u.write_texture.height         = height;
    command->u.write_texture.depth          = 1;
    command->u.write_texture.bytes_per_row  = bytes_per_row;
    command->u.write_texture.rows_per_image = rows_per_image;
    command->u.write_texture.data_base64    = buf;
    return true;
}



bool dvz_drp2_stream_write_texture_2d_region(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y,
    uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    if (command == NULL)
        return false;

    const char* src = data_base64 ? data_base64 : "";
    size_t n = strlen(src) + 1;
    char* buf = (char*)dvz_malloc(n);
    if (buf == NULL)
    {
        stream->count--;
        return false;
    }
    memcpy(buf, src, n);

    command->u.write_texture.texture_id     = texture_id;
    command->u.write_texture.mip_level      = mip_level;
    command->u.write_texture.origin_x       = origin_x;
    command->u.write_texture.origin_y       = origin_y;
    command->u.write_texture.origin_z       = 0;
    command->u.write_texture.width          = width;
    command->u.write_texture.height         = height;
    command->u.write_texture.depth          = 1;
    command->u.write_texture.bytes_per_row  = bytes_per_row;
    command->u.write_texture.rows_per_image = rows_per_image;
    command->u.write_texture.data_base64    = buf;
    return true;
}



bool dvz_drp2_stream_create_texture_3d(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    if (command == NULL)
        return false;
    command->u.create_texture.id    = id;
    command->u.create_texture.width = width;
    command->u.create_texture.height = height;
    command->u.create_texture.depth = depth;
    command->u.create_texture.usage =
        DVZ_DRP2_TEXTURE_USAGE_COPY_SRC | DVZ_DRP2_TEXTURE_USAGE_COPY_DST |
        DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    return true;
}



bool dvz_drp2_stream_write_texture_3d(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    if (command == NULL)
        return false;

    const char* src = data_base64 ? data_base64 : "";
    size_t n = strlen(src) + 1;
    char* buf = (char*)dvz_malloc(n);
    if (buf == NULL)
    {
        stream->count--;
        return false;
    }
    memcpy(buf, src, n);

    command->u.write_texture.texture_id     = texture_id;
    command->u.write_texture.mip_level      = mip_level;
    command->u.write_texture.origin_x       = origin_x;
    command->u.write_texture.origin_y       = origin_y;
    command->u.write_texture.origin_z       = origin_z;
    command->u.write_texture.width          = width;
    command->u.write_texture.height         = height;
    command->u.write_texture.depth          = depth;
    command->u.write_texture.bytes_per_row  = bytes_per_row;
    command->u.write_texture.rows_per_image = rows_per_image;
    command->u.write_texture.data_base64    = buf;
    return true;
}



/**
 * Append a BeginCommandEncoder command.
 *
 * @param stream the command stream
 * @param id the encoder id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_begin_command_encoder(DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    if (command == NULL)
        return false;
    command->u.begin_command_encoder.id = id;
    return true;
}



/**
 * Append a BeginRenderPass command with one color texture attachment.
 *
 * @param stream the command stream
 * @param id the render pass id
 * @param encoder_id the encoder id
 * @param texture_id the color attachment texture id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_begin_render_pass(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id)
{
    return dvz_drp2_stream_begin_render_pass_region_clear(
        stream, id, encoder_id, texture_id, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        false);
}



/**
 * Append a BeginRenderPass command for a normalized sub-region.
 *
 * @param stream the command stream
 * @param id the render pass id
 * @param encoder_id the encoder id
 * @param texture_id the color attachment texture id
 * @param r clear color red channel
 * @param g clear color green channel
 * @param b clear color blue channel
 * @param a clear color alpha channel
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @param clear whether to clear the target at pass begin
 * @return whether the command was appended
 */
bool dvz_drp2_stream_begin_render_pass_region_clear(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id,
    float r, float g, float b, float a, float x, float y, float width, float height, bool clear)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    if (command == NULL)
        return false;
    command->u.begin_render_pass.id = id;
    command->u.begin_render_pass.encoder_id = encoder_id;
    command->u.begin_render_pass.texture_id = texture_id;
    command->u.begin_render_pass.clear_color[0] = r;
    command->u.begin_render_pass.clear_color[1] = g;
    command->u.begin_render_pass.clear_color[2] = b;
    command->u.begin_render_pass.clear_color[3] = a;
    command->u.begin_render_pass.viewport[0] = x;
    command->u.begin_render_pass.viewport[1] = y;
    command->u.begin_render_pass.viewport[2] = width;
    command->u.begin_render_pass.viewport[3] = height;
    command->u.begin_render_pass.clear = clear;
    return true;
}



bool dvz_drp2_stream_begin_render_pass_clear(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id,
    float r, float g, float b, float a)
{
    return dvz_drp2_stream_begin_render_pass_region_clear(
        stream, id, encoder_id, texture_id, r, g, b, a, 0.0f, 0.0f, 1.0f, 1.0f, true);
}



/**
 * Append a BeginComputePass command.
 *
 * @param stream the command stream
 * @param id the compute pass id
 * @param encoder_id the encoder id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_begin_compute_pass(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    if (command == NULL)
        return false;
    command->u.begin_compute_pass.id = id;
    command->u.begin_compute_pass.encoder_id = encoder_id;
    return true;
}



/**
 * Append a SetPipeline command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param pipeline_id the pipeline id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_pipeline(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint64_t pipeline_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_PIPELINE);
    if (command == NULL)
        return false;
    command->u.set_pipeline.pass_id = pass_id;
    command->u.set_pipeline.pipeline_id = pipeline_id;
    return true;
}



/**
 * Append a SetBindGroup command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param slot the bind-group slot
 * @param bind_group_id the bind-group id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_bind_group(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t bind_group_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.set_bind_group.pass_id = pass_id;
    command->u.set_bind_group.slot = slot;
    command->u.set_bind_group.bind_group_id = bind_group_id;
    return true;
}



/**
 * Append a SetVertexBuffer command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param slot the vertex buffer slot
 * @param buffer_id the buffer id
 * @param offset the byte offset
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_vertex_buffer(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t buffer_id,
    uint64_t offset)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    if (command == NULL)
        return false;
    command->u.set_vertex_buffer.pass_id = pass_id;
    command->u.set_vertex_buffer.slot = slot;
    command->u.set_vertex_buffer.buffer_id = buffer_id;
    command->u.set_vertex_buffer.offset = offset;
    return true;
}



/**
 * Append a SetIndexBuffer command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param buffer_id the index buffer id
 * @param index_format the index format token
 * @param offset the byte offset
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_index_buffer(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint64_t buffer_id, const char* index_format,
    uint64_t offset)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_INDEX_BUFFER);
    if (command == NULL)
        return false;
    command->u.set_index_buffer.pass_id = pass_id;
    command->u.set_index_buffer.buffer_id = buffer_id;
    _copy_label(
        command->u.set_index_buffer.index_format, DVZ_DRP2_LABEL_SIZE,
        index_format ? index_format : "");
    command->u.set_index_buffer.offset = offset;
    return true;
}



/**
 * Append a Draw command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param vertex_count the vertex count
 * @param instance_count the instance count
 * @param first_vertex the first vertex
 * @param first_instance the first instance
 * @return whether the command was appended
 */
bool dvz_drp2_stream_draw(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t vertex_count,
    uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DRAW);
    if (command == NULL)
        return false;
    command->u.draw.pass_id = pass_id;
    command->u.draw.vertex_count = vertex_count;
    command->u.draw.instance_count = instance_count;
    command->u.draw.first_vertex = first_vertex;
    command->u.draw.first_instance = first_instance;
    return true;
}



/**
 * Append a DrawIndexed command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param index_count the index count
 * @param instance_count the instance count
 * @param first_index the first index
 * @param base_vertex the base vertex
 * @param first_instance the first instance
 * @return whether the command was appended
 */
bool dvz_drp2_stream_draw_indexed(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t index_count,
    uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DRAW_INDEXED);
    if (command == NULL)
        return false;
    command->u.draw_indexed.pass_id = pass_id;
    command->u.draw_indexed.index_count = index_count;
    command->u.draw_indexed.instance_count = instance_count;
    command->u.draw_indexed.first_index = first_index;
    command->u.draw_indexed.base_vertex = base_vertex;
    command->u.draw_indexed.first_instance = first_instance;
    return true;
}



/**
 * Append an EndRenderPass command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_end_render_pass(DvzDrp2CommandStream* stream, uint64_t pass_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_END_RENDER_PASS);
    if (command == NULL)
        return false;
    command->u.end_render_pass.pass_id = pass_id;
    return true;
}



/**
 * Append a DispatchWorkgroups command.
 *
 * @param stream the command stream
 * @param pass_id the compute pass id
 * @param x the x workgroup count
 * @param y the y workgroup count
 * @param z the z workgroup count
 * @return whether the command was appended
 */
bool dvz_drp2_stream_dispatch_workgroups(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t x, uint32_t y, uint32_t z)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);
    if (command == NULL)
        return false;
    command->u.dispatch.pass_id = pass_id;
    command->u.dispatch.x = x;
    command->u.dispatch.y = y;
    command->u.dispatch.z = z;
    return true;
}



/**
 * Append an EndComputePass command.
 *
 * @param stream the command stream
 * @param pass_id the compute pass id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_end_compute_pass(DvzDrp2CommandStream* stream, uint64_t pass_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_END_COMPUTE_PASS);
    if (command == NULL)
        return false;
    command->u.end_compute_pass.pass_id = pass_id;
    return true;
}



/**
 * Append a CopyBufferToBuffer command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_buffer_id the source buffer id
 * @param src_offset the source byte offset
 * @param dst_buffer_id the destination buffer id
 * @param dst_offset the destination byte offset
 * @param size the copied byte size
 * @return whether the command was appended
 */
bool dvz_drp2_stream_copy_buffer_to_buffer(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_buffer_id,
    uint64_t src_offset, uint64_t dst_buffer_id, uint64_t dst_offset, uint64_t size)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER);
    if (command == NULL)
        return false;
    command->u.copy_buffer_to_buffer.encoder_id = encoder_id;
    command->u.copy_buffer_to_buffer.src_buffer_id = src_buffer_id;
    command->u.copy_buffer_to_buffer.src_offset = src_offset;
    command->u.copy_buffer_to_buffer.dst_buffer_id = dst_buffer_id;
    command->u.copy_buffer_to_buffer.dst_offset = dst_offset;
    command->u.copy_buffer_to_buffer.size = size;
    return true;
}



/**
 * Append a CopyBufferToTexture command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_buffer_id the source buffer id
 * @param src_offset the source byte offset
 * @param dst_texture_id the destination texture id
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @return whether the command was appended
 */
bool dvz_drp2_stream_copy_buffer_to_texture(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_buffer_id,
    uint64_t src_offset, uint64_t dst_texture_id, uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE);
    if (command == NULL)
        return false;
    command->u.copy_buffer_to_texture.encoder_id = encoder_id;
    command->u.copy_buffer_to_texture.src_buffer_id = src_buffer_id;
    command->u.copy_buffer_to_texture.src_offset = src_offset;
    command->u.copy_buffer_to_texture.bytes_per_row = bytes_per_row;
    command->u.copy_buffer_to_texture.rows_per_image = rows_per_image;
    command->u.copy_buffer_to_texture.dst_texture_id = dst_texture_id;
    command->u.copy_buffer_to_texture.width = width;
    command->u.copy_buffer_to_texture.height = height;
    command->u.copy_buffer_to_texture.depth = 1;
    return true;
}



/**
 * Append a CopyTextureToBuffer command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_texture_id the source texture id
 * @param dst_buffer_id the destination buffer id
 * @param dst_offset the destination byte offset
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @param bytes_per_row the destination bytes per row
 * @param rows_per_image the destination rows per image
 * @return whether the command was appended
 */
bool dvz_drp2_stream_copy_texture_to_buffer(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_texture_id,
    uint64_t dst_buffer_id, uint64_t dst_offset, uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER);
    if (command == NULL)
        return false;
    command->u.copy_texture_to_buffer.encoder_id = encoder_id;
    command->u.copy_texture_to_buffer.src_texture_id = src_texture_id;
    command->u.copy_texture_to_buffer.dst_buffer_id = dst_buffer_id;
    command->u.copy_texture_to_buffer.dst_offset = dst_offset;
    command->u.copy_texture_to_buffer.width = width;
    command->u.copy_texture_to_buffer.height = height;
    command->u.copy_texture_to_buffer.bytes_per_row = bytes_per_row;
    command->u.copy_texture_to_buffer.rows_per_image = rows_per_image;
    return true;
}



/**
 * Append a CopyTextureToTexture command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_texture_id the source texture id
 * @param dst_texture_id the destination texture id
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @return whether the command was appended
 */
bool dvz_drp2_stream_copy_texture_to_texture(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_texture_id,
    uint64_t dst_texture_id, uint32_t width, uint32_t height)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE);
    if (command == NULL)
        return false;
    command->u.copy_texture_to_texture.encoder_id = encoder_id;
    command->u.copy_texture_to_texture.src_texture_id = src_texture_id;
    command->u.copy_texture_to_texture.dst_texture_id = dst_texture_id;
    command->u.copy_texture_to_texture.width = width;
    command->u.copy_texture_to_texture.height = height;
    command->u.copy_texture_to_texture.depth = 1;
    return true;
}



/**
 * Append a FinishCommandEncoder command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param command_buffer_id the command buffer id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_finish_command_encoder(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t command_buffer_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    if (command == NULL)
        return false;
    command->u.finish_command_encoder.encoder_id = encoder_id;
    command->u.finish_command_encoder.command_buffer_id = command_buffer_id;
    return true;
}



/**
 * Append a QueueSubmit command with one command buffer and no readback.
 *
 * @param stream the command stream
 * @param command_buffer_id the command buffer id
 * @param submission_id the submission id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_queue_submit(
    DvzDrp2CommandStream* stream, uint64_t command_buffer_id, uint64_t submission_id)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_QUEUE_SUBMIT);
    if (command == NULL)
        return false;
    command->u.queue_submit.command_buffer_id = command_buffer_id;
    command->u.queue_submit.submission_id = submission_id;
    return true;
}



/**
 * Append a QueueSubmit command with one command buffer and one readback request.
 *
 * @param stream the command stream
 * @param command_buffer_id the command buffer id
 * @param submission_id the submission id
 * @param buffer_id the readback buffer id
 * @param offset the readback byte offset
 * @param size the readback byte size
 * @return whether the command was appended
 */
bool dvz_drp2_stream_queue_submit_readback(
    DvzDrp2CommandStream* stream, uint64_t command_buffer_id, uint64_t submission_id,
    uint64_t buffer_id, uint64_t offset, uint64_t size)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_QUEUE_SUBMIT);
    if (command == NULL)
        return false;
    command->u.queue_submit.command_buffer_id = command_buffer_id;
    command->u.queue_submit.submission_id = submission_id;
    command->u.queue_submit.has_readback = true;
    command->u.queue_submit.buffer_id = buffer_id;
    command->u.queue_submit.offset = offset;
    command->u.queue_submit.size = size;
    return true;
}



/**
 * Serialize a command stream as a DRP2 fixture JSON document.
 *
 * @param stream the command stream
 * @param name the fixture name
 * @return an owned NUL-terminated JSON string
 */
char* dvz_drp2_stream_json(const DvzDrp2CommandStream* stream, const char* name)
{
    if (stream == NULL)
        return NULL;

    JsonBuilder builder = {0};
    if (!_json_init(&builder))
        return NULL;

    const char* fixture_name = name != NULL ? name : "drp2_stream";
    _json_append(
        &builder,
        "{\n"
        "  \"name\": \"%s\",\n"
        "  \"version\": { \"major\": 2, \"minor\": 0 },\n"
        "  \"commands\": [\n",
        fixture_name);

    for (uint32_t i = 0; i < stream->count; i++)
    {
        _json_append(&builder, "    ");
        _json_append_command(&builder, &stream->commands[i]);
        _json_append(&builder, "%s\n", i + 1 < stream->count ? "," : "");
    }

    _json_append(
        &builder,
        "  ],\n"
        "  \"expected\": { \"outcome\": \"success\" }\n"
        "}\n");
    if (builder.failed)
    {
        dvz_free(builder.data);
        return NULL;
    }
    return builder.data;
}



/**
 * Destroy a JSON string returned by dvz_drp2_stream_json().
 *
 * @param json the JSON string
 */
void dvz_drp2_stream_json_destroy(char* json) { dvz_free(json); }


bool dvz_drp2_stream_write_buffer_bytes(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const void* data)
{
    ANN(stream);
    /* WebGPU-shaped: size==0 is a valid no-op that does not need to be recorded. */
    if (size == 0)
        return true;
    if (data == NULL)
        return false;

    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_BUFFER);
    if (command == NULL)
        return false;
    command->type                       = DVZ_DRP2_COMMAND_WRITE_BUFFER;
    command->u.write_buffer.buffer_id   = buffer_id;
    command->u.write_buffer.offset      = offset;
    command->u.write_buffer.size        = size;
    command->u.write_buffer.data_raw    = data; /* borrowed; caller keeps alive */
    command->u.write_buffer.data_base64 = NULL; /* populated only for JSON serialization */
    return true;
}


bool dvz_drp2_stream_write_texture_2d_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const void* data)
{
    ANN(stream);
    if (data == NULL)
        return false;

    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    if (command == NULL)
        return false;
    command->type                            = DVZ_DRP2_COMMAND_WRITE_TEXTURE;
    command->u.write_texture.texture_id      = texture_id;
    command->u.write_texture.mip_level       = mip_level;
    command->u.write_texture.width           = width;
    command->u.write_texture.height          = height;
    command->u.write_texture.depth           = 1;
    command->u.write_texture.bytes_per_row   = bytes_per_row;
    command->u.write_texture.rows_per_image  = rows_per_image;
    command->u.write_texture.data_raw        = data; /* borrowed; caller keeps alive */
    command->u.write_texture.data_base64     = NULL;
    return true;
}
