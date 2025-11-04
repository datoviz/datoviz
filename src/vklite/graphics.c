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

static inline void set_dynamic(DvzGraphics* graphics, VkDynamicState state)
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



static inline bool is_dynamic(DvzGraphics* graphics, VkDynamicState state)
{
    ANN(graphics);
    for (uint32_t i = 0; i < graphics->dynamic_count; i++)
    {
        if (graphics->dynamic_states[i] == state)
        {
            return true;
        }
    }
    return false;
}



static int get_shader_index(DvzGraphics* graphics, VkShaderStageFlagBits stage)
{
    for (uint32_t i = 0; i < graphics->shader_count; i++)
    {
        if (graphics->shader_stages[i] == stage)
        {
            return (int)i;
        }
    }
    return -1;
}



/*************************************************************************************************/
/*  Pipeline creation helpers                                                                    */
/*************************************************************************************************/

static void set_shaders(DvzGraphics* graphics, VkPipelineShaderStageCreateInfo* shaders)
{
    ANN(graphics);
    ANN(shaders);

    for (uint32_t i = 0; i < graphics->shader_count; i++)
    {
        shaders[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaders[i].module = graphics->shaders[i];
        shaders[i].stage = graphics->shader_stages[i];
        shaders[i].pName = "main";
        shaders[i].pSpecializationInfo = &graphics->spec_info[i];
    }
}



static void set_vertex_input(
    DvzGraphics* graphics, VkPipelineVertexInputStateCreateInfo* vertex_input,
    VkVertexInputBindingDescription* bindings_info, VkVertexInputAttributeDescription* attrs_info)
{
    ANN(graphics);
    ANN(vertex_input);
    ANN(bindings_info);
    ANN(attrs_info);

    vertex_input->vertexBindingDescriptionCount = graphics->vertex_binding_count;
    vertex_input->pVertexBindingDescriptions = graphics->vertex_bindings;

    vertex_input->vertexAttributeDescriptionCount = graphics->vertex_attr_count;
    vertex_input->pVertexAttributeDescriptions = graphics->vertex_attrs;
}



static void set_viewport(DvzGraphics* graphics, VkPipelineViewportStateCreateInfo* viewport)
{
    ANN(graphics);
    ANN(viewport);

    viewport->viewportCount = 1;
    viewport->pViewports = &graphics->viewport;

    viewport->scissorCount = 1;
    viewport->pScissors = &graphics->scissor;
}



static void
set_dynamic_state(DvzGraphics* graphics, VkPipelineDynamicStateCreateInfo* dynamic_state)
{
    ANN(graphics);
    ANN(dynamic_state);

    dynamic_state->dynamicStateCount = graphics->dynamic_count;
    dynamic_state->pDynamicStates = graphics->dynamic_states;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_graphics(DvzDevice* device, DvzGraphics* graphics)
{
    ANN(device);
    ANN(graphics);
    graphics->device = device;

    graphics->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    graphics->rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    graphics->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    graphics->blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    graphics->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    graphics->rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    // Default values.
    graphics->rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    graphics->rasterization.cullMode = VK_CULL_MODE_NONE;
    graphics->rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    graphics->rasterization.lineWidth = 1.0f;
    graphics->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    graphics->blend.logicOpEnable = VK_FALSE;
    graphics->blend.logicOp = VK_LOGIC_OP_COPY;

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
    DvzGraphics* graphics, VkShaderStageFlagBits stage, uint32_t index, DvzSize offset,
    DvzSize size, void* data)
{
    ANN(graphics);
    ANN(data);
    int shader_idx = get_shader_index(graphics, stage);
    if (shader_idx < 0)
    {
        log_error("could not find shader stage %d in graphics pipeline", stage);
        return;
    }
    ASSERT(shader_idx >= 0);

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

    graphics->spec_entries[shader_idx][index].constantID = index;
    graphics->spec_entries[shader_idx][index].offset = offset;
    graphics->spec_entries[shader_idx][index].size = size;

    VkSpecializationInfo* spec_info = &graphics->spec_info[shader_idx];
    ANN(spec_info);
    spec_info->mapEntryCount = MAX(spec_info->mapEntryCount, index + 1);
    spec_info->dataSize = MAX(spec_info->dataSize, offset + size);
    ASSERT(spec_info->dataSize <= DVZ_MAX_SPEC_CONST_SIZE);

    // Copy the value in the specialization constant data buffer.
    dvz_memcpy(&graphics->spec_data[offset], size, data, size);
}



void dvz_graphics_attachment_color(DvzGraphics* graphics, uint32_t idx, VkFormat format)
{
    ANN(graphics);
    graphics->attachments_colors[idx] = format;
    graphics->rendering.colorAttachmentCount =
        MAX(graphics->rendering.colorAttachmentCount, idx + 1);
}



void dvz_graphics_attachment_depth(DvzGraphics* graphics, VkFormat format)
{
    ANN(graphics);
    graphics->rendering.depthAttachmentFormat = format;
}



void dvz_graphics_attachment_stencil(DvzGraphics* graphics, VkFormat format)
{
    ANN(graphics);
    graphics->rendering.stencilAttachmentFormat = format;
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
    graphics->input_assembly.topology = topology;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);
    }
}



void dvz_graphics_primitive_restart(DvzGraphics* graphics, int flags)
{
    ANN(graphics);
    graphics->input_assembly.primitiveRestartEnable = flags != 0;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE);
    }
}



void dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode, int flags)
{
    ANN(graphics);
    graphics->rasterization.polygonMode = polygon_mode;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        // NOTE: requires extensions, not core in Vulkan 1.3
        set_dynamic(graphics, VK_DYNAMIC_STATE_POLYGON_MODE_EXT);
    }
}



void dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode, int flags)
{
    ANN(graphics);
    graphics->rasterization.cullMode = cull_mode;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_CULL_MODE);
    }
}



void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face, int flags)
{
    ANN(graphics);
    graphics->rasterization.frontFace = front_face;

    if ((flags & DVZ_GRAPHICS_FLAGS_DYNAMIC) != 0)
    {
        set_dynamic(graphics, VK_DYNAMIC_STATE_FRONT_FACE);
    }
}



void dvz_graphics_depth(
    DvzGraphics* graphics, bool clamp, bool depth_write, VkCompareOp compare, int flags)
{
    ANN(graphics);

    graphics->depth_stencil.depthTestEnable = flags != 0;
    graphics->depth_stencil.depthWriteEnable = depth_write;
    graphics->depth_stencil.depthCompareOp = compare;

    graphics->rasterization.depthClampEnable = clamp;

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
    graphics->depth_stencil.depthBoundsTestEnable = flags != 0;
    graphics->depth_stencil.minDepthBounds = min;
    graphics->depth_stencil.maxDepthBounds = max;

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
    graphics->rasterization.depthBiasEnable = flags != 0;
    graphics->rasterization.depthBiasConstantFactor = constant_factor;
    graphics->rasterization.depthBiasClamp = clamp;
    graphics->rasterization.depthBiasSlopeFactor = slope_factor;

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

    graphics->depth_stencil.stencilTestEnable = flags != 0;

    VkStencilOpState* state = NULL;
    if ((mask & VK_STENCIL_FACE_FRONT_BIT) != 0)
        state = &graphics->depth_stencil.front;
    else if ((mask & VK_STENCIL_FACE_BACK_BIT) != 0)
        state = &graphics->depth_stencil.back;
    ANN(state);

    state->compareMask = compare_mask;
    state->compareOp = compare;
    state->depthFailOp = depth_fail;
    state->failOp = fail;
    state->passOp = pass;
    state->reference = reference;
    state->writeMask = write_mask;

    if (((mask & VK_STENCIL_FACE_FRONT_BIT) != 0) && ((mask & VK_STENCIL_FACE_BACK_BIT) != 0))
    {
        graphics->depth_stencil.back = graphics->depth_stencil.front;
    }

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
    graphics->blend.logicOpEnable = flags != 0;
    graphics->blend.logicOp = op;
    glm_vec4_copy(constants, graphics->blend.blendConstants);

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
    graphics->blend_attachments[idx].srcColorBlendFactor = src;
    graphics->blend_attachments[idx].dstColorBlendFactor = dst;
    graphics->blend_attachments[idx].colorBlendOp = op;
    graphics->blend_attachments[idx].colorWriteMask = mask;
}



void dvz_graphics_blend_alpha(
    DvzGraphics* graphics, uint32_t idx, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op)
{
    ANN(graphics);
    graphics->blend_attachments[idx].srcAlphaBlendFactor = src;
    graphics->blend_attachments[idx].dstAlphaBlendFactor = dst;
    graphics->blend_attachments[idx].alphaBlendOp = op;
}



void dvz_graphics_multisampling(
    DvzGraphics* graphics, VkSampleCountFlagBits samples, float min_sample_shading,
    bool alpha_coverage)
{
    ANN(graphics);
    graphics->multisampling.sampleShadingEnable = samples != VK_SAMPLE_COUNT_1_BIT;
    graphics->multisampling.rasterizationSamples = samples;
    graphics->multisampling.minSampleShading = min_sample_shading;
    graphics->multisampling.alphaToCoverageEnable = alpha_coverage;
}



int dvz_graphics_create(DvzGraphics* graphics)
{
    ANN(graphics);

    DvzDevice* device = graphics->device;
    ANN(device);

    // Create the pipeline.
    VkGraphicsPipelineCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Shaders.
    VkPipelineShaderStageCreateInfo shaders[DVZ_MAX_SHADERS] = {0};
    set_shaders(graphics, shaders);
    info.stageCount = graphics->shader_count;
    info.pStages = shaders;

    // Vertex input.
    VkVertexInputBindingDescription bindings_info[DVZ_MAX_VERTEX_BINDINGS] = {0};
    VkVertexInputAttributeDescription attrs_info[DVZ_MAX_VERTEX_ATTRS] = {0};
    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    set_vertex_input(graphics, &vertex_input, bindings_info, attrs_info);
    info.pVertexInputState = &vertex_input;

    // Input assembly.
    info.pInputAssemblyState = &graphics->input_assembly;

    // Rasterization.
    info.pRasterizationState = &graphics->rasterization;

    // Attachments and blending.
    graphics->blend.attachmentCount = graphics->rendering.colorAttachmentCount;
    graphics->blend.pAttachments = graphics->blend_attachments;
    info.pColorBlendState = &graphics->blend;

    // Depth stencil.
    info.pDepthStencilState = &graphics->depth_stencil;

    // Multisampling.
    info.pMultisampleState = &graphics->multisampling;

    // Viewport.
    VkPipelineViewportStateCreateInfo viewport = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    set_viewport(graphics, &viewport);
    info.pViewportState = &viewport;

    // Dynamic states.
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    set_dynamic_state(graphics, &dynamic_state);
    info.pDynamicState = &dynamic_state;

    // Dynamic rendering.
    // TODO: need to enable dynamic rendering Vulkan 1.3 feature when creating the device.
    graphics->rendering.pColorAttachmentFormats = graphics->attachments_colors;
    info.pNext = &graphics->rendering;

    // Pipeline layout.
    info.layout = graphics->layout;

    // Creation.
    log_trace("creating graphics pipeline...");
    VK_RETURN_RESULT(vkCreateGraphicsPipelines(
        device->vk_device, VK_NULL_HANDLE, 1, &info, NULL, &graphics->vk_pipeline));
    if (out == 0)
    {
        dvz_obj_created(&graphics->obj);
        log_trace("graphics created");
    }
    return out;
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

    ASSERT(idx < cmds->count);
    VkCommandBuffer cmd = cmds->cmds[idx];
    ANNVK(cmd);

    // Bind the graphics pipeline.
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics->vk_pipeline);

    // Dynamic states.
    // TODO: go through all dynamic states and call the relevant vkCmdSet*() commands.

    // Primitive topology.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY))
    {
        vkCmdSetPrimitiveTopology(cmd, graphics->input_assembly.topology);
    }

    // Primitive restart.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE))
    {
        vkCmdSetPrimitiveRestartEnable(cmd, graphics->input_assembly.primitiveRestartEnable);
    }

    // Polygon mode.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_POLYGON_MODE_EXT))
    {
        vkCmdSetPolygonModeEXT(cmd, graphics->rasterization.polygonMode);
    }

    // Cull mode.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_CULL_MODE))
    {
        vkCmdSetCullMode(cmd, graphics->rasterization.cullMode);
    }

    // Front face.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_FRONT_FACE))
    {
        vkCmdSetFrontFace(cmd, graphics->rasterization.frontFace);
    }

    // Depth test.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE))
    {
        vkCmdSetDepthTestEnable(cmd, graphics->depth_stencil.depthTestEnable);
        vkCmdSetDepthWriteEnable(cmd, graphics->depth_stencil.depthWriteEnable);
        vkCmdSetDepthCompareOp(cmd, graphics->depth_stencil.depthCompareOp);
    }

    // Depth bounds test.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE))
    {
        vkCmdSetDepthBoundsTestEnable(cmd, graphics->depth_stencil.depthBoundsTestEnable);
        vkCmdSetDepthBounds(
            cmd, graphics->depth_stencil.minDepthBounds, graphics->depth_stencil.maxDepthBounds);
    }

    // Depth bias.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE))
    {
        vkCmdSetDepthBiasEnable(cmd, graphics->rasterization.depthBiasEnable);
        vkCmdSetDepthBias(
            cmd, graphics->rasterization.depthBiasConstantFactor,
            graphics->rasterization.depthBiasClamp, graphics->rasterization.depthBiasSlopeFactor);
    }

    // Stencil test.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE))
    {
        vkCmdSetStencilTestEnable(cmd, graphics->depth_stencil.stencilTestEnable);

        // Front and back are identical.
        if (memcmp(
                &graphics->depth_stencil.front, &graphics->depth_stencil.back,
                sizeof(graphics->depth_stencil.front)))
        {
            vkCmdSetStencilOp(
                cmd, VK_STENCIL_FACE_FRONT_AND_BACK, graphics->depth_stencil.front.failOp,
                graphics->depth_stencil.front.passOp, graphics->depth_stencil.front.depthFailOp,
                graphics->depth_stencil.front.compareOp);
            vkCmdSetStencilReference(
                cmd, VK_STENCIL_FACE_FRONT_AND_BACK, graphics->depth_stencil.front.reference);
            vkCmdSetStencilCompareMask(
                cmd, VK_STENCIL_FACE_FRONT_AND_BACK, graphics->depth_stencil.front.compareMask);
            vkCmdSetStencilWriteMask(
                cmd, VK_STENCIL_FACE_FRONT_AND_BACK, graphics->depth_stencil.front.writeMask);
        }

        // Front and back are different.
        else
        {
            // Front.
            vkCmdSetStencilOp(
                cmd, VK_STENCIL_FACE_FRONT_BIT, graphics->depth_stencil.front.failOp,
                graphics->depth_stencil.front.passOp, graphics->depth_stencil.front.depthFailOp,
                graphics->depth_stencil.front.compareOp);
            vkCmdSetStencilReference(
                cmd, VK_STENCIL_FACE_FRONT_BIT, graphics->depth_stencil.front.reference);
            vkCmdSetStencilCompareMask(
                cmd, VK_STENCIL_FACE_FRONT_BIT, graphics->depth_stencil.front.compareMask);
            vkCmdSetStencilWriteMask(
                cmd, VK_STENCIL_FACE_FRONT_BIT, graphics->depth_stencil.front.writeMask);

            // Back.
            vkCmdSetStencilOp(
                cmd, VK_STENCIL_FACE_BACK_BIT, graphics->depth_stencil.back.failOp,
                graphics->depth_stencil.back.passOp, graphics->depth_stencil.back.depthFailOp,
                graphics->depth_stencil.back.compareOp);
            vkCmdSetStencilReference(
                cmd, VK_STENCIL_FACE_BACK_BIT, graphics->depth_stencil.back.reference);
            vkCmdSetStencilCompareMask(
                cmd, VK_STENCIL_FACE_BACK_BIT, graphics->depth_stencil.back.compareMask);
            vkCmdSetStencilWriteMask(
                cmd, VK_STENCIL_FACE_BACK_BIT, graphics->depth_stencil.back.writeMask);
        }
    }

    // Scissor.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_SCISSOR))
    {
        vkCmdSetScissor(cmd, 0, 1, &graphics->scissor);
    }

    // Viewport.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_VIEWPORT))
    {
        vkCmdSetViewport(cmd, 0, 1, &graphics->viewport);
    }

    // Blend constants.
    if (is_dynamic(graphics, VK_DYNAMIC_STATE_BLEND_CONSTANTS))
    {
        vkCmdSetBlendConstants(cmd, graphics->blend.blendConstants);
    }
}
