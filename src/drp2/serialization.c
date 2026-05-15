/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command stream serialization                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_json.h"
#include "_stream.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return "SetViewport";
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return "SetScissor";
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


static const char* _vertex_format_name(uint32_t format)
{
    switch (format)
    {
    case VK_FORMAT_R32_SFLOAT:
        return "float32";
    case VK_FORMAT_R32G32_SFLOAT:
        return "float32x2";
    case VK_FORMAT_R32G32B32_SFLOAT:
        return "float32x3";
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return "float32x4";
    case VK_FORMAT_R8G8B8A8_UNORM:
        return "unorm8x4";
    default:
        return NULL;
    }
}



static const char* _texture_format_name(uint32_t format)
{
    switch (format)
    {
    case 0:
    case VK_FORMAT_R8G8B8A8_UNORM:
        return "rgba8unorm";
    case VK_FORMAT_B8G8R8A8_UNORM:
        return "bgra8unorm";
    case VK_FORMAT_R16_SFLOAT:
        return "r16float";
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return "rgba16float";
    case VK_FORMAT_R32_SFLOAT:
        return "r32float";
    case VK_FORMAT_R32_UINT:
        return "r32uint";
    case VK_FORMAT_R32_SINT:
        return "r32sint";
    case VK_FORMAT_R32G32_UINT:
        return "rg32uint";
    case VK_FORMAT_D32_SFLOAT:
        return "depth32float";
    default:
        return "rgba8unorm";
    }
}



static const char* _topology_name(uint32_t topology)
{
    switch (topology)
    {
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
        return "point-list";
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
        return "line-list";
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        return "line-strip";
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return "triangle-list";
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        return "triangle-strip";
    default:
        return "triangle-list";
    }
}



static const char* _step_mode_name(uint32_t step_mode)
{
    switch (step_mode)
    {
    case DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE:
        return "instance";
    case DVZ_DRP2_VERTEX_STEP_MODE_VERTEX:
    default:
        return "vertex";
    }
}


static const char* _binding_type_name(DvzDrp2BindingType type)
{
    switch (type)
    {
    case DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER:
        return "uniform_buffer";
    case DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER:
        return "storage_buffer";
    case DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE:
        return "sampled_texture";
    case DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE:
        return "storage_texture";
    case DVZ_DRP2_BINDING_TYPE_SAMPLER:
        return "sampler";
    case DVZ_DRP2_BINDING_TYPE_NONE:
    default:
        return "none";
    }
}



static const char* _resource_kind_name(DvzDrp2BindingResourceKind kind)
{
    switch (kind)
    {
    case DVZ_DRP2_BINDING_RESOURCE_BUFFER:
        return "buffer";
    case DVZ_DRP2_BINDING_RESOURCE_TEXTURE:
        return "texture";
    case DVZ_DRP2_BINDING_RESOURCE_TEXTURE_VIEW:
        return "texture_view";
    case DVZ_DRP2_BINDING_RESOURCE_SAMPLER:
        return "sampler";
    case DVZ_DRP2_BINDING_RESOURCE_NONE:
    default:
        return "none";
    }
}



static const char* _access_name(DvzDrp2BindingAccess access)
{
    switch (access)
    {
    case DVZ_DRP2_BINDING_ACCESS_READ:
        return "read";
    case DVZ_DRP2_BINDING_ACCESS_READ_WRITE:
    default:
        return "read_write";
    }
}



static void _append_visibility(JsonBuilder* builder, uint32_t visibility)
{
    bool first = true;
    _json_append(builder, "\"visibility\": [");
    if ((visibility & DVZ_DRP2_SHADER_STAGE_VERTEX) != 0)
    {
        _json_append(builder, "\"VERTEX\"");
        first = false;
    }
    if ((visibility & DVZ_DRP2_SHADER_STAGE_FRAGMENT) != 0)
    {
        _json_append(builder, "%s\"FRAGMENT\"", first ? "" : ", ");
        first = false;
    }
    if ((visibility & DVZ_DRP2_SHADER_STAGE_COMPUTE) != 0)
        _json_append(builder, "%s\"COMPUTE\"", first ? "" : ", ");
    _json_append(builder, "]");
}



static void _json_append_vertex_buffers(JsonBuilder* builder, const DvzDrp2Command* command)
{
    ANN(builder);
    ANN(command);

    if (command->u.create_render_pipeline.binding_count == 0)
    {
        if (command->u.create_render_pipeline.vertex_buffer_slots == 0)
            _json_append(builder, ", \"vertex_buffers\": []");
        return;
    }

    _json_append(
        builder, ", \"topology\": \"%s\"",
        _topology_name(command->u.create_render_pipeline.topology));
    _json_append(builder, ", \"vertex_buffers\": [");
    for (uint32_t b = 0; b < command->u.create_render_pipeline.binding_count; b++)
    {
        if (b > 0)
            _json_append(builder, ", ");
        _json_append(
            builder,
            "{ \"array_stride\": %" PRIu32 ", \"step_mode\": \"%s\", \"attributes\": [",
            command->u.create_render_pipeline.binding_strides[b],
            _step_mode_name(command->u.create_render_pipeline.binding_step_modes[b]));

        bool first_attr = true;
        for (uint32_t a = 0; a < command->u.create_render_pipeline.attr_count; a++)
        {
            if (command->u.create_render_pipeline.attr_bindings[a] != b)
                continue;
            const char* format =
                _vertex_format_name(command->u.create_render_pipeline.attr_formats[a]);
            if (format == NULL)
                continue;
            if (!first_attr)
                _json_append(builder, ", ");
            first_attr = false;
            _json_append(
                builder,
                "{ \"shader_location\": %" PRIu32 ", \"offset\": %" PRIu32
                ", \"format\": \"%s\" }",
                command->u.create_render_pipeline.attr_locations[a],
                command->u.create_render_pipeline.attr_offsets[a], format);
        }
        _json_append(builder, "] }");
    }
    _json_append(builder, "]");
}



/**
 * Append a DRP2 color write mask as a channel-name array.
 *
 * @param builder the JSON builder
 * @param color_write_mask the internal Vulkan color component bitmask
 */
static void _json_append_color_write_mask(JsonBuilder* builder, uint32_t color_write_mask)
{
    ANN(builder);
    uint32_t mask = color_write_mask == 0 ? 0xFu : color_write_mask;
    bool first = true;

    _json_append(builder, "[");
    if ((mask & VK_COLOR_COMPONENT_R_BIT) != 0)
    {
        _json_append(builder, "\"red\"");
        first = false;
    }
    if ((mask & VK_COLOR_COMPONENT_G_BIT) != 0)
    {
        _json_append(builder, "%s\"green\"", first ? "" : ", ");
        first = false;
    }
    if ((mask & VK_COLOR_COMPONENT_B_BIT) != 0)
    {
        _json_append(builder, "%s\"blue\"", first ? "" : ", ");
        first = false;
    }
    if ((mask & VK_COLOR_COMPONENT_A_BIT) != 0)
        _json_append(builder, "%s\"alpha\"", first ? "" : ", ");
    _json_append(builder, "]");
}



/**
 * Append the default color target metadata for a render pipeline.
 *
 * @param builder the JSON builder
 * @param command the CreateRenderPipeline command
 */
static void _json_append_color_targets(JsonBuilder* builder, const DvzDrp2Command* command)
{
    ANN(builder);
    ANN(command);
    uint32_t count = command->u.create_render_pipeline.color_target_count;
    if (count == 0)
        count = 1;
    _json_append(builder, ", \"color_targets\": [");
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2ColorTarget* target = &command->u.create_render_pipeline.color_targets[i];
        const char* format = "rgba8unorm";
        if (target->format == VK_FORMAT_R16G16B16A16_SFLOAT)
            format = "rgba16float";
        else if (target->format == VK_FORMAT_R16_SFLOAT)
            format = "r16float";
        if (i > 0)
            _json_append(builder, ", ");
        _json_append(builder, "{ \"format\": \"%s\", \"write_mask\": ", format);
        _json_append_color_write_mask(builder, target->color_write_mask);
        if (target->blend_enabled)
        {
            _json_append(
                builder,
                ", \"blend\": { \"color\": { \"src_factor\": %" PRIu32
                ", \"dst_factor\": %" PRIu32 ", \"operation\": %" PRIu32
                " }, \"alpha\": { \"src_factor\": %" PRIu32 ", \"dst_factor\": %" PRIu32
                ", \"operation\": %" PRIu32 " } }",
                target->src_color_blend_factor, target->dst_color_blend_factor,
                target->color_blend_op, target->src_alpha_blend_factor,
                target->dst_alpha_blend_factor, target->alpha_blend_op);
        }
        _json_append(builder, " }");
    }
    _json_append(builder, "]");
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



/**
 * Return the JSON token used for one Vulkan depth compare operation.
 *
 * @param compare_op the VkCompareOp value
 * @return the JSON token string
 */
static const char* _depth_compare_name(uint32_t compare_op)
{
    switch ((VkCompareOp)compare_op)
    {
    case VK_COMPARE_OP_NEVER:
        return "never";
    case VK_COMPARE_OP_LESS:
        return "less";
    case VK_COMPARE_OP_EQUAL:
        return "equal";
    case VK_COMPARE_OP_LESS_OR_EQUAL:
        return "less-or-equal";
    case VK_COMPARE_OP_GREATER:
        return "greater";
    case VK_COMPARE_OP_NOT_EQUAL:
        return "not-equal";
    case VK_COMPARE_OP_GREATER_OR_EQUAL:
        return "greater-or-equal";
    case VK_COMPARE_OP_ALWAYS:
    default:
        return "always";
    }
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
            ", \"depth\": %" PRIu32 ", \"format\": \"%s\", \"usage\": ",
            _command_name(command->type), command->u.create_texture.id,
            command->u.create_texture.depth > 1 ? "3d" : "2d",
            command->u.create_texture.width, command->u.create_texture.height,
            command->u.create_texture.depth > 0 ? command->u.create_texture.depth : 1,
            _texture_format_name(command->u.create_texture.format));
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
        if (command->u.create_render_pipeline.bind_group_layout_count > 0)
        {
            _json_append(builder, ", \"bind_group_layout_ids\": [");
            for (uint32_t i = 0; i < command->u.create_render_pipeline.bind_group_layout_count; i++)
            {
                if (i > 0)
                    _json_append(builder, ", ");
                _json_append(
                    builder, "%" PRIu64,
                    command->u.create_render_pipeline.bind_group_layout_ids[i]);
            }
            _json_append(builder, "]");
        }
        _json_append_vertex_buffers(builder, command);
        _json_append_color_targets(builder, command);
        if (command->u.create_render_pipeline.has_depth_attachment)
        {
            _json_append(
                builder,
                ", \"depth_stencil\": { \"format\": \"depth32float\", "
                "\"depth_write_enabled\": %s, \"depth_compare\": \"%s\" }",
                command->u.create_render_pipeline.depth_write_enabled ? "true" : "false",
                _depth_compare_name(command->u.create_render_pipeline.depth_compare_op));
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
        if (command->u.create_compute_pipeline.bind_group_layout_count > 0)
        {
            _json_append(builder, ", \"bind_group_layout_ids\": [");
            for (uint32_t i = 0; i < command->u.create_compute_pipeline.bind_group_layout_count; i++)
            {
                if (i > 0)
                    _json_append(builder, ", ");
                _json_append(
                    builder, "%" PRIu64,
                    command->u.create_compute_pipeline.bind_group_layout_ids[i]);
            }
            _json_append(builder, "]");
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
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"entries\": [",
            _command_name(command->type), command->u.create_bind_group_layout.id);
        for (uint32_t i = 0; i < command->u.create_bind_group_layout.entry_count; i++)
        {
            const DvzDrp2BindGroupLayoutEntry* entry =
                &command->u.create_bind_group_layout.entries[i];
            if (i > 0)
                _json_append(builder, ", ");
            _json_append(
                builder, "{ \"binding\": %" PRIu32 ", \"binding_type\": \"%s\"",
                entry->binding, _binding_type_name(entry->binding_type));
            if (entry->visibility != 0)
            {
                _json_append(builder, ", ");
                _append_visibility(builder, entry->visibility);
            }
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER ||
                entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
            {
                _json_append(builder, ", \"access\": \"%s\"", _access_name(entry->access));
            }
            if (entry->has_dynamic_offset)
                _json_append(builder, ", \"has_dynamic_offset\": true");
            _json_append(builder, " }");
        }
        _json_append(builder, "] }");
        break;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"bind_group_layout_id\": %" PRIu64
            ", \"entries\": [",
            _command_name(command->type), command->u.create_bind_group.id,
            command->u.create_bind_group.bind_group_layout_id);
        for (uint32_t i = 0; i < command->u.create_bind_group.entry_count; i++)
        {
            const DvzDrp2BindGroupEntry* entry = &command->u.create_bind_group.entries[i];
            if (i > 0)
                _json_append(builder, ", ");
            _json_append(
                builder,
                "{ \"binding\": %" PRIu32 ", \"binding_type\": \"%s\", "
                "\"resource_kind\": \"%s\", \"resource_id\": %" PRIu64,
                entry->binding, _binding_type_name(entry->binding_type),
                _resource_kind_name(entry->resource_kind), entry->resource_id);
            if (entry->resource_kind == DVZ_DRP2_BINDING_RESOURCE_BUFFER)
            {
                _json_append(
                    builder, ", \"offset\": %" PRIu64 ", \"size\": %" PRIu64,
                    entry->offset, entry->size);
            }
            _json_append(builder, " }");
        }
        _json_append(builder, "] }");
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
            ", \"color_attachments\": [ ",
            _command_name(command->type), command->u.begin_render_pass.id,
            command->u.begin_render_pass.encoder_id);
        uint32_t color_count = command->u.begin_render_pass.color_attachment_count;
        if (color_count == 0)
            color_count = 1;
        for (uint32_t i = 0; i < color_count; i++)
        {
            const DvzDrp2ColorAttachment* attachment =
                command->u.begin_render_pass.color_attachment_count > 0
                    ? &command->u.begin_render_pass.color_attachments[i]
                    : NULL;
            uint64_t texture_id =
                attachment != NULL ? attachment->texture_id :
                                     command->u.begin_render_pass.texture_id;
            bool clear =
                attachment != NULL ? attachment->clear : command->u.begin_render_pass.clear;
            const float* clear_color =
                attachment != NULL ? attachment->clear_color :
                                     command->u.begin_render_pass.clear_color;
            if (i > 0)
                _json_append(builder, ", ");
            _json_append(
                builder,
                "{ \"texture_id\": %" PRIu64
                ", \"load_op\": \"%s\", \"store_op\": \"store\", "
                "\"clear_value\": { \"r\": %g, \"g\": %g, \"b\": %g, \"a\": %g } }",
                texture_id, clear ? "clear" : "load", (double)clear_color[0],
                (double)clear_color[1], (double)clear_color[2], (double)clear_color[3]);
        }
        _json_append(builder, " ]");
        if (command->u.begin_render_pass.has_depth_attachment)
        {
            _json_append(
                builder,
                ", \"depth_stencil_attachment\": { \"format\": \"depth32float\", "
                "\"load_op\": \"%s\", \"store_op\": \"store\", "
                "\"clear_value\": { \"depth\": %g } }",
                command->u.begin_render_pass.clear ? "clear" : "load",
                (double)command->u.begin_render_pass.clear_depth);
        }
        _json_append(builder, " }");
        break;
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        _json_append(
            builder, "{ \"cmd\": \"%s\", \"id\": %" PRIu64 ", \"encoder_id\": %" PRIu64 " }",
            _command_name(command->type), command->u.begin_compute_pass.id,
            command->u.begin_compute_pass.encoder_id);
        break;
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"viewport\": { \"x\": %g, \"y\": %g, "
            "\"width\": %g, \"height\": %g } }",
            _command_name(command->type), command->u.set_viewport.pass_id,
            (double)command->u.set_viewport.viewport[0],
            (double)command->u.set_viewport.viewport[1],
            (double)command->u.set_viewport.viewport[2],
            (double)command->u.set_viewport.viewport[3]);
        break;
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        _json_append(
            builder,
            "{ \"cmd\": \"%s\", \"pass_id\": %" PRIu64 ", \"scissor\": { \"x\": %g, \"y\": %g, "
            "\"width\": %g, \"height\": %g } }",
            _command_name(command->type), command->u.set_scissor.pass_id,
            (double)command->u.set_scissor.scissor[0],
            (double)command->u.set_scissor.scissor[1],
            (double)command->u.set_scissor.scissor[2],
            (double)command->u.set_scissor.scissor[3]);
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
            ", \"bind_group_id\": %" PRIu64,
            _command_name(command->type), command->u.set_bind_group.pass_id,
            command->u.set_bind_group.slot, command->u.set_bind_group.bind_group_id);
        if (command->u.set_bind_group.dynamic_offset_count > 0)
        {
            _json_append(builder, ", \"dynamic_offsets\": [");
            for (uint32_t i = 0; i < command->u.set_bind_group.dynamic_offset_count; i++)
            {
                if (i > 0)
                    _json_append(builder, ", ");
                _json_append(builder, "%" PRIu64, command->u.set_bind_group.dynamic_offsets[i]);
            }
            _json_append(builder, "]");
        }
        _json_append(builder, " }");
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
