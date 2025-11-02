/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/vklite/graphics.h"
#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "cglm/include/cglm/vec4.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/vec.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "types.h"
#include <vulkan/vulkan_core.h>



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void set_dynamic(DvzGraphics* graphics, VkDynamicState state)
{
    // TODO: for now, once a state has been set as dynamic, it can't be set back as fixed.
    ANN(graphics);
    for (uint32_t i = 0; i < graphics->dynamic_count; i++)
    {
        if (graphics->dynamic_states[i] == state)
        {
            return;
        }
    }
    uint32_t i = graphics->dynamic_count++;
    graphics->dynamic_states[i] = state;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_graphics(DvzDevice* device, DvzGraphics* graphics)
{
    ANN(device);
    ANN(graphics);
    graphics->device = device;
    dvz_obj_init(&graphics->obj);
}



void dvz_graphics_shader(DvzGraphics* graphics, VkShaderStageFlagBits stage, VkShaderModule shader)
{
    ANN(graphics);
    for (uint32_t i = 0; i < graphics->shader_count; i++)
    {
        if (graphics->shader_stages[i] == stage)
        {
            graphics->shaders[i] = shader;
            return;
        }
    }
    uint32_t i = graphics->shader_count++;
    graphics->shader_stages[i] = stage;
    graphics->shaders[i] = shader;
}



void dvz_graphics_spec(
    DvzGraphics* graphics, uint32_t index, DvzSize offset, DvzSize size, void* data)
{
    ANN(graphics);
    ANN(data);

    if (index >= DVZ_MAX_SPEC_CONST)
    {
        log_error(
            "there can be no more than %d specialization constants per pipeline",
            DVZ_MAX_SPEC_CONST);
        return;
    }

    if (offset + size >= DVZ_MAX_SPEC_CONST_SIZE)
    {
        log_error(
            "the specialization constant data buffer can be no more than %s",
            dvz_pretty_size(DVZ_MAX_SPEC_CONST_SIZE));
        return;
    }

    graphics->spec_entries[index].constantID = index;
    graphics->spec_entries[index].offset = offset;
    graphics->spec_entries[index].size = size;

    graphics->spec_info.mapEntryCount = MAX(graphics->spec_info.mapEntryCount, index + 1);
    graphics->spec_info.dataSize = MAX(graphics->spec_info.dataSize, offset + size);
    ASSERT(graphics->spec_info.dataSize <= DVZ_MAX_SPEC_CONST_SIZE);

    // Copy the value in the specialization constant data buffer.
    dvz_memcpy(&graphics->spec_data[offset], size, data, size);
}



void dvz_graphics_attachment_color(DvzGraphics* graphics, uint32_t idx, VkFormat format)
{
    ANN(graphics);
    graphics->attachments_colors[idx] = format;
}



void dvz_graphics_attachment_depth(DvzGraphics* graphics, VkFormat format)
{
    ANN(graphics);
    graphics->attachments_depth = format;
}



void dvz_graphics_attachment_stencil(DvzGraphics* graphics, VkFormat format)
{
    ANN(graphics);
    graphics->attachments_stencil = format;
}



void dvz_graphics_vertex_binding(
    DvzGraphics* graphics, uint32_t binding, DvzSize stride, VkVertexInputRate input_rate)
{
    ANN(graphics);
    for (uint32_t i = 0; i < graphics->vertex_binding_count; i++)
    {
        if (graphics->vertex_bindings[i].binding == binding)
        {
            graphics->vertex_bindings[i].stride = stride;
            graphics->vertex_bindings[i].inputRate = input_rate;
            return;
        }
    }
    uint32_t i = graphics->vertex_binding_count++;
    graphics->vertex_bindings[i].binding = binding;
    graphics->vertex_bindings[i].stride = stride;
    graphics->vertex_bindings[i].inputRate = input_rate;
}



void dvz_graphics_vertex_attr(
    DvzGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format, DvzSize offset)
{
    ANN(graphics);
    ANN(graphics);
    for (uint32_t i = 0; i < graphics->vertex_attr_count; i++)
    {
        if (graphics->vertex_attrs[i].binding == binding &&
            graphics->vertex_attrs[i].location == location)
        {
            graphics->vertex_attrs[i].format = format;
            graphics->vertex_attrs[i].offset = offset;
            return;
        }
    }
    uint32_t i = graphics->vertex_attr_count++;
    graphics->vertex_attrs[i].binding = binding;
    graphics->vertex_attrs[i].location = location;
    graphics->vertex_attrs[i].format = format;
    graphics->vertex_attrs[i].offset = offset;
}



void dvz_graphics_layout(DvzGraphics* graphics, VkPipelineLayout layout)
{
    ANN(graphics);
    graphics->layout = layout;
}



void dvz_graphics_primitive(DvzGraphics* graphics, VkPrimitiveTopology topology, int flags)
{
    ANN(graphics);
    graphics->topology = topology;
}



void dvz_graphics_primitive_restart(DvzGraphics* graphics, int flags)
{
    ANN(graphics);
    graphics->primitive_restart = flags != 0;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE);
    }
}



void dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode, int flags)
{
    ANN(graphics);
    graphics->polygon_mode = polygon_mode;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        log_warn("polygon mode dynamic state not implemented yet, requires extension");
        // set_dynamic(graphics, VK_DYNAMIC_STATE_POLYGON_MODE_EXT);
    }
}



void dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode, int flags)
{
    ANN(graphics);
    graphics->cull_mode = cull_mode;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_CULL_MODE);
    }
}



void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face, int flags)
{
    ANN(graphics);
    graphics->front_face = front_face;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_FRONT_FACE);
    }
}



void dvz_graphics_depth(DvzGraphics* graphics, bool depth_write, VkCompareOp compare, int flags)
{
    ANN(graphics);
    graphics->depth_test = flags != 0;
    graphics->depth_write = depth_write;
    graphics->depth_compare = compare;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE);
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP);
    }
}



void dvz_graphics_depth_bounds(DvzGraphics* graphics, float min, float max, int flags)
{
    ANN(graphics);
    graphics->depth_bounds_test = flags != 0;
    graphics->depth_bounds[0] = min;
    graphics->depth_bounds[0] = max;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE);
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    }
}



void dvz_graphics_depth_bias(
    DvzGraphics* graphics, float constant_factor, float clamp, float slope_factor, int flags)
{
    ANN(graphics);
    graphics->depth_bias = flags != 0;
    graphics->depth_constant = constant_factor;
    graphics->depth_clamp = clamp;
    graphics->depth_slope = slope_factor;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE);
        set_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BIAS);
    }
}



void dvz_graphics_stencil(
    DvzGraphics* graphics, VkStencilFaceFlags mask, VkStencilOp fail, VkStencilOp pass,
    VkStencilOp depth_fail, VkCompareOp compare, uint32_t compare_mask, uint32_t write_mask,
    uint32_t reference, int flags)
{
    ANN(graphics);
    graphics->stencil_test = flags != 0;
    graphics->stencil_mask = mask;
    graphics->stencil_fail = fail;
    graphics->stencil_depth_fail = depth_fail;
    graphics->stencil_compare = compare;
    graphics->stencil_compare_mask = compare_mask;
    graphics->stencil_write_mask = write_mask;
    graphics->stencil_reference = reference;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE);
        set_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_OP);
        set_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
        set_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
        set_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }
}



void dvz_graphics_scissor(
    DvzGraphics* graphics, int32_t x, int32_t y, uint32_t width, uint32_t height, int flags)
{
    ANN(graphics);
    graphics->scissor =
        (VkRect2D){.offset.x = x, .offset.y = y, .extent.width = width, .extent.height = height};

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_SCISSOR);
    }
}



void dvz_graphics_viewport(
    DvzGraphics* graphics, float x, float y, float width, float height, float min_depth,
    float max_depth, int flags)
{
    ANN(graphics);
    graphics->viewport = (VkViewport){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .minDepth = min_depth,
        .maxDepth = max_depth};

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_VIEWPORT);
    }
}



void dvz_graphics_blend(DvzGraphics* graphics, VkLogicOp op, vec4 constants, int flags)
{
    ANN(graphics);
    graphics->blend_enable = flags != 0;
    graphics->blend_op = op;
    glm_vec4_copy(constants, graphics->blend_constants);

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    }
}



void dvz_graphics_blend_color(
    DvzGraphics* graphics, uint32_t idx, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op,
    VkColorComponentFlags mask)
{
    ANN(graphics);
    graphics->blend[idx].srcColorBlendFactor = src;
    graphics->blend[idx].dstColorBlendFactor = dst;
    graphics->blend[idx].colorBlendOp = op;
    graphics->blend[idx].colorWriteMask = mask;
}



void dvz_graphics_blend_alpha(
    DvzGraphics* graphics, uint32_t idx, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op)
{
    ANN(graphics);
    graphics->blend[idx].srcAlphaBlendFactor = src;
    graphics->blend[idx].dstAlphaBlendFactor = dst;
    graphics->blend[idx].alphaBlendOp = op;
}



void dvz_graphics_multisampling(
    DvzGraphics* graphics, VkSampleCountFlagBits samples, float min_sample_shading,
    bool alpha_coverage)
{
    ANN(graphics);
    graphics->msaa_samples = samples;
    graphics->msaa_min_sample_shading = min_sample_shading;
    graphics->msaa_alpha_coverage = alpha_coverage;
}



void dvz_graphics_create(DvzGraphics* graphics)
{
    ANN(graphics);
    // TODOf
}



void dvz_graphics_destroy(DvzGraphics* graphics)
{
    ANN(graphics);
    ANN(graphics->device);

    if (!dvz_obj_is_created(&graphics->obj))
    {
        log_trace("skip destruction of already-destroyed graphics");
        return;
    }

    if (graphics->vk_pipeline != VK_NULL_HANDLE)
    {
        log_trace("destroying graphics...");
        vkDestroyPipeline(dvz_device_handle(graphics->device), graphics->vk_pipeline, NULL);
        graphics->vk_pipeline = VK_NULL_HANDLE;
        log_trace("graphics destroyed");
    }

    dvz_obj_destroyed(&graphics->obj);
}



void dvz_cmd_bind_graphics(DvzCommands* cmds, uint32_t idx, DvzGraphics* graphics)
{
    ANN(cmds);
    ANN(graphics);
    vkCmdBindPipeline(cmds->cmds[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics->vk_pipeline);
}
