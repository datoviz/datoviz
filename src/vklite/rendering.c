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

#include <stddef.h>
#include <volk.h>

#include "../vk/macros.h"
#include "_assertions.h"
#include "datoviz/math/types.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/rendering.h"



/*************************************************************************************************/
/*  Attachment                                                                                   */
/*************************************************************************************************/

void dvz_attachment_image(DvzAttachment* attachment, VkImageView view, VkImageLayout layout)
{
    ANN(attachment);
    attachment->imageView = view;
    attachment->imageLayout = layout;
}



void dvz_attachment_resolve(
    DvzAttachment* attachment, VkResolveModeFlagBits mode, VkImageView view, VkImageLayout layout)
{
    ANN(attachment);
    attachment->resolveMode = mode;
    attachment->resolveImageView = view;
    attachment->resolveImageLayout = layout;
}



void dvz_attachment_ops(
    DvzAttachment* attachment, VkAttachmentLoadOp load, VkAttachmentStoreOp store)
{
    ANN(attachment);
    attachment->loadOp = load;
    attachment->storeOp = store;
}



void dvz_attachment_clear(DvzAttachment* attachment, VkClearValue clear)
{
    ANN(attachment);
    attachment->clearValue = clear;
}



/*************************************************************************************************/
/*  Rendering                                                                                    */
/*************************************************************************************************/

void dvz_rendering(DvzRendering* rendering)
{
    ANN(rendering);

    // Default values.
    rendering->info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering->info.layerCount = 1;
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
    rendering->attachments[idx].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->attachments[idx];
}



DvzAttachment* dvz_rendering_depth(DvzRendering* rendering)
{
    ANN(rendering);
    rendering->depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->depth;
}



DvzAttachment* dvz_rendering_stencil(DvzRendering* rendering)
{
    ANN(rendering);
    rendering->stencil.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    return &rendering->stencil;
}



void dvz_cmd_rendering_begin(DvzCommands* cmds, DvzRendering* rendering)
{
    ANN(cmds);
    ANN(rendering);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    VkRenderingAttachmentInfo attachments[DVZ_MAX_ATTACHMENTS] = {0};
    for (uint32_t i = 0; i < rendering->info.colorAttachmentCount; i++)
    {
        attachments[i] = rendering->attachments[i];
    }

    rendering->info.pColorAttachments = attachments;
    rendering->info.pDepthAttachment = rendering->depth.sType != 0 ? &rendering->depth : NULL;
    rendering->info.pStencilAttachment =
        rendering->stencil.sType != 0 ? &rendering->stencil : NULL;

    vkCmdBeginRendering(cmd, &rendering->info);
}



void dvz_cmd_rendering_default(
    DvzCommands* cmds, VkImageView image_view, uint32_t width, uint32_t height,
    VkClearValue clear_value, DvzRendering* rendering)
{
    ANN(cmds);
    ANN(rendering);

    dvz_rendering(rendering);
    dvz_rendering_area(rendering, 0, 0, width, height);
    DvzAttachment* catt = dvz_rendering_color(rendering, 0);
    dvz_attachment_image(catt, image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(catt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(catt, clear_value);
}



void dvz_cmd_rendering_end(DvzCommands* cmds)
{
    ANN(cmds);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    vkCmdEndRendering(cmd);
}



/*************************************************************************************************/
/*  Drawing                                                                                      */
/*************************************************************************************************/

void dvz_cmd_draw(
    DvzCommands* cmds, uint32_t first_vertex, uint32_t vertex_count, uint32_t first_instance,
    uint32_t instance_count)
{
    ANN(cmds);
    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    ASSERT(vertex_count > 0);
    vkCmdDraw(cmd, vertex_count, instance_count, first_vertex, first_instance);
}



void dvz_cmd_draw_indexed(
    DvzCommands* cmds, uint32_t first_index, int32_t vertex_offset, uint32_t index_count,
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(cmds);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);
    ASSERT(index_count > 0);
    vkCmdDrawIndexed(cmd, index_count, instance_count, first_index, vertex_offset, first_instance);
}



void dvz_cmd_draw_indirect(
    DvzCommands* cmds, VkBuffer indirect, DvzSize offset, uint32_t draw_count, DvzSize stride)
{
    ANN(cmds);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);
    vkCmdDrawIndirect(cmd, indirect, offset, draw_count, stride);
}



void dvz_cmd_draw_indexed_indirect(
    DvzCommands* cmds, VkBuffer indirect, DvzSize offset, uint32_t draw_count, DvzSize stride)
{
    ANN(cmds);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    vkCmdDrawIndexedIndirect(cmd, indirect, offset, draw_count, stride);
}
