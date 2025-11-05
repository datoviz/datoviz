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
#include <volk.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_commands(DvzDevice* device, DvzQueue* queue, uint32_t count, DvzCommands* cmds)
{
    ANN(cmds);
    ANN(device);
    ANN(queue);

    ASSERT(0 < count && count <= DVZ_MAX_SWAPCHAIN_IMAGES);
    log_trace("creating commands");

    cmds->device = device;
    cmds->queue = queue;
    cmds->count = count;

    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = dvz_device_command_pool(device, dvz_queue_family(queue));
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = count;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(dvz_device_handle(device), &info, cmds->cmds));

    dvz_obj_init(&cmds->obj);
}



VkCommandBuffer dvz_commands_handle(DvzCommands* cmds)
{
    ANN(cmds);
    return cmds->cmds[cmds->current];
}



void dvz_commands_current(DvzCommands* cmds, uint32_t current)
{
    ANN(cmds);
    if (current >= cmds->count)
    {
        log_error("the current index (%d) must be no greater than %d", current, cmds->count);
        return;
    }
    cmds->current = current;
}



void dvz_cmd_begin(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);


    // log_trace("begin command buffer");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(dvz_commands_handle(cmds), &begin_info));
}



void dvz_cmd_end(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);

    // log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(dvz_commands_handle(cmds)));

    dvz_obj_created(&cmds->obj);
}



void dvz_cmd_reset(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);

    log_trace("reset command buffer #%d", cmds->current);
    ANNVK(cmd);
    VK_CHECK_RESULT(vkResetCommandBuffer(cmd, 0));

    // NOTE: when resetting, we mark the object as not created because it is no longer filled with
    // commands.
    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_free(DvzCommands* cmds)
{
    ANN(cmds);

    DvzDevice* device = cmds->device;
    ANN(device);

    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    VkCommandPool cpool = dvz_device_command_pool(device, dvz_queue_family(cmds->queue));

    log_trace("free %d command buffer(s)", cmds->count);
    vkFreeCommandBuffers(vkd, cpool, cmds->count, cmds->cmds);

    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_submit(DvzCommands* cmds)
{
    ANN(cmds);

    DvzDevice* device = cmds->device;
    ANN(device);

    log_trace("submit %d command buffer(s)", cmds->count);

    DvzQueue* queue = cmds->queue;
    ANN(queue);

    // NOTE: inefficient device-level wait.
    dvz_device_wait(device);

    VkQueue vk_queue = dvz_queue_handle(queue);
    ANNVK(vk_queue);

    // Submit.
    VkSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = cmds->count;
    info.pCommandBuffers = cmds->cmds;
    vkQueueSubmit(vk_queue, 1, &info, VK_NULL_HANDLE);

    // Wait.
    dvz_queue_wait(queue);
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
