/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Rendering                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/vklite/rendering.h"
#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Attachment                                                                                   */
/*************************************************************************************************/

void dvz_attachment_image(DvzAttachment* attachment, VkImageView view, VkImageLayout layout)
{
    ANN(attachment);
    attachment->info.imageView = view;
    attachment->info.imageLayout = layout;
}



void dvz_attachment_resolve(
    DvzAttachment* attachment, VkResolveModeFlagBits mode, VkImageView view, VkImageLayout layout)
{
    ANN(attachment);
    attachment->info.resolveMode = mode;
    attachment->info.resolveImageView = view;
    attachment->info.resolveImageLayout = layout;
}



void dvz_attachment_ops(
    DvzAttachment* attachment, VkAttachmentLoadOp load, VkAttachmentStoreOp store)
{
    ANN(attachment);
    attachment->info.loadOp = load;
    attachment->info.storeOp = store;
}



void dvz_attachment_clear(DvzAttachment* attachment, VkClearValue clear)
{
    ANN(attachment);
    attachment->info.clearValue = clear;
}



/*************************************************************************************************/
/*  Rendering                                                                                    */
/*************************************************************************************************/

void dvz_rendering(DvzRendering* rendering)
{
    ANN(rendering);
    rendering->info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
}



void dvz_rendering_area(
    DvzRendering* rendering, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    ANN(rendering);
    rendering->info.renderArea.offset.x = x;
    rendering->info.renderArea.offset.y = y;
    rendering->info.renderArea.extent.width = width;
    rendering->info.renderArea.extent.height = height;
}



void dvz_rendering_layers(DvzRendering* rendering, uint32_t count)
{
    ANN(rendering);
    ASSERT(count > 0);
    rendering->info.layerCount = count;
}



DvzAttachment* dvz_rendering_color(DvzRendering* rendering, uint32_t idx)
{
    ANN(rendering);
    ASSERT(idx < DVZ_MAX_ATTACHMENTS);
    rendering->info.colorAttachmentCount = MAX(rendering->info.colorAttachmentCount, idx + 1);
    rendering->attachments[idx].info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->attachments[idx];
}



DvzAttachment* dvz_rendering_depth(DvzRendering* rendering)
{
    ANN(rendering);
    rendering->depth.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->depth;
}



DvzAttachment* dvz_rendering_stencil(DvzRendering* rendering)
{
    ANN(rendering);
    rendering->stencil.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->stencil;
}



void dvz_cmd_rendering_begin(DvzCommands* cmds, uint32_t idx, DvzRendering* rendering)
{
    ANN(cmds);
    ANN(rendering);

    VkRenderingAttachmentInfo attachments[DVZ_MAX_ATTACHMENTS] = {0};
    for (uint32_t i = 0; i < rendering->info.colorAttachmentCount; i++)
    {
        attachments[i] = rendering->attachments[i].info;
    }

    rendering->info.pColorAttachments = attachments;
    rendering->info.pDepthAttachment = &rendering->depth.info;
    rendering->info.pStencilAttachment = &rendering->stencil.info;

    vkCmdBeginRendering(cmds->cmds[idx], &rendering->info);
}



void dvz_cmd_rendering_end(DvzCommands* cmds, uint32_t idx)
{
    ANN(cmds);
    vkCmdEndRendering(cmds->cmds[idx]);
}
