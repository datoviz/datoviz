/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzSlots DvzSlots;
typedef struct DvzGraphics DvzGraphics;
typedef struct DvzCommands DvzCommands;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics flags for settings that support dynamic state.
typedef enum
{
    DVZ_GRAPHICS_FLAGS_DISABLE = 0,
    DVZ_GRAPHICS_FLAGS_FIXED = 1,
    DVZ_GRAPHICS_FLAGS_DYNAMIC = 2,
} DvzGraphicFlags;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a graphics pipeline.
 *
 * @param device the device
 * @param[out graphics] the created graphics pipeline
 */
DVZ_EXPORT void dvz_graphics(DvzDevice* device, DvzGraphics* graphics);



/**
 * Set the path to a shader for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param shader the shader module
 */
DVZ_EXPORT void
dvz_graphics_shader(DvzGraphics* graphics, VkShaderStageFlagBits stage, VkShaderModule shader);



/**
 * Set a specialization constant.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param index the specialization constant index in the shader
 * @param offset the offset, in bytes, of that constant, without the specialization constant data
 * @param size the size of the specialization constant value
 * @param data the value of the constant
 */
DVZ_EXPORT void dvz_graphics_spec(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, uint32_t index, DvzSize offset,
    DvzSize size, void* data);



/**
 * Declare the format of a color attachment.
 *
 * @param graphics the graphics pipeline
 * @param idx the color attachment index
 * @param format the format
 */
DVZ_EXPORT void
dvz_graphics_attachment_color(DvzGraphics* graphics, uint32_t idx, VkFormat format);



/**
 * Declare the format of a depth attachment.
 *
 * @param graphics the graphics pipeline
 * @param format the format
 */
DVZ_EXPORT void dvz_graphics_attachment_depth(DvzGraphics* graphics, VkFormat format);



/**
 * Declare the format of a stencil attachment.
 *
 * @param graphics the graphics pipeline
 * @param format the format
 */
DVZ_EXPORT void dvz_graphics_attachment_stencil(DvzGraphics* graphics, VkFormat format);



/**
 * Set the vertex binding.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index
 * @param stride the stride in the vertex buffer, in bytes
 * @param input_rate the vertex input rate, VK_VERTEX_INPUT_RATE_VERTEX|INSTANCE
 */
DVZ_EXPORT void dvz_graphics_vertex_binding(
    DvzGraphics* graphics, uint32_t binding, DvzSize stride, VkVertexInputRate input_rate);



/**
 * Add a vertex attribute.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index (as specified in the vertex shader)
 * @param location the location index (as specified in the vertex shader)
 * @param format the format
 * @param offset the offset, in bytes
 */
DVZ_EXPORT void dvz_graphics_vertex_attr(
    DvzGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format, DvzSize offset);



/**
 * Set the pipeline layout.
 *
 * @param graphics the graphics pipeline
 * @param layout the pipeline layout
 */
DVZ_EXPORT void dvz_graphics_layout(DvzGraphics* graphics, VkPipelineLayout layout);



/**
 * Set the graphics pipeline primitive topology
 *
 * @param graphics the graphics pipeline
 * @param topology the primitive topology
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void
dvz_graphics_primitive(DvzGraphics* graphics, VkPrimitiveTopology topology, int flags);



/**
 * Set whether a special index value (when using indexed rendering) allows to restart the
 * primitive.
 *
 * @param graphics the graphics pipeline
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_primitive_restart(DvzGraphics* graphics, int flags);



/**
 * Set the graphics polygon mode.
 *
 * @param graphics the graphics pipeline
 * @param polygon_mode the polygon mode
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void
dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode, int flags);



/**
 * Set the graphics cull mode.
 *
 * @param graphics the graphics pipeline
 * @param cull_mode the cull mode
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void
dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode, int flags);



/**
 * Set the graphics front face.
 *
 * @param graphics the graphics pipeline
 * @param front_face the front face
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face, int flags);



/**
 * Set the depth test.
 *
 * @param graphics the graphics pipeline
 * @param clamp enable or disable depth clamp
 * @param depth_write enable or disable the depth write
 * @param compare depth compare operation
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_depth(
    DvzGraphics* graphics, bool clamp, bool depth_write, VkCompareOp compare, int flags);



/**
 * Set the depth bounds.
 *
 * @param graphics the graphics pipieline
 * @param min the minimum depth bound
 * @param max the maximum depth bound
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_depth_bounds(DvzGraphics* graphics, float min, float max, int flags);



/**
 * Enable or disable the depth bias.
 *
 * @param graphics the graphics pipeline
 * @param constant_factor the depth bias constant factor
 * @param clamp the depth bias clamp
 * @param slope_factor the depth bias slope factor
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_depth_bias(
    DvzGraphics* graphics, //
    float constant_factor, float clamp, float slope_factor, int flags);



/**
 * Set the stencil test.
 *
 * @param graphics the graphics pipeline
 * @param mask front or back
 * @param fail fail operation
 * @param pass pass operation
 * @param depth_fail depth fail operation
 * @param compare compare operation
 * @param compare_mask bits of the stencil values participating in the stencil test
 * @param write_mask bits of the stencil values updated by the stencil test in the attachment
 * @param reference stencil reference value
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_stencil(
    DvzGraphics* graphics, VkStencilFaceFlags mask, VkStencilOp fail, VkStencilOp pass,
    VkStencilOp depth_fail, VkCompareOp compare, uint32_t compare_mask, uint32_t write_mask,
    uint32_t reference, int flags);



/**
 * Set the scissor.
 *
 * @param graphics the graphics pipeline
 * @param x the offset x
 * @param y the offset y
 * @param width the width
 * @param height the height
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_scissor(
    DvzGraphics* graphics, int32_t x, int32_t y, uint32_t width, uint32_t height, int flags);



/**
 * Set the viewport.
 *
 * @param graphics the graphics pipeline
 * @param x the offset x
 * @param y the offset y
 * @param width the width
 * @param height the height
 * @param min_depth the minimum depth
 * @param max_depth the maximum depth
 * @param flags indicate whether this setting is fixed or dynamic state set in the command buffer
 */
DVZ_EXPORT void dvz_graphics_viewport(
    DvzGraphics* graphics, float x, float y, float width, float height, float min_depth,
    float max_depth, int flags);


/**
 * Set the blending parameters.
 *
 * @param graphics the graphics pipeline
 * @param enable whether to enable blending
 * @param op the logic operation
 * @param constants the blending constants
 * @param flags indicate whether the blending constants are fixed or dynamic state set in the
 * command buffer
 */
DVZ_EXPORT
void dvz_graphics_blend(DvzGraphics* graphics, VkLogicOp op, vec4 constants, int flags);



/**
 * Set the color blending parameters of a color attachment.
 *
 * @param graphics the graphics pipeline
 * @param idx the attachment index
 * @param src the source color blend factor
 * @param dst the destination color blend factor
 * @param op the color blend operation
 * @param mask the color write mask
 */
DVZ_EXPORT void dvz_graphics_blend_color(
    DvzGraphics* graphics, uint32_t idx, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op,
    VkColorComponentFlags mask);



/**
 * Set the alpha blending parameters of a color attachment.
 *
 * @param graphics the graphics pipeline
 * @param idx the attachment index
 * @param src the source alpha blend factor
 * @param dst the destination alpha blend factor
 * @param op the alpha blend operation
 */
DVZ_EXPORT void dvz_graphics_blend_alpha(
    DvzGraphics* graphics, uint32_t idx, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op);



/**
 * Set multisampling.
 *
 * @param graphics the graphics pipeline
 * @param samples the number of samples
 * @param min_sample_shading if >0, enable sample shading and set the minimum fraction of sample
 * shading
 * @param alpha_coverage alpha channel is used for relative sample coverage
 */
DVZ_EXPORT void dvz_graphics_multisampling(
    DvzGraphics* graphics, VkSampleCountFlagBits samples, float min_sample_shading,
    bool alpha_coverage);



/**
 * Create a graphics pipeline after it has been set up.
 *
 * @param graphics the graphics pipeline
 * @returns the creation result code
 */
DVZ_EXPORT int dvz_graphics_create(DvzGraphics* graphics);



/**
 * Destroy a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 */
DVZ_EXPORT void dvz_graphics_destroy(DvzGraphics* graphics);



/**
 * Bind a graphics pipeline.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param graphics the graphics pipeline
 */
DVZ_EXPORT void dvz_cmd_bind_graphics(DvzCommands* cmds, uint32_t idx, DvzGraphics* graphics);



EXTERN_C_OFF
