/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vulkan enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Shader format.
typedef enum
{
    DVZ_SHADER_NONE,
    DVZ_SHADER_SPIRV,
    DVZ_SHADER_GLSL,
} DvzShaderFormat;



// Default queue.
typedef enum
{
    // NOTE: by convention in vklite, the first queue MUST support transfers
    DVZ_DEFAULT_QUEUE_TRANSFER,
    DVZ_DEFAULT_QUEUE_COMPUTE,
    DVZ_DEFAULT_QUEUE_RENDER,
    DVZ_DEFAULT_QUEUE_PRESENT,
    DVZ_DEFAULT_QUEUE_COUNT,
} DvzDefaultQueue;



/*************************************************************************************************/
/*  Vulkan wrapper enums, avoiding dependency to vulkan.h                                        */
/*  WARNING: they must match exactly the corresponding Vulkan enums.                             */
/*************************************************************************************************/

// VkPrimitiveTopology wrapper.
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,

} DvzPrimitiveTopology;



// VkFormat wrapper.
// NOTE: we only included the most common ones, this list can be completed as needed.
// IMPORTANT: the original Vulkan enum values need to be used:
// see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
//
// NOTE: see https://vulkan.gpuinfo.org/listbufferformats.php for hardware support
// Avoid using poorly-supported formats.
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8_UINT = 13,
    DVZ_FORMAT_R8_SINT = 14,
    DVZ_FORMAT_R8G8_UNORM = 16,
    DVZ_FORMAT_R8G8_SNORM = 17,
    DVZ_FORMAT_R8G8_UINT = 20,
    DVZ_FORMAT_R8G8_SINT = 21,
    DVZ_FORMAT_R8G8B8_UNORM = 23, // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_SNORM = 24, // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_UINT = 27,  // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_SINT = 28,  // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R8G8B8A8_SNORM = 38,
    DVZ_FORMAT_R8G8B8A8_UINT = 41,
    DVZ_FORMAT_R8G8B8A8_SINT = 42,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
    DVZ_FORMAT_R32G32_UINT = 101,
    DVZ_FORMAT_R32G32_SINT = 102,
    DVZ_FORMAT_R32G32_SFLOAT = 103,
    DVZ_FORMAT_R32G32B32_UINT = 104,   // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SINT = 105,   // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SFLOAT = 106, // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32A32_UINT = 107,
    DVZ_FORMAT_R32G32B32A32_SINT = 108,
    DVZ_FORMAT_R32G32B32A32_SFLOAT = 109,

    // NOTE: poor GPU hardware support
    DVZ_FORMAT_R64_UINT = 110,
    DVZ_FORMAT_R64_SINT = 111,
    DVZ_FORMAT_R64_SFLOAT = 112,
    DVZ_FORMAT_R64G64_UINT = 113,
    DVZ_FORMAT_R64G64_SINT = 114,
    DVZ_FORMAT_R64G64_SFLOAT = 115,
    DVZ_FORMAT_R64G64B64_UINT = 116,
    DVZ_FORMAT_R64G64B64_SINT = 117,
    DVZ_FORMAT_R64G64B64_SFLOAT = 118,
    DVZ_FORMAT_R64G64B64A64_UINT = 119,
    DVZ_FORMAT_R64G64B64A64_SINT = 120,
    DVZ_FORMAT_R64G64B64A64_SFLOAT = 121,
} DvzFormat;



// Color mask.
// VkColorComponentFlagBits wrapper
typedef enum
{
    DVZ_MASK_COLOR_R = 0x00000001,
    DVZ_MASK_COLOR_G = 0x00000002,
    DVZ_MASK_COLOR_B = 0x00000004,
    DVZ_MASK_COLOR_A = 0x00000008,
    DVZ_MASK_COLOR_ALL = 0x0000000F,
} DvzColorMask;



// VkFilter wrapper.
typedef enum
{
    DVZ_FILTER_NEAREST = 0,
    DVZ_FILTER_LINEAR = 1,
    DVZ_FILTER_CUBIC_IMG = 1000015000,
} DvzFilter;



// VkSamplerAddressMode wrapper.
typedef enum
{
    DVZ_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
} DvzSamplerAddressMode;



// VkVertexInputRate wrapper.
typedef enum
{
    DVZ_VERTEX_INPUT_RATE_VERTEX = 0,
    DVZ_VERTEX_INPUT_RATE_INSTANCE = 1,
} DvzVertexInputRate;



// VkPolygonMode wrapper.
typedef enum
{
    DVZ_POLYGON_MODE_FILL = 0,
    DVZ_POLYGON_MODE_LINE = 1,
    DVZ_POLYGON_MODE_POINT = 2,
} DvzPolygonMode;



// VkFrontFace wrapper.
typedef enum
{
    DVZ_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    DVZ_FRONT_FACE_CLOCKWISE = 1,
} DvzFrontFace;



// VkCullModeFlagBits wrapper.
typedef enum
{
    DVZ_CULL_MODE_NONE = 0,
    DVZ_CULL_MODE_FRONT = 0x00000001,
    DVZ_CULL_MODE_BACK = 0x00000002,
} DvzCullMode;



// VkShaderStageFlagBits wrapper.
typedef enum
{
    DVZ_SHADER_VERTEX = 0x00000001,
    DVZ_SHADER_TESSELLATION_CONTROL = 0x00000002,
    DVZ_SHADER_TESSELLATION_EVALUATION = 0x00000004,
    DVZ_SHADER_GEOMETRY = 0x00000008,
    DVZ_SHADER_FRAGMENT = 0x00000010,
    DVZ_SHADER_COMPUTE = 0x00000020,
} DvzShaderType;



typedef int32_t DvzShaderStageFlags;



// VkDescriptorType wrapper.
typedef enum
{
    DVZ_DESCRIPTOR_TYPE_SAMPLER = 0,
    DVZ_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    DVZ_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    DVZ_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    DVZ_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
} DvzDescriptorType;
