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
#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_commands.h"
#include "_log.h"
#include "_vk_utils.h"
#include "obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty commands wrapper.
 *
 * @return allocated commands wrapper, or NULL on allocation failure
 */
DvzCommands* dvz_commands_create_wrapper(void)
{
    DvzCommands* cmds = (DvzCommands*)dvz_calloc(1, sizeof(DvzCommands));
    ANN(cmds);
    return cmds;
}



/**
 * Free a commands wrapper allocated by dvz_commands_create_wrapper().
 *
 * @param cmds commands wrapper to free
 */
void dvz_commands_free(DvzCommands* cmds)
{
    if (cmds == NULL)
    {
        return;
    }
    dvz_free(cmds);
}

/**
 * Release owned command buffers from a commands wrapper.
 *
 * @param cmds the commands wrapper
 */
static void _commands_release(DvzCommands* cmds)
{
    ANN(cmds);
    if (cmds->device == NULL || cmds->count == 0)
    {
        cmds->count = 0;
        cmds->current = 0;
        return;
    }
    if (cmds->queue == NULL)
    {
        cmds->count = 0;
        cmds->current = 0;
        return;
    }

    VkDevice vkd = dvz_device_handle(cmds->device);
    ANNVK(vkd);
    VkCommandPool cpool = dvz_device_command_pool(cmds->device, dvz_queue_family(cmds->queue));
    if (cpool == VK_NULL_HANDLE)
    {
        log_warn(
            "skip command buffer free: missing command pool for queue family %u",
            dvz_queue_family(cmds->queue));
        cmds->count = 0;
        cmds->current = 0;
        return;
    }

    log_trace("free %d command buffer(s)", cmds->count);
    vkFreeCommandBuffers(vkd, cpool, cmds->count, cmds->cmds);
    dvz_memset(cmds->cmds, sizeof(cmds->cmds), 0, sizeof(cmds->cmds));
    cmds->count = 0;
    cmds->current = 0;
}

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
    VkResult res = vkAllocateCommandBuffers(dvz_device_handle(device), &info, cmds->cmds);
    if (res != VK_SUCCESS)
    {
        vk_result_check(res, __FILE__, __LINE__);
        cmds->count = 0;
        return;
    }

    dvz_obj_init(&cmds->obj);
}



/**
 * Allocate a single primary command buffer from the device command pool of a queue family.
 *
 * @param device the device
 * @param queue_family queue family index used to select the command pool
 * @return the allocated command buffer, or VK_NULL_HANDLE on failure
 */
VkCommandBuffer dvz_command_buffer_alloc(DvzDevice* device, uint32_t queue_family)
{
    ANN(device);

    VkCommandPool cpool = dvz_device_command_pool(device, queue_family);
    if (cpool == VK_NULL_HANDLE)
    {
        log_error("missing command pool for queue family %u", queue_family);
        return VK_NULL_HANDLE;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkResult res = vkAllocateCommandBuffers(dvz_device_handle(device), &info, &cmd);
    if (res != VK_SUCCESS)
    {
        vk_result_check(res, __FILE__, __LINE__);
        return VK_NULL_HANDLE;
    }
    return cmd;
}



/**
 * Free a single command buffer from the device command pool of a queue family.
 *
 * @param device the device
 * @param queue_family queue family index used to select the command pool
 * @param cmd command buffer to free
 */
void dvz_command_buffer_free(DvzDevice* device, uint32_t queue_family, VkCommandBuffer cmd)
{
    ANN(device);
    if (cmd == VK_NULL_HANDLE)
    {
        return;
    }

    VkCommandPool cpool = dvz_device_command_pool(device, queue_family);
    if (cpool == VK_NULL_HANDLE)
    {
        log_warn("skip command buffer free: missing command pool for queue family %u", queue_family);
        return;
    }
    vkFreeCommandBuffers(dvz_device_handle(device), cpool, 1, &cmd);
}



VkCommandBuffer dvz_commands_handle(DvzCommands* cmds)
{
    ANN(cmds);
    if (cmds->count == 0 || cmds->current >= cmds->count)
    {
        return VK_NULL_HANDLE;
    }
    return cmds->cmds[cmds->current];
}



uint32_t dvz_commands_count(DvzCommands* cmds)
{
    ANN(cmds);
    return cmds->count;
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



/**
 * Start recording a command buffer and report Vulkan failures.
 *
 * @param cmds the commands wrapper
 * @return 0 on success, non-zero on Vulkan or state failure
 */
int dvz_cmd_begin_result(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    if (cmds->borrowed_recording)
    {
        log_error("cannot begin a borrowed recording command buffer");
        return 1;
    }


    // log_trace("begin command buffer");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);
    VkResult res = vkBeginCommandBuffer(cmd, &begin_info);
    return vk_result_check(res, __FILE__, __LINE__);
}



/**
 * Start recording a command buffer.
 *
 * @param cmds the commands wrapper
 */
void dvz_cmd_begin(DvzCommands* cmds)
{
    (void)dvz_cmd_begin_result(cmds);
}



/**
 * Stop recording a command buffer and report Vulkan failures.
 *
 * @param cmds the commands wrapper
 * @return 0 on success, non-zero on Vulkan or state failure
 */
int dvz_cmd_end_result(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    if (cmds->borrowed_recording)
    {
        log_error("cannot end a borrowed recording command buffer");
        return 1;
    }

    // log_trace("end command buffer");
    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);
    VkResult res = vkEndCommandBuffer(cmd);
    int out = vk_result_check(res, __FILE__, __LINE__);
    if (out != 0)
        return out;

    dvz_obj_created(&cmds->obj);
    return 0;
}



/**
 * Stop recording a command buffer.
 *
 * @param cmds the commands wrapper
 */
void dvz_cmd_end(DvzCommands* cmds)
{
    (void)dvz_cmd_end_result(cmds);
}



void dvz_cmd_reset(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    if (cmds->borrowed_recording)
    {
        log_error("cannot reset a borrowed recording command buffer");
        return;
    }

    VkCommandBuffer cmd = dvz_commands_handle(cmds);

    log_trace("reset command buffer #%d", cmds->current);
    ANNVK(cmd);
    VK_CHECK_RESULT(vkResetCommandBuffer(cmd, 0));

    // NOTE: when resetting, we mark the object as not created because it is no longer filled with
    // commands.
    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_release(DvzCommands* cmds)
{
    ANN(cmds);
    if (cmds->borrowed_recording)
    {
        log_error("cannot free a borrowed recording command buffer");
        cmds->count = 0;
        cmds->current = 0;
        cmds->borrowed_recording = false;
        dvz_memset(cmds->cmds, sizeof(cmds->cmds), 0, sizeof(cmds->cmds));
        dvz_obj_destroyed(&cmds->obj);
        return;
    }
    _commands_release(cmds);
    dvz_obj_init(&cmds->obj);
}



/**
 * Submit a command buffer on its queue and report Vulkan failures.
 *
 * @param cmds the commands wrapper
 * @return 0 on success, non-zero on Vulkan or state failure
 */
int dvz_cmd_submit_result(DvzCommands* cmds)
{
    ANN(cmds);
    ASSERT(cmds->count > 0);
    if (cmds->borrowed_recording)
    {
        log_error("cannot submit a borrowed recording command buffer");
        return 1;
    }
    if (!dvz_obj_is_created(&cmds->obj))
    {
        log_error("cannot submit commands before recording them");
        return 1;
    }

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
    VkCommandBufferSubmitInfo submit_cmds[DVZ_MAX_SWAPCHAIN_IMAGES] = {0};
    for (uint32_t i = 0; i < cmds->count; ++i)
    {
        ANNVK(cmds->cmds[i]);
        submit_cmds[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit_cmds[i].commandBuffer = cmds->cmds[i];
    }

    VkSubmitInfo2 info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = cmds->count,
        .pCommandBufferInfos = submit_cmds,
    };
    VkResult res = vkQueueSubmit2(vk_queue, 1, &info, VK_NULL_HANDLE);
    if (res != VK_SUCCESS)
    {
        vk_result_check(res, __FILE__, __LINE__);
        return 1;
    }

    // Wait.
    dvz_queue_wait(queue);
    return 0;
}



/**
 * Submit a command buffer on its queue.
 *
 * @param cmds the commands wrapper
 */
void dvz_cmd_submit(DvzCommands* cmds)
{
    (void)dvz_cmd_submit_result(cmds);
}



void dvz_commands_destroy(DvzCommands* cmds)
{
    if (cmds == NULL)
    {
        return;
    }
    if (cmds->borrowed_recording)
    {
        log_error("cannot destroy a borrowed recording command buffer");
        cmds->count = 0;
        cmds->current = 0;
        cmds->borrowed_recording = false;
        dvz_memset(cmds->cmds, sizeof(cmds->cmds), 0, sizeof(cmds->cmds));
        dvz_obj_destroyed(&cmds->obj);
        return;
    }
    // NOTE: dvz_obj_is_created() is intentionally NOT used here. For commands, the CREATED
    // state is set after dvz_cmd_end() (recording complete), not after dvz_commands()
    // (allocation). count == 0 is the correct allocation-state guard.
    if (cmds->count == 0)
    {
        log_trace("skip destruction of already-destroyed commands");
        return;
    }
    log_trace("destroy commands");
    _commands_release(cmds);
    dvz_obj_destroyed(&cmds->obj);
}



void dvz_commands_wrap(DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds)
{

    ANN(cmds);
    ANN(device);

    cmds->device = device;
    cmds->count = 1;
    cmds->cmds[0] = vk_cmd;
    cmds->borrowed_recording = false;

    dvz_obj_created(&cmds->obj);
}



/**
 * Wrap an externally-owned Vulkan command buffer that is already recording.
 *
 * @param device the device
 * @param vk_cmd the borrowed recording Vulkan command buffer
 * @param[out] cmds the created command buffers
 */
void dvz_commands_wrap_borrowed_recording(
    DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds)
{
    dvz_commands_wrap(device, vk_cmd, cmds);
    cmds->borrowed_recording = true;
}
