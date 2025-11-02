/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../vk/types.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/images.h"
#include <volk.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_SWAPCHAIN_IMAGES 4
#define DVZ_MAX_SETS             4
#define DVZ_MAX_BINDINGS         16
#define DVZ_MAX_PUSH_CONSTANTS   8
#define DVZ_MAX_SPEC_CONST       8

// Arbitrarily limit the spec constant data buffer size which simplifies the implementation.
#define DVZ_MAX_SPEC_CONST_SIZE 128



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzQueue DvzQueue;

typedef struct DvzCommands DvzCommands;
typedef struct DvzSampler DvzSampler;
typedef struct DvzCompute DvzCompute;
typedef struct DvzPush DvzPush;
typedef struct DvzSlots DvzSlots;
typedef struct DvzDescriptors DvzDescriptors;
typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferViews DvzBufferViews;
typedef struct DvzImages DvzImages;
typedef struct DvzImageViews DvzImageViews;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCommands
{
    DvzObject obj;
    DvzDevice* device;
    DvzQueue* queue;

    uint32_t count;
    VkCommandBuffer cmds[DVZ_MAX_SWAPCHAIN_IMAGES];
    bool blocked[DVZ_MAX_SWAPCHAIN_IMAGES]; // if true, no need to refill it in the FRAME
};



struct DvzSampler
{
    DvzObject obj;
    DvzDevice* device;

    VkFilter min_filter;
    VkFilter mag_filter;
    VkSamplerAddressMode address_modes[3]; // x, y, z
    float anisotropy;

    VkSampler vk_sampler;
};



struct DvzCompute
{
    DvzObject obj;
    DvzDevice* device;

    VkShaderModule shader;
    VkPipelineLayout layout;

    VkSpecializationMapEntry spec_entries[DVZ_MAX_SPEC_CONST];
    VkSpecializationInfo spec_info;
    unsigned char spec_data[DVZ_MAX_SPEC_CONST_SIZE]; // specialization constant data buffer

    VkPipeline vk_pipeline;
};



struct DvzShader
{
    DvzObject obj; // used to hold the id in the mapping structure
    DvzDevice* device;
    DvzShaderType type;
    DvzSize size;
    VkShaderModule handle;
    uint32_t* buffer; // only for SPIRV obj_type
};



struct DvzSlots
{
    DvzObject obj;
    DvzDevice* device;

    // Binding types, for each set and each binding in each set.
    uint32_t set_count;
    uint32_t binding_counts[DVZ_MAX_SETS];
    VkDescriptorSetLayoutBinding bindings[DVZ_MAX_SETS][DVZ_MAX_BINDINGS];

    // Push constants.
    uint32_t push_count;
    VkPushConstantRange pushs[DVZ_MAX_PUSH_CONSTANTS];

    // Descriptor set layouts.
    VkDescriptorSetLayout set_layouts[DVZ_MAX_BINDINGS];

    // Pipeline layout.
    VkPipelineLayout pipeline_layout;
};



struct DvzDescriptors
{
    DvzSlots* slots;
    DvzDevice* device;
    VkDescriptorSet vk_descriptors[DVZ_MAX_SETS];
};



struct DvzBuffer
{
    DvzObject obj;
    DvzDevice* device;
    DvzVma* allocator;

    DvzSize req_size;
    VkBufferUsageFlags req_usage;

    VkBuffer vk_buffer;
    DvzAllocation alloc;
};



struct DvzImages
{
    DvzObject obj;
    DvzDevice* device;
    DvzVma* allocator;

    uint32_t count;
    uvec3 shape;
    uint32_t mip;    // number of mip levels
    uint32_t layers; // number of array layers
    bool is_swapchain;

    VkFormat format;
    VkImageType image_type;
    VkImageLayout layout;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSampleCountFlags samples; // for multisample antialiasing

    VkImage vk_images[DVZ_MAX_IMAGES];
    DvzAllocation allocs[DVZ_MAX_IMAGES];
};



struct DvzImageViews
{
    DvzObject obj;
    DvzDevice* device;
    DvzImages* images;
    VkImageViewType type;
    VkImageAspectFlags aspect;
    uint32_t mip_base;
    uint32_t mip_count;
    uint32_t layers_base;
    uint32_t layers_count;
    VkImageView vk_views[DVZ_MAX_IMAGES];
};
