/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "types.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_commands(DvzCommands* cmds, DvzDevice* device, DvzQueue* queue, uint32_t count)
{
    ANN(cmds);
    ANN(device);

    ASSERT(0 < count && count <= DVZ_MAX_SWAPCHAIN_IMAGES);
    uint32_t qf = dvz_queue_family(queue);
    uint32_t qi = dvz_queue_index(queue);
    log_trace("creating commands on queue #%d, queue family #%d", qi, qf);

    cmds->device = device;
    cmds->queue_idx = qi;
    cmds->count = count;

    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = dvz_device_command_pool(device, qf);
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = count;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(dvz_device_handle(device), &info, cmds->cmds));

    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_begin(DvzCommands* cmds, uint32_t idx)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    // log_trace("begin command buffer");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmds->cmds[idx], &begin_info));
}



void dvz_cmd_end(DvzCommands* cmds, uint32_t idx)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    // log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(cmds->cmds[idx]));

    dvz_obj_created(&cmds->obj);
}



void dvz_cmd_reset(DvzCommands* cmds, uint32_t idx)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    log_trace("reset command buffer #%d", idx);
    ASSERT(cmds->cmds[idx] != VK_NULL_HANDLE);
    VK_CHECK_RESULT(vkResetCommandBuffer(cmds->cmds[idx], 0));

    // NOTE: when resetting, we mark the object as not created because it is no longer filled with
    // commands.
    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_free(DvzCommands* cmds)
{
    // ANN(cmds);
    // ASSERT(cmds->count > 0);
    // ANN(cmds->gpu);
    // ASSERT(cmds->gpu->device != VK_NULL_HANDLE);

    // log_trace("free %d command buffer(s)", cmds->count);
    // vkFreeCommandBuffers(
    //     cmds->gpu->device, cmds->gpu->queues.cmd_pools[cmds->queue_idx], //
    //     cmds->count, cmds->cmds);

    // dvz_obj_init(&cmds->obj);
}



void dvz_cmd_submit_sync(DvzCommands* cmds, uint32_t idx)
{
    // ANN(cmds);
    // ASSERT(cmds->count > 0);
    // // NOTE: idx is NOT used for now

    // log_debug("[SLOW] submit %d command buffer(s) to queue #%d", cmds->count, cmds->queue_idx);

    // DvzQueues* q = &cmds->gpu->queues;
    // VkQueue queue = q->queues[cmds->queue_idx];

    // // NOTE: hard synchronization on the whole GPU here, otherwise write after write hasard
    // warning
    // // if just waiting on the queue.
    // vkDeviceWaitIdle(cmds->gpu->device);
    // VkSubmitInfo info = {0};
    // info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // info.commandBufferCount = cmds->count;
    // info.pCommandBuffers = cmds->cmds;
    // vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
    // vkQueueWaitIdle(queue);
}



void dvz_commands_destroy(DvzCommands* cmds)
{
    ANN(cmds);
    if (!dvz_obj_is_created(&cmds->obj))
    {
        log_trace("skip destruction of already-destroyed commands");
        return;
    }
    log_trace("destroy commands");
    dvz_obj_destroyed(&cmds->obj);
}
