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
#include "_compat.h"
#include "_log.h"
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

    stream->capacity *= 2;
    stream->commands =
        (DvzDrp2Command*)dvz_realloc(stream->commands, stream->capacity * sizeof(DvzDrp2Command));
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
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return "CreateTexture";
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return "CreateShaderModule";
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return "CreateRenderPipeline";
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return "CreateComputePipeline";
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



static void _json_init(JsonBuilder* builder)
{
    ANN(builder);
    builder->capacity = DVZ_DRP2_JSON_INITIAL_CAPACITY;
    builder->count = 0;
    builder->data = (char*)dvz_calloc(builder->capacity, sizeof(char));
    ANN(builder->data);
}



static void _json_ensure(JsonBuilder* builder, uint64_t count)
{
    ANN(builder);
    ANN(builder->data);
    if (builder->count + count + 1 <= builder->capacity)
        return;

    while (builder->count + count + 1 > builder->capacity)
        builder->capacity *= 2;
    builder->data = (char*)dvz_realloc(builder->data, builder->capacity);
    ANN(builder->data);
}



static void _json_append(JsonBuilder* builder, const char* format, ...)
{
    ANN(builder);
    ANN(format);

    while (true)
    {
        va_list args;
        va_start(args, format);
        int written = dvz_vsnprintf(
            builder->data + builder->count, (size_t)(builder->capacity - builder->count), format,
            args);
        va_end(args);

        if (written < 0)
        {
            _json_ensure(builder, builder->capacity);
            continue;
        }

        uint64_t written_u = (uint64_t)written;
        if (builder->count + written_u + 1 <= builder->capacity)
        {
            builder->count += written_u;
            return;
        }
        _json_ensure(builder, written_u);
    }
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
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64
            ", \"dimension\": \"2d\", \"width\": %" PRIu32 ", \"height\": %" PRIu32
            ", \"depth\": 1, \"format\": \"rgba8unorm\", \"usage\": ",
            _command_name(command->type), command->u.create_texture.id,
            command->u.create_texture.width, command->u.create_texture.height);
        _json_append_texture_usage(builder, command->u.create_texture.usage);
        _json_append(builder, ", \"mip_level_count\": 1, \"sample_count\": 1 }");
        break;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"stage\": \"%s\", \"format\": \"wgsl\", "
            "\"entry_point\": \"main\", \"code\": \"%s\" }",
            _command_name(command->type), command->u.create_shader_module.id,
            command->u.create_shader_module.stage, command->u.create_shader_module.code);
        break;
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"vertex_buffer_slots\": %" PRIu32
            ", \"vertex_shader_module_id\": %" PRIu64
            ", \"fragment_shader_module_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.create_render_pipeline.id,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.vertex_shader_module_id,
            command->u.create_render_pipeline.fragment_shader_module_id);
        break;
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"compute_shader_module_id\": %" PRIu64
            " }",
            _command_name(command->type), command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id);
        break;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"buffer_id\": %" PRIu64 ", \"offset\": %" PRIu64
            ", \"size\": %" PRIu64 ", \"data\": \"%s\" }",
            _command_name(command->type), command->u.write_buffer.buffer_id,
            command->u.write_buffer.offset, command->u.write_buffer.size,
            command->u.write_buffer.data_base64);
        break;
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
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
            command->u.write_texture.rows_per_image, command->u.write_texture.data_base64);
        break;
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
            "\"clear_value\": { \"r\": 0, \"g\": 0, \"b\": 0, \"a\": 1 } } ] }",
            _command_name(command->type), command->u.begin_render_pass.id,
            command->u.begin_render_pass.encoder_id, command->u.begin_render_pass.texture_id);
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
    ANN(stream);
    stream->capacity = DVZ_DRP2_INITIAL_COMMAND_CAPACITY;
    stream->commands = (DvzDrp2Command*)dvz_calloc(stream->capacity, sizeof(DvzDrp2Command));
    ANN(stream->commands);
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
    command->u.create_texture.usage = usage;
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
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    if (command == NULL)
        return false;
    command->u.create_shader_module.id = id;
    _copy_label(command->u.create_shader_module.stage, DVZ_DRP2_LABEL_SIZE, stage ? stage : "");
    _copy_label(command->u.create_shader_module.code, DVZ_DRP2_LABEL_SIZE, code ? code : "");
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
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    if (command == NULL)
        return false;
    command->u.create_render_pipeline.id = id;
    command->u.create_render_pipeline.vertex_shader_module_id = vertex_shader_module_id;
    command->u.create_render_pipeline.fragment_shader_module_id = fragment_shader_module_id;
    command->u.create_render_pipeline.vertex_buffer_slots = vertex_buffer_slots;
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
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    if (command == NULL)
        return false;
    command->u.create_compute_pipeline.id = id;
    command->u.create_compute_pipeline.compute_shader_module_id = compute_shader_module_id;
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
    command->u.write_buffer.buffer_id = buffer_id;
    command->u.write_buffer.offset = offset;
    command->u.write_buffer.size = size;
    _copy_label(
        command->u.write_buffer.data_base64, DVZ_DRP2_LABEL_SIZE,
        data_base64 ? data_base64 : "");
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
    command->u.write_texture.texture_id = texture_id;
    command->u.write_texture.mip_level = mip_level;
    command->u.write_texture.width = width;
    command->u.write_texture.height = height;
    command->u.write_texture.depth = 1;
    command->u.write_texture.bytes_per_row = bytes_per_row;
    command->u.write_texture.rows_per_image = rows_per_image;
    _copy_label(
        command->u.write_texture.data_base64, DVZ_DRP2_LABEL_SIZE,
        data_base64 ? data_base64 : "");
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
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    if (command == NULL)
        return false;
    command->u.begin_render_pass.id = id;
    command->u.begin_render_pass.encoder_id = encoder_id;
    command->u.begin_render_pass.texture_id = texture_id;
    return true;
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
    _json_init(&builder);

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
    return builder.data;
}



/**
 * Destroy a JSON string returned by dvz_drp2_stream_json().
 *
 * @param json the JSON string
 */
void dvz_drp2_stream_json_destroy(char* json) { dvz_free(json); }
