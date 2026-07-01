/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Backend-neutral render protocol types                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Texture, attachment, and vertex attribute format tokens used by scene and DRP2 protocol APIs.
// Numeric values preserve the previous Datoviz v0.4-dev assignments, but they are Datoviz protocol
// tokens and not a public Vulkan contract.
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
    DVZ_FORMAT_R8G8B8A8_SRGB = 43,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_B8G8R8A8_SRGB = 50,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R16_UINT = 74,
    DVZ_FORMAT_R16_SINT = 75,
    DVZ_FORMAT_R16_SFLOAT = 76,
    DVZ_FORMAT_R16G16B16A16_UNORM = 91,
    DVZ_FORMAT_R16G16B16A16_SNORM = 92,
    DVZ_FORMAT_R16G16B16A16_UINT = 95,
    DVZ_FORMAT_R16G16B16A16_SINT = 96,
    DVZ_FORMAT_R16G16B16A16_SFLOAT = 97,
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
    DVZ_FORMAT_D16_UNORM = 124,
    DVZ_FORMAT_X8_D24_UNORM_PACK32 = 125,
    DVZ_FORMAT_D32_SFLOAT = 126,
    DVZ_FORMAT_D16_UNORM_S8_UINT = 128,
    DVZ_FORMAT_D24_UNORM_S8_UINT = 129,
    DVZ_FORMAT_D32_SFLOAT_S8_UINT = 130,
} DvzFormat;



// Primitive topology.
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
} DvzPrimitiveTopology;



// Depth/stencil comparison operation.
typedef enum
{
    DVZ_COMPARE_OP_NEVER = 0,
    DVZ_COMPARE_OP_LESS = 1,
    DVZ_COMPARE_OP_EQUAL = 2,
    DVZ_COMPARE_OP_LESS_OR_EQUAL = 3,
    DVZ_COMPARE_OP_GREATER = 4,
    DVZ_COMPARE_OP_NOT_EQUAL = 5,
    DVZ_COMPARE_OP_GREATER_OR_EQUAL = 6,
    DVZ_COMPARE_OP_ALWAYS = 7,
} DvzCompareOp;



// Front-face winding.
typedef enum
{
    DVZ_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    DVZ_FRONT_FACE_CLOCKWISE = 1,
} DvzFrontFace;



// Face culling mode.
typedef enum
{
    DVZ_CULL_MODE_NONE = 0,
    DVZ_CULL_MODE_FRONT = 0x00000001,
    DVZ_CULL_MODE_BACK = 0x00000002,
    DVZ_CULL_MODE_FRONT_AND_BACK = 0x00000003,
} DvzCullMode;



// Blend factor.
typedef enum
{
    DVZ_BLEND_FACTOR_ZERO = 0,
    DVZ_BLEND_FACTOR_ONE = 1,
    DVZ_BLEND_FACTOR_SRC_COLOR = 2,
    DVZ_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
    DVZ_BLEND_FACTOR_DST_COLOR = 4,
    DVZ_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
    DVZ_BLEND_FACTOR_SRC_ALPHA = 6,
    DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
    DVZ_BLEND_FACTOR_DST_ALPHA = 8,
    DVZ_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    DVZ_BLEND_FACTOR_CONSTANT_COLOR = 10,
    DVZ_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
    DVZ_BLEND_FACTOR_CONSTANT_ALPHA = 12,
    DVZ_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
    DVZ_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
} DvzBlendFactor;



// Blend operation.
typedef enum
{
    DVZ_BLEND_OP_ADD = 0,
    DVZ_BLEND_OP_SUBTRACT = 1,
    DVZ_BLEND_OP_REVERSE_SUBTRACT = 2,
    DVZ_BLEND_OP_MIN = 3,
    DVZ_BLEND_OP_MAX = 4,
} DvzBlendOp;



// Color component write mask.
typedef enum
{
    DVZ_MASK_COLOR_R = 0x00000001,
    DVZ_MASK_COLOR_G = 0x00000002,
    DVZ_MASK_COLOR_B = 0x00000004,
    DVZ_MASK_COLOR_A = 0x00000008,
    DVZ_MASK_COLOR_ALL = 0x0000000F,
} DvzColorMask;
