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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_stream.h"




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



/**
 * Ensure the command stream has room for debug labels.
 *
 * @param stream the command stream
 * @return whether the label array can accept another item
 */
static bool _ensure_label_capacity(DvzDrp2CommandStream* stream)
{
    ANN(stream);
    if (stream->labels == NULL || stream->label_capacity == 0)
    {
        stream->label_capacity = 64;
        stream->labels =
            (DvzDrp2DebugLabel*)dvz_calloc(stream->label_capacity, sizeof(DvzDrp2DebugLabel));
        return stream->labels != NULL;
    }

    if (stream->label_count < stream->label_capacity)
        return true;

    if (stream->label_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = stream->label_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzDrp2DebugLabel), &bytes))
        return false;

    DvzDrp2DebugLabel* labels = (DvzDrp2DebugLabel*)dvz_realloc(stream->labels, bytes);
    if (labels == NULL)
        return false;

    stream->label_capacity = capacity;
    stream->labels = labels;
    return true;
}



static void _copy_label(char* dst, uint64_t dst_size, const char* src)
{
    ANN(dst);
    ANN(src);
    dvz_strlcpy(dst, src, (size_t)dst_size);
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
 * Release the owner lock associated with a DRP2 stream without destroying the stream.
 */
void _dvz_drp2_stream_release_owner(DvzDrp2CommandStream* stream)
{
    if (stream == NULL)
        return;
    if (stream->owner_release != NULL && !stream->owner_released)
    {
        stream->owner_release(stream->owner);
        stream->owner_released = true;
    }
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
    _dvz_drp2_stream_release_owner(stream);
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* cmd = &stream->commands[i];
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            if (cmd->u.write_buffer.data_raw_owned)
                dvz_free(cmd->u.write_buffer.data_raw);
            dvz_free(cmd->u.write_buffer.data_base64);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            if (cmd->u.write_texture.data_raw_owned)
                dvz_free((void*)(uintptr_t)cmd->u.write_texture.data_raw);
            dvz_free(cmd->u.write_texture.data_base64);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
            dvz_free(cmd->u.create_shader_module.code);
    }
    dvz_free(stream->commands);
    dvz_free(stream->labels);
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


static bool _payload_info(
    const DvzDrp2Command* command, const void** out_ptr, uint64_t* out_size)
{
    if (out_ptr != NULL)
        *out_ptr = NULL;
    if (out_size != NULL)
        *out_size = 0;
    if (command == NULL)
        return false;

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
        size = (uint64_t)command->u.write_texture.depth *
               (uint64_t)command->u.write_texture.rows_per_image *
               (uint64_t)command->u.write_texture.bytes_per_row;
    }

    if (ptr == NULL || size == 0)
        return false;
    if (out_ptr != NULL)
        *out_ptr = ptr;
    if (out_size != NULL)
        *out_size = size;
    return true;
}



uint32_t dvz_drp2_stream_payload_count(const DvzDrp2CommandStream* stream)
{
    if (stream == NULL)
        return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        if (_payload_info(&stream->commands[i], NULL, NULL))
            count++;
    }
    return count;
}



static const DvzDrp2Command* _payload_command(
    const DvzDrp2CommandStream* stream, uint32_t payload_index, uint32_t* out_command_index)
{
    if (out_command_index != NULL)
        *out_command_index = UINT32_MAX;
    if (stream == NULL)
        return NULL;

    uint32_t count = 0;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        if (!_payload_info(&stream->commands[i], NULL, NULL))
            continue;
        if (count == payload_index)
        {
            if (out_command_index != NULL)
                *out_command_index = i;
            return &stream->commands[i];
        }
        count++;
    }
    return NULL;
}



uint32_t
dvz_drp2_stream_payload_command_index(const DvzDrp2CommandStream* stream, uint32_t payload_index)
{
    uint32_t command_index = UINT32_MAX;
    (void)_payload_command(stream, payload_index, &command_index);
    return command_index;
}



const void* dvz_drp2_stream_payload_ptr(
    const DvzDrp2CommandStream* stream, uint32_t payload_index)
{
    const DvzDrp2Command* command = _payload_command(stream, payload_index, NULL);
    const void* ptr = NULL;
    (void)_payload_info(command, &ptr, NULL);
    return ptr;
}



uint64_t dvz_drp2_stream_payload_size(
    const DvzDrp2CommandStream* stream, uint32_t payload_index)
{
    const DvzDrp2Command* command = _payload_command(stream, payload_index, NULL);
    uint64_t size = 0;
    (void)_payload_info(command, NULL, &size);
    return size;
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
 * Attach a debug label to a numeric DRP2 id in a command stream.
 *
 * @param stream the command stream
 * @param id the DRP2 object or transient id
 * @param label the debug label, or NULL
 * @return whether the label was recorded
 */
bool dvz_drp2_stream_set_label(DvzDrp2CommandStream* stream, uint64_t id, const char* label)
{
    if (stream == NULL || id == 0)
        return false;

    for (uint32_t i = 0; i < stream->label_count; i++)
    {
        if (stream->labels[i].id == id)
        {
            _copy_label(stream->labels[i].label, DVZ_DRP2_LABEL_SIZE, label ? label : "");
            return true;
        }
    }

    if (!_ensure_label_capacity(stream))
        return false;

    DvzDrp2DebugLabel* entry = &stream->labels[stream->label_count++];
    entry->id = id;
    _copy_label(entry->label, DVZ_DRP2_LABEL_SIZE, label ? label : "");
    return true;
}



/**
 * Return a debug label attached to a numeric DRP2 id.
 *
 * @param stream the command stream
 * @param id the DRP2 object or transient id
 * @return the label, or NULL when none exists
 */
const char* dvz_drp2_stream_label(const DvzDrp2CommandStream* stream, uint64_t id)
{
    if (stream == NULL || id == 0)
        return NULL;

    for (uint32_t i = 0; i < stream->label_count; i++)
    {
        if (stream->labels[i].id == id)
            return stream->labels[i].label[0] != '\0' ? stream->labels[i].label : NULL;
    }
    return NULL;
}



/**
 * Return the numeric DRP2 id attached to a debug label.
 *
 * @param stream the command stream
 * @param label the debug label
 * @return the id, or 0 when none exists
 */
uint64_t dvz_drp2_stream_label_id(const DvzDrp2CommandStream* stream, const char* label)
{
    if (stream == NULL || label == NULL || label[0] == '\0')
        return 0;

    for (uint32_t i = 0; i < stream->label_count; i++)
    {
        if (strcmp(stream->labels[i].label, label) == 0)
            return stream->labels[i].id;
    }
    return 0;
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
    return dvz_drp2_stream_create_texture_2d_format_usage(
        stream, id, width, height, VK_FORMAT_R8G8B8A8_UNORM,
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
    return dvz_drp2_stream_create_texture_2d_format_usage(
        stream, id, width, height, VK_FORMAT_R8G8B8A8_UNORM, usage);
}



/**
 * Append a CreateTexture command for a 2D texture with explicit format and usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param format texture format, using VkFormat values
 * @param usage texture usage flags
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_2d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage)
{
    return dvz_drp2_stream_create_texture_2d_format_usage_samples(
        stream, id, width, height, format, usage, 1);
}



/**
 * Append a CreateTexture command for a 2D texture with explicit format, usage, and samples.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param format texture format, using VkFormat values
 * @param usage texture usage flags
 * @param sample_count raster sample count, with 0 treated as 1
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_2d_format_usage_samples(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage, uint32_t sample_count)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    if (command == NULL)
        return false;
    command->u.create_texture.id = id;
    command->u.create_texture.width = width;
    command->u.create_texture.height = height;
    command->u.create_texture.depth = 1;
    command->u.create_texture.format = format;
    command->u.create_texture.usage = usage;
    command->u.create_texture.sample_count = sample_count == 0 ? 1 : sample_count;
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
    dvz_memcpy(code_copy, n, src, n);

    command->u.create_shader_module.id = id;
    _copy_label(command->u.create_shader_module.stage, DVZ_DRP2_LABEL_SIZE, stage ? stage : "");
    _copy_label(
        command->u.create_shader_module.format, DVZ_DRP2_LABEL_SIZE,
        format != NULL && format[0] != '\0' ? format : "wgsl");
    command->u.create_shader_module.code = code_copy;
    return true;
}



/**
 * Append a CreateShaderModule command from a precompiled SPIR-V binary.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage ("VERTEX" or "FRAGMENT")
 * @param spirv pointer to SPIR-V bytecode (borrowed — caller keeps alive until execute)
 * @param spirv_size size in bytes
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_shader_module_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const unsigned char* spirv, uint64_t spirv_size)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    if (command == NULL)
        return false;
    command->u.create_shader_module.id = id;
    _copy_label(command->u.create_shader_module.stage,  DVZ_DRP2_LABEL_SIZE, stage ? stage : "");
    _copy_label(command->u.create_shader_module.format, DVZ_DRP2_LABEL_SIZE, "spirv");
    command->u.create_shader_module.spirv      = spirv;
    command->u.create_shader_module.spirv_size = spirv_size;
    return true;
}


/**
 * Attach optional built-in shader identity metadata to a CreateShaderModule command.
 *
 * @param stream the command stream
 * @param shader_module_id the shader module id
 * @param family stable built-in shader family id
 * @param variant stable built-in shader variant id
 * @param version built-in shader contract version
 * @return whether the matching command was found and updated
 */
bool dvz_drp2_stream_shader_set_builtin_identity(
    DvzDrp2CommandStream* stream, uint64_t shader_module_id, const char* family,
    const char* variant, uint32_t version)
{
    if (stream == NULL || shader_module_id == 0 || family == NULL || variant == NULL)
        return false;

    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type != DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE ||
            command->u.create_shader_module.id != shader_module_id)
            continue;

        _copy_label(
            command->u.create_shader_module.builtin_family, DVZ_DRP2_LABEL_SIZE, family);
        _copy_label(
            command->u.create_shader_module.builtin_variant, DVZ_DRP2_LABEL_SIZE, variant);
        command->u.create_shader_module.builtin_version = version;
        return true;
    }
    return false;
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
    command->u.create_render_pipeline.sample_count = 1;
    command->u.create_render_pipeline.color_target_count = 1;
    command->u.create_render_pipeline.color_targets[0].format = 0;
    command->u.create_render_pipeline.color_targets[0].color_write_mask = 0xFu;
    if (bind_group_layout_id != 0)
    {
        command->u.create_render_pipeline.bind_group_layout_count = 1;
        command->u.create_render_pipeline.bind_group_layout_ids[0] = bind_group_layout_id;
    }
    return true;
}



/**
 * Append a CreateRenderPipeline command with explicit vertex input layout and topology.
 */
bool dvz_drp2_stream_pipeline_set_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.bind_group_layout_count = bind_group_layout_id == 0 ? 0 : 1;
    command->u.create_render_pipeline.bind_group_layout_ids[0] = bind_group_layout_id;
    return true;
}



bool dvz_drp2_stream_pipeline_set_bind_group_layout2(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id2)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    if (command->u.create_render_pipeline.bind_group_layout_count < 1)
        command->u.create_render_pipeline.bind_group_layout_count = 1;
    if (bind_group_layout_id2 != 0)
    {
        command->u.create_render_pipeline.bind_group_layout_count = 2;
        command->u.create_render_pipeline.bind_group_layout_ids[1] = bind_group_layout_id2;
    }
    else if (command->u.create_render_pipeline.bind_group_layout_count == 2)
    {
        command->u.create_render_pipeline.bind_group_layout_count = 1;
        command->u.create_render_pipeline.bind_group_layout_ids[1] = 0;
    }
    return true;
}


bool dvz_drp2_stream_pipeline_set_bind_group_layouts(
    DvzDrp2CommandStream* stream, uint32_t count, const uint64_t* bind_group_layout_ids)
{
    ANN(stream);
    if (stream->count == 0 || count > DVZ_DRP2_MAX_BIND_GROUPS)
        return false;
    if (count > 0)
        ANN(bind_group_layout_ids);
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.bind_group_layout_count = count;
    dvz_memset(
        command->u.create_render_pipeline.bind_group_layout_ids,
        sizeof(command->u.create_render_pipeline.bind_group_layout_ids), 0,
        sizeof(command->u.create_render_pipeline.bind_group_layout_ids));
    if (count > 0)
    {
        dvz_memcpy(
            command->u.create_render_pipeline.bind_group_layout_ids,
            sizeof(command->u.create_render_pipeline.bind_group_layout_ids), bind_group_layout_ids,
            count * sizeof(uint64_t));
    }
    return true;
}



/**
 * Attach depth state to the most recent CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param depth_write_enabled whether depth writes are enabled
 * @param depth_compare_op the VkCompareOp depth compare operator
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_pipeline_set_depth_state(
    DvzDrp2CommandStream* stream, bool depth_write_enabled, uint32_t depth_compare_op)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.has_depth_attachment = true;
    command->u.create_render_pipeline.depth_write_enabled = depth_write_enabled;
    command->u.create_render_pipeline.depth_compare_op = depth_compare_op;
    return true;
}


/**
 * Attach raster state to the most recent CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param cull_mode the VkCullModeFlags value
 * @param front_face the VkFrontFace value
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_pipeline_set_raster_state(
    DvzDrp2CommandStream* stream, uint32_t cull_mode, uint32_t front_face)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.has_raster_state = true;
    command->u.create_render_pipeline.cull_mode = cull_mode;
    command->u.create_render_pipeline.front_face = front_face;
    return true;
}



/**
 * Set multisampling state on the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param sample_count raster sample count, with 0 treated as 1
 * @param alpha_to_coverage_enabled whether alpha-to-coverage is enabled
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
bool dvz_drp2_stream_pipeline_set_multisampling(
    DvzDrp2CommandStream* stream, uint32_t sample_count, bool alpha_to_coverage_enabled)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.sample_count = sample_count == 0 ? 1 : sample_count;
    command->u.create_render_pipeline.alpha_to_coverage_enabled = alpha_to_coverage_enabled;
    return true;
}



bool dvz_drp2_stream_pipeline_set_color_target(
    DvzDrp2CommandStream* stream, uint32_t idx, uint32_t format)
{
    ANN(stream);
    if (stream->count == 0 || idx >= DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.color_target_count =
        MAX(command->u.create_render_pipeline.color_target_count, idx + 1);
    command->u.create_render_pipeline.color_targets[idx].format = format;
    if (command->u.create_render_pipeline.color_targets[idx].color_write_mask == 0)
        command->u.create_render_pipeline.color_targets[idx].color_write_mask = 0xFu;
    return true;
}



bool dvz_drp2_stream_pipeline_set_color_blend(
    DvzDrp2CommandStream* stream, uint32_t idx, uint32_t src_color, uint32_t dst_color,
    uint32_t color_op, uint32_t src_alpha, uint32_t dst_alpha, uint32_t alpha_op,
    uint32_t color_write_mask)
{
    ANN(stream);
    if (stream->count == 0 || idx >= DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        return false;
    command->u.create_render_pipeline.color_target_count =
        MAX(command->u.create_render_pipeline.color_target_count, idx + 1);
    DvzDrp2ColorTarget* target = &command->u.create_render_pipeline.color_targets[idx];
    target->blend_enabled = true;
    target->src_color_blend_factor = src_color;
    target->dst_color_blend_factor = dst_color;
    target->color_blend_op = color_op;
    target->src_alpha_blend_factor = src_alpha;
    target->dst_alpha_blend_factor = dst_alpha;
    target->alpha_blend_op = alpha_op;
    target->color_write_mask = color_write_mask;
    return true;
}


/**
 * Attach optional built-in pipeline identity metadata to a CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param render_pipeline_id the render pipeline id
 * @param pipeline stable built-in pipeline id
 * @param version built-in pipeline contract version
 * @return whether the matching command was found and updated
 */
bool dvz_drp2_stream_pipeline_set_builtin_identity(
    DvzDrp2CommandStream* stream, uint64_t render_pipeline_id, const char* pipeline,
    uint32_t version)
{
    if (stream == NULL || render_pipeline_id == 0 || pipeline == NULL)
        return false;

    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE ||
            command->u.create_render_pipeline.id != render_pipeline_id)
            continue;

        _copy_label(
            command->u.create_render_pipeline.builtin_pipeline, DVZ_DRP2_LABEL_SIZE,
            pipeline);
        command->u.create_render_pipeline.builtin_version = version;
        return true;
    }
    return false;
}



bool dvz_drp2_stream_create_render_pipeline_ex(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint32_t topology,
    uint32_t binding_count, const uint32_t* binding_strides,
    uint32_t attr_count, const uint32_t* attr_bindings, const uint32_t* attr_locations,
    const uint32_t* attr_formats, const uint32_t* attr_offsets)
{
    return dvz_drp2_stream_create_render_pipeline_ex2(
        stream, id, vertex_shader_module_id, fragment_shader_module_id, vertex_buffer_slots,
        topology, binding_count, binding_strides, NULL, attr_count, attr_bindings,
        attr_locations, attr_formats, attr_offsets);
}



bool dvz_drp2_stream_create_render_pipeline_ex2(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint32_t topology,
    uint32_t binding_count, const uint32_t* binding_strides,
    const uint32_t* binding_step_modes,
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
    command->u.create_render_pipeline.bind_group_layout_count = 0;
    command->u.create_render_pipeline.has_depth_attachment = false;
    command->u.create_render_pipeline.depth_write_enabled = false;
    command->u.create_render_pipeline.depth_compare_op = VK_COMPARE_OP_ALWAYS;
    command->u.create_render_pipeline.has_raster_state = false;
    command->u.create_render_pipeline.cull_mode = VK_CULL_MODE_NONE;
    command->u.create_render_pipeline.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command->u.create_render_pipeline.sample_count = 1;
    command->u.create_render_pipeline.alpha_to_coverage_enabled = false;
    command->u.create_render_pipeline.color_target_count = 1;
    command->u.create_render_pipeline.color_targets[0].format = 0;
    command->u.create_render_pipeline.color_targets[0].color_write_mask = 0xFu;
    command->u.create_render_pipeline.topology = topology;
    uint32_t nb = binding_count < 16 ? binding_count : 16;
    command->u.create_render_pipeline.binding_count = nb;
    for (uint32_t i = 0; i < nb; i++)
    {
        command->u.create_render_pipeline.binding_strides[i] = binding_strides[i];
        command->u.create_render_pipeline.binding_step_modes[i] =
            binding_step_modes != NULL ? binding_step_modes[i] : DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
    }
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
    if (bind_group_layout_id != 0)
    {
        command->u.create_compute_pipeline.bind_group_layout_count = 1;
        command->u.create_compute_pipeline.bind_group_layout_ids[0] = bind_group_layout_id;
    }
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
    return dvz_drp2_stream_create_sampler_filter(
        stream, id, DVZ_DRP2_FILTER_LINEAR, DVZ_DRP2_FILTER_LINEAR);
}


/**
 * Append a CreateSampler command with explicit min/mag filters.
 *
 * @param stream the command stream
 * @param id the sampler id
 * @param mag_filter magnification filter
 * @param min_filter minification filter
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_sampler_filter(
    DvzDrp2CommandStream* stream, uint64_t id, DvzDrp2FilterMode mag_filter,
    DvzDrp2FilterMode min_filter)
{
    if ((mag_filter != DVZ_DRP2_FILTER_LINEAR && mag_filter != DVZ_DRP2_FILTER_NEAREST) ||
        (min_filter != DVZ_DRP2_FILTER_LINEAR && min_filter != DVZ_DRP2_FILTER_NEAREST))
        return false;
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    if (command == NULL)
        return false;
    command->u.create_sampler.id = id;
    command->u.create_sampler.mag_filter = mag_filter;
    command->u.create_sampler.min_filter = min_filter;
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
    DvzDrp2BindGroupLayoutEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 2, entries);
}



/**
 * Append a CreateBindGroupLayout command for compute input/output storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_storage_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2BindGroupLayoutEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE,
            .access = DVZ_DRP2_BINDING_ACCESS_READ_WRITE,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 2, entries);
}



/**
 * Append a CreateBindGroupLayout command for one uniform buffer (VS + FS visible).
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_uniform_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    DvzDrp2BindGroupLayoutEntry entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 1, &entry);
}


bool dvz_drp2_stream_create_bind_group_layout_entries(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t entry_count,
    const DvzDrp2BindGroupLayoutEntry* entries)
{
    if (entry_count == 0 || entry_count > DVZ_DRP2_MAX_BINDINGS)
        return false;
    ANN(entries);
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    if (command == NULL)
        return false;
    command->u.create_bind_group_layout.id = id;
    command->u.create_bind_group_layout.entry_count = entry_count;
    dvz_memcpy(
        command->u.create_bind_group_layout.entries,
        sizeof(command->u.create_bind_group_layout.entries), entries,
        entry_count * sizeof(DvzDrp2BindGroupLayoutEntry));
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
    DvzDrp2BindGroupEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = texture_id,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = sampler_id,
        },
    };
    return dvz_drp2_stream_create_bind_group_entries(
        stream, id, bind_group_layout_id, 2, entries);
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
    DvzDrp2BindGroupEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
            .resource_id = buffer0_id,
            .size = buffer_size,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
            .resource_id = buffer1_id,
            .size = buffer_size,
        },
    };
    return dvz_drp2_stream_create_bind_group_entries(
        stream, id, bind_group_layout_id, 2, entries);
}



/**
 * Append a CreateBindGroup command for one uniform buffer with a sub-allocation offset.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param buffer_id the uniform buffer id
 * @param offset byte offset into the buffer for this sub-allocation
 * @param size bound range size in bytes
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_uniform_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t buffer_id,
    uint64_t offset, uint64_t size)
{
    DvzDrp2BindGroupEntry entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = buffer_id,
        .offset = offset,
        .size = size,
    };
    return dvz_drp2_stream_create_bind_group_entries(
        stream, id, bind_group_layout_id, 1, &entry);
}


bool dvz_drp2_stream_create_bind_group_entries(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id,
    uint32_t entry_count, const DvzDrp2BindGroupEntry* entries)
{
    if (entry_count == 0 || entry_count > DVZ_DRP2_MAX_BINDINGS)
        return false;
    ANN(entries);
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.create_bind_group.id = id;
    command->u.create_bind_group.bind_group_layout_id = bind_group_layout_id;
    command->u.create_bind_group.entry_count = entry_count;
    dvz_memcpy(
        command->u.create_bind_group.entries, sizeof(command->u.create_bind_group.entries),
        entries, entry_count * sizeof(DvzDrp2BindGroupEntry));
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
    dvz_memcpy(buf, n, src, n);

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
    dvz_memcpy(buf, n, src, n);

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
    dvz_memcpy(buf, n, src, n);

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



/**
 * Append a CreateTexture command for a 3D texture with default RGBA8 format and sampled usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_3d(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth)
{
    return dvz_drp2_stream_create_texture_3d_format_usage(
        stream, id, width, height, depth, VK_FORMAT_R8G8B8A8_UNORM,
        DVZ_DRP2_TEXTURE_USAGE_COPY_SRC | DVZ_DRP2_TEXTURE_USAGE_COPY_DST |
            DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING);
}



/**
 * Append a CreateTexture command for a 3D texture with explicit format and usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @param format texture format, using VkFormat values
 * @param usage texture usage flags
 * @return whether the command was appended
 */
bool dvz_drp2_stream_create_texture_3d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t format, uint32_t usage)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    if (command == NULL)
        return false;
    command->u.create_texture.id    = id;
    command->u.create_texture.width = width;
    command->u.create_texture.height = height;
    command->u.create_texture.depth = depth;
    command->u.create_texture.format = format;
    command->u.create_texture.usage = usage;
    command->u.create_texture.sample_count = 1;
    return true;
}



/**
 * Append a WriteTexture command for a 3D texture subregion.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param origin_z z offset in texels
 * @param width the written width
 * @param height the written height
 * @param depth the written depth
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
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
    dvz_memcpy(buf, n, src, n);

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
 * Append a WriteTexture command for a 3D texture subregion using raw bytes.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param origin_z z offset in texels
 * @param width the written width
 * @param height the written height
 * @param depth the written depth
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data raw texture bytes
 * @return whether the command was appended
 */
bool dvz_drp2_stream_write_texture_3d_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, const void* data)
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
    command->u.write_texture.origin_x        = origin_x;
    command->u.write_texture.origin_y        = origin_y;
    command->u.write_texture.origin_z        = origin_z;
    command->u.write_texture.width           = width;
    command->u.write_texture.height          = height;
    command->u.write_texture.depth           = depth;
    command->u.write_texture.bytes_per_row   = bytes_per_row;
    command->u.write_texture.rows_per_image  = rows_per_image;
    command->u.write_texture.data_raw        = data; /* borrowed; caller keeps alive */
    command->u.write_texture.data_base64     = NULL;
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
    command->u.begin_render_pass.color_attachment_count = 1;
    command->u.begin_render_pass.color_attachments[0].texture_id = texture_id;
    command->u.begin_render_pass.color_attachments[0].clear = clear;
    command->u.begin_render_pass.color_attachments[0].load_op =
        clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR : DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    command->u.begin_render_pass.color_attachments[0].store_op =
        DVZ_DRP2_ATTACHMENT_STORE_STORE;
    command->u.begin_render_pass.color_attachments[0].access =
        DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    command->u.begin_render_pass.color_attachments[0].clear_color[0] = r;
    command->u.begin_render_pass.color_attachments[0].clear_color[1] = g;
    command->u.begin_render_pass.color_attachments[0].clear_color[2] = b;
    command->u.begin_render_pass.color_attachments[0].clear_color[3] = a;
    command->u.begin_render_pass.has_depth_attachment = false;
    command->u.begin_render_pass.depth_texture_id = 0;
    command->u.begin_render_pass.depth_load_op =
        clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR : DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    command->u.begin_render_pass.depth_store_op = DVZ_DRP2_ATTACHMENT_STORE_STORE;
    command->u.begin_render_pass.depth_access = DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    command->u.begin_render_pass.depth_ops_explicit = false;
    command->u.begin_render_pass.clear_depth = 1.0f;
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



/**
 * Add a color attachment to the most recently appended BeginRenderPass command.
 *
 * @param stream the command stream
 * @param texture_id the color attachment texture id
 * @param r clear color red channel
 * @param g clear color green channel
 * @param b clear color blue channel
 * @param a clear color alpha channel
 * @param clear whether to clear this attachment at render-pass begin
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_add_color_attachment(
    DvzDrp2CommandStream* stream, uint64_t texture_id, float r, float g, float b, float a,
    bool clear)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        return false;
    uint32_t idx = command->u.begin_render_pass.color_attachment_count;
    if (idx >= DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return false;
    command->u.begin_render_pass.color_attachments[idx].texture_id = texture_id;
    command->u.begin_render_pass.color_attachments[idx].clear = clear;
    command->u.begin_render_pass.color_attachments[idx].load_op =
        clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR : DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    command->u.begin_render_pass.color_attachments[idx].store_op =
        DVZ_DRP2_ATTACHMENT_STORE_STORE;
    command->u.begin_render_pass.color_attachments[idx].access =
        DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    command->u.begin_render_pass.color_attachments[idx].clear_color[0] = r;
    command->u.begin_render_pass.color_attachments[idx].clear_color[1] = g;
    command->u.begin_render_pass.color_attachments[idx].clear_color[2] = b;
    command->u.begin_render_pass.color_attachments[idx].clear_color[3] = a;
    command->u.begin_render_pass.color_attachment_count++;
    return true;
}



/**
 * Set load/store operations on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param load_op the attachment load operation
 * @param store_op the attachment store operation
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, DvzDrp2AttachmentLoadOp load_op,
    DvzDrp2AttachmentStoreOp store_op)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS ||
        attachment_index >= command->u.begin_render_pass.color_attachment_count)
        return false;
    command->u.begin_render_pass.color_attachments[attachment_index].load_op = load_op;
    command->u.begin_render_pass.color_attachments[attachment_index].store_op = store_op;
    command->u.begin_render_pass.color_attachments[attachment_index].clear =
        load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR;
    return true;
}



/**
 * Set access intent on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param access the attachment access intent
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, DvzDrp2AttachmentAccess access)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS ||
        attachment_index >= command->u.begin_render_pass.color_attachment_count)
        return false;
    command->u.begin_render_pass.color_attachments[attachment_index].access = access;
    return true;
}



/**
 * Set the resolve target on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param resolve_texture_id the single-sample resolve texture id, or 0 to disable resolve
 * @param resolve_mode backend-native resolve mode, with 0 treated as average
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_color_attachment_resolve(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, uint64_t resolve_texture_id,
    uint32_t resolve_mode)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS ||
        attachment_index >= command->u.begin_render_pass.color_attachment_count)
        return false;
    command->u.begin_render_pass.color_attachments[attachment_index].resolve_texture_id =
        resolve_texture_id;
    command->u.begin_render_pass.color_attachments[attachment_index].resolve_mode =
        resolve_mode == 0 ? VK_RESOLVE_MODE_AVERAGE_BIT : resolve_mode;
    return true;
}



/**
 * Attach a transient depth attachment request to the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param clear_depth the depth clear value
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_depth(DvzDrp2CommandStream* stream, float clear_depth)
{
    return dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 0, clear_depth);
}



/**
 * Attach a named depth texture to the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param depth_texture_id the depth attachment texture id, or 0 for transient depth
 * @param clear_depth the depth clear value
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_depth_texture(
    DvzDrp2CommandStream* stream, uint64_t depth_texture_id, float clear_depth)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        return false;
    command->u.begin_render_pass.has_depth_attachment = true;
    command->u.begin_render_pass.depth_texture_id = depth_texture_id;
    command->u.begin_render_pass.depth_load_op =
        command->u.begin_render_pass.clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR :
                                             DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    command->u.begin_render_pass.depth_store_op = DVZ_DRP2_ATTACHMENT_STORE_STORE;
    command->u.begin_render_pass.depth_access = DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    command->u.begin_render_pass.depth_ops_explicit = false;
    command->u.begin_render_pass.clear_depth = clear_depth;
    return true;
}



/**
 * Set load/store operations on the depth attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param load_op the depth attachment load operation
 * @param store_op the depth attachment store operation
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_depth_ops(
    DvzDrp2CommandStream* stream, DvzDrp2AttachmentLoadOp load_op,
    DvzDrp2AttachmentStoreOp store_op)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        return false;
    command->u.begin_render_pass.has_depth_attachment = true;
    command->u.begin_render_pass.depth_load_op = load_op;
    command->u.begin_render_pass.depth_store_op = store_op;
    command->u.begin_render_pass.depth_ops_explicit = true;
    return true;
}



/**
 * Set access intent on the depth attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param access the depth attachment access intent
 * @return whether the most recent command was updated
 */
bool dvz_drp2_stream_begin_render_pass_set_depth_access(
    DvzDrp2CommandStream* stream, DvzDrp2AttachmentAccess access)
{
    ANN(stream);
    if (stream->count == 0)
        return false;
    DvzDrp2Command* command = &stream->commands[stream->count - 1];
    if (command->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        return false;
    command->u.begin_render_pass.has_depth_attachment = true;
    command->u.begin_render_pass.depth_access = access;
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
 * Append a SetViewport command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_viewport(
    DvzDrp2CommandStream* stream, uint64_t pass_id, float x, float y, float width, float height)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_VIEWPORT);
    if (command == NULL)
        return false;
    command->u.set_viewport.pass_id = pass_id;
    command->u.set_viewport.viewport[0] = x;
    command->u.set_viewport.viewport[1] = y;
    command->u.set_viewport.viewport[2] = width;
    command->u.set_viewport.viewport[3] = height;
    return true;
}



/**
 * Append a SetScissor command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @return whether the command was appended
 */
bool dvz_drp2_stream_set_scissor(
    DvzDrp2CommandStream* stream, uint64_t pass_id, float x, float y, float width, float height)
{
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_SCISSOR);
    if (command == NULL)
        return false;
    command->u.set_scissor.pass_id = pass_id;
    command->u.set_scissor.scissor[0] = x;
    command->u.set_scissor.scissor[1] = y;
    command->u.set_scissor.scissor[2] = width;
    command->u.set_scissor.scissor[3] = height;
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
    return dvz_drp2_stream_set_bind_group_dynamic(
        stream, pass_id, slot, bind_group_id, 0, NULL);
}


bool dvz_drp2_stream_set_bind_group_dynamic(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t bind_group_id,
    uint32_t dynamic_offset_count, const uint64_t* dynamic_offsets)
{
    if (dynamic_offset_count > DVZ_DRP2_MAX_BINDINGS)
        return false;
    if (dynamic_offset_count > 0)
        ANN(dynamic_offsets);
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    if (command == NULL)
        return false;
    command->u.set_bind_group.pass_id = pass_id;
    command->u.set_bind_group.slot = slot;
    command->u.set_bind_group.bind_group_id = bind_group_id;
    command->u.set_bind_group.dynamic_offset_count = dynamic_offset_count;
    if (dynamic_offset_count > 0)
    {
        dvz_memcpy(
            command->u.set_bind_group.dynamic_offsets,
            sizeof(command->u.set_bind_group.dynamic_offsets), dynamic_offsets,
            dynamic_offset_count * sizeof(uint64_t));
    }
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
 * Append a ResourceBarrier command for a buffer range.
 *
 * @param stream the command stream
 * @param encoder_id the open command encoder id
 * @param buffer_id the buffer id
 * @param src_stage the producer stage
 * @param src_access the producer access
 * @param dst_stage the consumer stage
 * @param dst_access the consumer access
 * @param offset the first byte in the synchronized range
 * @param size the synchronized byte size, or 0 for the rest of the buffer
 * @return whether the command was appended
 */
bool dvz_drp2_stream_resource_barrier(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t buffer_id,
    const char* src_stage, const char* src_access, const char* dst_stage, const char* dst_access,
    uint64_t offset, uint64_t size)
{
    if (src_stage == NULL || src_access == NULL || dst_stage == NULL || dst_access == NULL)
        return false;
    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_RESOURCE_BARRIER);
    if (command == NULL)
        return false;
    command->u.resource_barrier.encoder_id = encoder_id;
    command->u.resource_barrier.buffer_id = buffer_id;
    command->u.resource_barrier.offset = offset;
    command->u.resource_barrier.size = size;
    _copy_label(
        command->u.resource_barrier.src_stage,
        sizeof(command->u.resource_barrier.src_stage), src_stage);
    _copy_label(
        command->u.resource_barrier.src_access,
        sizeof(command->u.resource_barrier.src_access), src_access);
    _copy_label(
        command->u.resource_barrier.dst_stage,
        sizeof(command->u.resource_barrier.dst_stage), dst_stage);
    _copy_label(
        command->u.resource_barrier.dst_access,
        sizeof(command->u.resource_barrier.dst_access), dst_access);
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



bool dvz_drp2_stream_write_buffer_bytes(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const void* data)
{
    ANN(stream);
    /* WebGPU-shaped: size==0 is a valid no-op that does not need to be recorded. */
    if (size == 0)
        return true;
    if (data == NULL || size > SIZE_MAX)
        return false;

    DvzDrp2Command* command = _append_command(stream, DVZ_DRP2_COMMAND_WRITE_BUFFER);
    if (command == NULL)
        return false;
    void* data_copy = dvz_malloc((size_t)size);
    if (data_copy == NULL)
    {
        stream->count--;
        return false;
    }
    dvz_memcpy(data_copy, (size_t)size, data, (size_t)size);
    command->type                          = DVZ_DRP2_COMMAND_WRITE_BUFFER;
    command->u.write_buffer.buffer_id      = buffer_id;
    command->u.write_buffer.offset         = offset;
    command->u.write_buffer.size           = size;
    command->u.write_buffer.data_raw       = data_copy;
    command->u.write_buffer.data_raw_owned = true;
    command->u.write_buffer.data_base64    = NULL; /* populated only for JSON serialization */
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


bool dvz_drp2_stream_write_texture_2d_region_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t origin_x,
    uint32_t origin_y, uint32_t width, uint32_t height, uint32_t bytes_per_row,
    uint32_t rows_per_image, const void* data)
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
    command->u.write_texture.origin_x        = origin_x;
    command->u.write_texture.origin_y        = origin_y;
    command->u.write_texture.origin_z        = 0;
    command->u.write_texture.width           = width;
    command->u.write_texture.height          = height;
    command->u.write_texture.depth           = 1;
    command->u.write_texture.bytes_per_row   = bytes_per_row;
    command->u.write_texture.rows_per_image  = rows_per_image;
    command->u.write_texture.data_raw        = data; /* borrowed; caller keeps alive */
    command->u.write_texture.data_base64     = NULL;
    return true;
}
