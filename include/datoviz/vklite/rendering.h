/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Rendering                                                                                    */
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
typedef struct DvzCommands DvzCommands;

typedef struct DvzAttachment DvzAttachment;
typedef struct DvzRendering DvzRendering;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



EXTERN_C_ON

/*************************************************************************************************/
/*  Attachment                                                                                   */
/*************************************************************************************************/

/**
 * Set the attachment image view and layout.
 *
 * @param attachment the attachment
 * @param view the image view
 * @param layout the image layout
 */
DVZ_EXPORT void
dvz_attachment_image(DvzAttachment* attachment, VkImageView view, VkImageLayout layout);



/**
 * When using multisampling, set the resolve mod and image view.
 *
 * @param attachment the attachment
 * @param mode the resolve mode flags
 * @param view the resolve image view
 * @param layout the resolve image layout
 */
DVZ_EXPORT void dvz_attachment_resolve(
    DvzAttachment* attachment, VkResolveModeFlagBits mode, VkImageView view, VkImageLayout layout);



/**
 * Set the attachment load and store operations.
 *
 * @param attachment the attachment
 * @param load the load operation
 * @param store the store operation
 */
DVZ_EXPORT void
dvz_attachment_ops(DvzAttachment* attachment, VkAttachmentLoadOp load, VkAttachmentStoreOp store);



/**
 * Set the attachment clear value.
 *
 * @param attachment the attachment
 * @param clear the clear value
 */
DVZ_EXPORT void dvz_attachment_clear(DvzAttachment* attachment, VkClearValue clear);



/*************************************************************************************************/
/*  Rendering                                                                                    */
/*************************************************************************************************/

/**
 * Create a rendering.
 *
 * @param rendering the rendering
 */
DVZ_EXPORT void dvz_rendering(DvzRendering* rendering);



/**
 * Set a rendering area.
 *
 * @param rendering the rendering
 * @param x the offset x
 * @param y the offset y
 * @param width the width
 * @param height the height
 */
DVZ_EXPORT void
dvz_rendering_area(DvzRendering* rendering, int32_t x, int32_t y, uint32_t width, uint32_t height);



/**
 * Set the number of layers in a rendering.
 *
 * @param rendering the rendering
 * @param count the number of layers
 */
DVZ_EXPORT void dvz_rendering_layers(DvzRendering* rendering, uint32_t count);



/**
 * Return a color attachment of a rendering.
 *
 * @param rendering the rendering
 * @param idx the color attachment index
 * @returns the attachment
 */
DVZ_EXPORT DvzAttachment* dvz_rendering_color(DvzRendering* rendering, uint32_t idx);



/**
 * Return the depth attachment of a rendering.
 *
 * @param rendering the rendering
 * @returns the attachment
 */
DVZ_EXPORT DvzAttachment* dvz_rendering_depth(DvzRendering* rendering);



/**
 * Return the stencil attachment of a rendering.
 *
 * @param rendering the rendering
 * @returns the attachment
 */
DVZ_EXPORT DvzAttachment* dvz_rendering_stencil(DvzRendering* rendering);



/**
 * Begin a rendering.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param rendering the rendering
 */
DVZ_EXPORT void dvz_cmd_rendering_begin(DvzCommands* cmds, uint32_t idx, DvzRendering* rendering);



/**
 * End a rendering.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
DVZ_EXPORT void dvz_cmd_rendering_end(DvzCommands* cmds, uint32_t idx);



EXTERN_C_OFF
