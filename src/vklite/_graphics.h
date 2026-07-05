/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graphics internals                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "obj.h"
#include "datoviz/vklite/graphics.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGraphics
{
    DvzObject obj;
    DvzDevice* device;

    uint32_t shader_count;
    VkShaderStageFlagBits shader_stages[DVZ_MAX_SHADERS];
    VkShaderModule shaders[DVZ_MAX_SHADERS];
    VkPipelineLayout layout;

    VkSpecializationMapEntry spec_entries[DVZ_MAX_SHADERS][DVZ_MAX_SPEC_CONST];
    VkSpecializationInfo spec_info[DVZ_MAX_SHADERS];
    unsigned char spec_data[DVZ_MAX_SHADERS][DVZ_MAX_SPEC_CONST_SIZE];

    uint32_t vertex_binding_count;
    VkVertexInputBindingDescription vertex_bindings[DVZ_MAX_VERTEX_BINDINGS];

    uint32_t vertex_attr_count;
    VkVertexInputAttributeDescription vertex_attrs[DVZ_MAX_VERTEX_ATTRS];

    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;

    VkFormat attachments_colors[DVZ_MAX_ATTACHMENTS];
    VkPipelineRenderingCreateInfo rendering;

    VkPipelineColorBlendAttachmentState blend_attachments[DVZ_MAX_ATTACHMENTS];
    VkPipelineColorBlendStateCreateInfo blend;

    VkRect2D scissor;
    VkViewport viewport;

    VkPipelineMultisampleStateCreateInfo multisampling;

    uint32_t dynamic_count;
    VkDynamicState dynamic_states[DVZ_MAX_DYNAMIC_STATES];

    VkPipeline vk_pipeline;
};
