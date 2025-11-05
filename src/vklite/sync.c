/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sync                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <volk.h>

#include "../src/vk/macros.h"
#include "_assertions.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sync.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Memory barrier                                                                               */
/*************************************************************************************************/

void dvz_barrier_memory_stage(
    DvzBarrierMemory* bmem, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst)
{
    ANN(bmem);
    bmem->srcStageMask = src;
    bmem->dstStageMask = dst;
}



void dvz_barrier_memory_access(DvzBarrierMemory* bmem, VkAccessFlags2 src, VkAccessFlags2 dst)
{
    ANN(bmem);
    bmem->srcAccessMask = src;
    bmem->dstAccessMask = dst;
}



/*************************************************************************************************/
/*  Buffer barrier                                                                               */
/*************************************************************************************************/

void dvz_barrier_buffer_stage( //
    DvzBarrierBuffer* bbuf, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst)
{
    ANN(bbuf);
    bbuf->srcStageMask = src;
    bbuf->dstStageMask = dst;
}



void dvz_barrier_buffer_access( //
    DvzBarrierBuffer* bbuf, VkAccessFlags2 src, VkAccessFlags2 dst)
{
    ANN(bbuf);
    bbuf->srcAccessMask = src;
    bbuf->dstAccessMask = dst;
}



void dvz_barrier_buffer_queue( //
    DvzBarrierBuffer* bbuf, uint32_t src, uint32_t dst)
{
    ANN(bbuf);
    bbuf->srcQueueFamilyIndex = src;
    bbuf->dstQueueFamilyIndex = dst;
}



/*************************************************************************************************/
/*  Image barrier                                                                                */
/*************************************************************************************************/

void dvz_barrier_image_stage( //
    DvzBarrierImage* bimg, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst)
{
    ANN(bimg);
    bimg->srcStageMask = src;
    bimg->dstStageMask = dst;
}



void dvz_barrier_image_access( //
    DvzBarrierImage* bimg, VkAccessFlags2 src, VkAccessFlags2 dst)
{
    ANN(bimg);
    bimg->srcAccessMask = src;
    bimg->dstAccessMask = dst;
}



void dvz_barrier_image_layout( //
    DvzBarrierImage* bimg, VkImageLayout old, VkImageLayout new)
{
    ANN(bimg);
    bimg->oldLayout = old;
    bimg->newLayout = new;
}



void dvz_barrier_image_queue( //
    DvzBarrierImage* bimg, uint32_t src, uint32_t dst)
{
    ANN(bimg);
    bimg->srcQueueFamilyIndex = src;
    bimg->dstQueueFamilyIndex = dst;
}



void dvz_barrier_image_aspect( //
    DvzBarrierImage* bimg, VkImageAspectFlags aspect)
{
    ANN(bimg);
    bimg->subresourceRange.aspectMask = aspect;
}



void dvz_barrier_image_mip( //
    DvzBarrierImage* bimg, uint32_t base, uint32_t count)
{
    ANN(bimg);
    bimg->subresourceRange.baseMipLevel = base;
    bimg->subresourceRange.levelCount = count;
}



void dvz_barrier_image_layers(DvzBarrierImage* bimg, uint32_t base, uint32_t count)
{
    ANN(bimg);
    bimg->subresourceRange.baseArrayLayer = base;
    bimg->subresourceRange.layerCount = count;
}



/*************************************************************************************************/
/*  Barriers                                                                                     */
/*************************************************************************************************/

void dvz_barriers(DvzBarriers* barriers)
{
    ANN(barriers);
    barriers->info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barriers->info.pMemoryBarriers = barriers->bmems;
    barriers->info.pBufferMemoryBarriers = barriers->bbufs;
    barriers->info.pImageMemoryBarriers = barriers->bimg;
}



void dvz_barriers_flags(DvzBarriers* barriers, VkDependencyFlags flags)
{
    ANN(barriers);
    barriers->info.dependencyFlags = flags;
}


DvzBarrierMemory* dvz_barriers_memory(DvzBarriers* barriers)
{
    ANN(barriers);
    DvzBarrierMemory* bmem = &barriers->bmems[barriers->info.memoryBarrierCount++];
    ANN(bmem);

    bmem->sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;

    return bmem;
}


DvzBarrierBuffer*
dvz_barriers_buffer(DvzBarriers* barriers, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
{
    ANN(barriers);

    DvzBarrierBuffer* bbuf = &barriers->bbufs[barriers->info.bufferMemoryBarrierCount++];
    ANN(bbuf);

    bbuf->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    bbuf->buffer = buffer;
    bbuf->offset = offset;
    bbuf->size = size;

    return bbuf;
}


DvzBarrierImage* dvz_barriers_image(DvzBarriers* barriers, VkImage img)
{
    ANN(barriers);
    ANNVK(img);

    DvzBarrierImage* bimg = &barriers->bimg[barriers->info.imageMemoryBarrierCount++];

    bimg->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    bimg->image = img;

    // Default values.
    bimg->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bimg->subresourceRange.levelCount = 1;
    bimg->subresourceRange.layerCount = 1;

    return bimg;
}



void dvz_cmd_barriers(DvzCommands* cmds, uint32_t idx, DvzBarriers* barriers)
{
    ANN(cmds);
    ANN(barriers);
    vkCmdPipelineBarrier2(cmds->cmds[idx], &barriers->info);
}



/*************************************************************************************************/
/*  Fence                                                                                        */
/*************************************************************************************************/

void dvz_fence(DvzDevice* device, bool signaled, DvzFence* fence)
{
    ANN(device);
    ANN(fence);

    fence->device = device;

    VkFenceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK_RESULT(vkCreateFence(fence->device->vk_device, &info, NULL, &fence->vk_fence));

    dvz_obj_created(&fence->obj);
}



void dvz_fence_wait(DvzFence* fence)
{
    ANN(fence);
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        vkWaitForFences(fence->device->vk_device, 1, &fence->vk_fence, VK_TRUE, 100000000);
    }
    else
    {
        log_trace("skip wait for fence %u", fence->vk_fence);
    }
}



bool dvz_fence_ready(DvzFence* fence)
{
    ANN(fence);
    VK_RETURN_RESULT(vkGetFenceStatus(fence->device->vk_device, fence->vk_fence));
    return (bool)out;
}



void dvz_fence_reset(DvzFence* fence)
{
    ANN(fence);
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        vkResetFences(fence->device->vk_device, 1, &fence->vk_fence);
    }
}



void dvz_fence_destroy(DvzFence* fence)
{
    ANN(fence);
    if (!dvz_obj_is_created(&fence->obj))
    {
        log_trace("skip destruction of already-destroyed fence");
        return;
    }

    log_trace("destroying fence...");
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(fence->device->vk_device, fence->vk_fence, NULL);
        fence->vk_fence = VK_NULL_HANDLE;
    }
    dvz_obj_destroyed(&fence->obj);
}
