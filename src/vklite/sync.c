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

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_sync.h"
#include "_vk_utils.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sync.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Wrapper allocation                                                                           */
/*************************************************************************************************/

/**
 * Allocate an empty fence wrapper.
 *
 * @return allocated fence wrapper, or NULL on allocation failure
 */
DvzFence* dvz_fence_create_wrapper(void)
{
    DvzFence* fence = (DvzFence*)dvz_calloc(1, sizeof(DvzFence));
    ANN(fence);
    return fence;
}



/**
 * Free a fence wrapper allocated by dvz_fence_create_wrapper().
 *
 * @param fence fence wrapper to free
 */
void dvz_fence_free(DvzFence* fence)
{
    if (fence == NULL)
    {
        return;
    }
    dvz_free(fence);
}



/**
 * Allocate an empty semaphore wrapper.
 *
 * @return allocated semaphore wrapper, or NULL on allocation failure
 */
DvzSemaphore* dvz_semaphore_create_wrapper(void)
{
    DvzSemaphore* semaphore = (DvzSemaphore*)dvz_calloc(1, sizeof(DvzSemaphore));
    ANN(semaphore);
    return semaphore;
}



/**
 * Free a semaphore wrapper allocated by dvz_semaphore_create_wrapper().
 *
 * @param semaphore semaphore wrapper to free
 */
void dvz_semaphore_free(DvzSemaphore* semaphore)
{
    if (semaphore == NULL)
    {
        return;
    }
    dvz_free(semaphore);
}



/**
 * Allocate an empty submit wrapper.
 *
 * @return allocated submit wrapper, or NULL on allocation failure
 */
DvzSubmit* dvz_submit_create_wrapper(void)
{
    DvzSubmit* submit = (DvzSubmit*)dvz_calloc(1, sizeof(DvzSubmit));
    ANN(submit);
    return submit;
}



/**
 * Free a submit wrapper allocated by dvz_submit_create_wrapper().
 *
 * @param submit submit wrapper to free
 */
void dvz_submit_free(DvzSubmit* submit)
{
    if (submit == NULL)
    {
        return;
    }
    dvz_free(submit);
}



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
    DvzBarrierImage* bimg, VkImageLayout old, VkImageLayout new_layout)
{
    ANN(bimg);
    bimg->oldLayout = old;
    bimg->newLayout = new_layout;
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
    dvz_memset(barriers, sizeof(*barriers), 0, sizeof(*barriers));
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
    if (barriers->info.memoryBarrierCount >= DVZ_MAX_BARRIERS)
    {
        log_error("too many memory barriers (max=%d)", DVZ_MAX_BARRIERS);
        return NULL;
    }
    DvzBarrierMemory* bmem = &barriers->bmems[barriers->info.memoryBarrierCount++];
    ANN(bmem);

    bmem->sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;

    return bmem;
}


DvzBarrierBuffer*
dvz_barriers_buffer(DvzBarriers* barriers, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
{
    ANN(barriers);
    if (barriers->info.bufferMemoryBarrierCount >= DVZ_MAX_BARRIERS)
    {
        log_error("too many buffer barriers (max=%d)", DVZ_MAX_BARRIERS);
        return NULL;
    }

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
    if (barriers->info.imageMemoryBarrierCount >= DVZ_MAX_BARRIERS)
    {
        log_error("too many image barriers (max=%d)", DVZ_MAX_BARRIERS);
        return NULL;
    }

    DvzBarrierImage* bimg = &barriers->bimg[barriers->info.imageMemoryBarrierCount++];

    bimg->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    bimg->image = img;

    // Default values.
    bimg->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bimg->subresourceRange.levelCount = 1;
    bimg->subresourceRange.layerCount = 1;

    return bimg;
}



void dvz_cmd_barriers(DvzCommands* cmds, DvzBarriers* barriers)
{
    ANN(cmds);
    ANN(barriers);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    log_trace(
        "record barrier (%d memory barriers, %d buffer barriers, %d image barriers)",
        barriers->info.memoryBarrierCount, barriers->info.bufferMemoryBarrierCount,
        barriers->info.imageMemoryBarrierCount);
    vkCmdPipelineBarrier2(cmd, &barriers->info);
}



/**
 * Return the number of recorded memory barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return the memory-barrier count
 */
uint32_t dvz_barriers_memory_count(DvzBarriers* barriers)
{
    ANN(barriers);
    return barriers->info.memoryBarrierCount;
}



/**
 * Return the number of recorded buffer barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return the buffer-barrier count
 */
uint32_t dvz_barriers_buffer_count(DvzBarriers* barriers)
{
    ANN(barriers);
    return barriers->info.bufferMemoryBarrierCount;
}



/**
 * Return the number of recorded image barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return the image-barrier count
 */
uint32_t dvz_barriers_image_count(DvzBarriers* barriers)
{
    ANN(barriers);
    return barriers->info.imageMemoryBarrierCount;
}



/**
 * Return the dependency flags configured on a barrier set.
 *
 * @param barriers the barrier set
 * @return the dependency flags
 */
VkDependencyFlags dvz_barriers_dependency_flags(DvzBarriers* barriers)
{
    ANN(barriers);
    return barriers->info.dependencyFlags;
}



/**
 * Return the maximum number of barriers supported per barrier type.
 *
 * @param barriers the barrier set
 * @return the barrier capacity
 */
uint32_t dvz_barriers_capacity(DvzBarriers* barriers)
{
    ANN(barriers);
    return DVZ_MAX_BARRIERS;
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

    VkDevice vkd = dvz_device_handle(fence->device);
    ANNVK(vkd);
    VK_CHECK_RESULT(vkCreateFence(vkd, &info, NULL, &fence->vk_fence));

    dvz_obj_created(&fence->obj);
}



bool dvz_fence_wait(DvzFence* fence)
{
    ANN(fence);
    VkDevice vkd = dvz_device_handle(fence->device);
    ANNVK(vkd);
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        VkResult res = vkWaitForFences(vkd, 1, &fence->vk_fence, VK_TRUE, UINT64_MAX);
        if (res != VK_SUCCESS)
        {
            log_error("failed to wait for fence (%d)", res);
            return false;
        }
        return true;
    }
    else
    {
        log_trace("skip wait for null fence");
        return false;
    }
}



VkFence dvz_fence_handle(DvzFence* fence)
{
    ANN(fence);
    return fence->vk_fence;
}



bool dvz_fence_ready(DvzFence* fence)
{
    ANN(fence);
    VkDevice vkd = dvz_device_handle(fence->device);
    ANNVK(vkd);
    VK_RETURN_RESULT(vkGetFenceStatus(vkd, fence->vk_fence));
    return (bool)out;
}



void dvz_fence_reset(DvzFence* fence)
{
    ANN(fence);
    VkDevice vkd = dvz_device_handle(fence->device);
    ANNVK(vkd);
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        vkResetFences(vkd, 1, &fence->vk_fence);
    }
}



void dvz_fence_destroy(DvzFence* fence)
{
    if (fence == NULL)
    {
        return;
    }
    if (!dvz_obj_is_created(&fence->obj))
    {
        log_trace("skip destruction of already-destroyed fence");
        return;
    }

    log_trace("destroying fence...");
    VkDevice vkd = dvz_device_handle(fence->device);
    ANNVK(vkd);
    if (fence->vk_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(vkd, fence->vk_fence, NULL);
        fence->vk_fence = VK_NULL_HANDLE;
    }
    dvz_obj_destroyed(&fence->obj);
}



/*************************************************************************************************/
/*  Semaphore                                                                                    */
/*************************************************************************************************/

void dvz_semaphore(DvzDevice* device, DvzSemaphore* semaphore)
{
    ANN(device);

    semaphore->device = device;
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(vkd, &info, NULL, &semaphore->vk_semaphore));
    log_trace("created semaphore %#x", semaphore->vk_semaphore);
    dvz_obj_created(&semaphore->obj);
}



void dvz_semaphore_timeline(
    DvzDevice* device, uint64_t value, DvzSemaphore* semaphore,
    VkExternalSemaphoreHandleTypeFlags handle_type)
{
    ANN(device);
    ANN(semaphore);

    semaphore->device = device;
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    VkSemaphoreTypeCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = value,
    };

    VkExportSemaphoreCreateInfo export_info = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = handle_type,
        .pNext = &timeline_info,
    };

    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (handle_type != 0)
    {
        info.pNext = &export_info;
    }
    else
    {
        info.pNext = &timeline_info;
    }
    VK_CHECK_RESULT(vkCreateSemaphore(vkd, &info, NULL, &semaphore->vk_semaphore));
    log_trace("created timeline semaphore %#x", semaphore->vk_semaphore);
    dvz_obj_created(&semaphore->obj);
}



void dvz_semaphore_signal(DvzSemaphore* semaphore, uint64_t value)
{
    ANN(semaphore);
    VkSemaphoreSignalInfo signalInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = semaphore->vk_semaphore,
        .value = value,
    };
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);
    vkSignalSemaphore(vkd, &signalInfo);
}



void dvz_semaphore_wait(DvzSemaphore* semaphore, uint64_t value)
{
    ANN(semaphore);
    VkSemaphoreWaitInfo waitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &semaphore->vk_semaphore,
        .pValues = &value,
    };
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);
    VK_CHECK_RESULT(vkWaitSemaphores(vkd, &waitInfo, UINT64_MAX));
}



uint64_t dvz_semaphore_query(DvzSemaphore* semaphore)
{
    ANN(semaphore);
    uint64_t current = 0;
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);
    vkGetSemaphoreCounterValue(vkd, semaphore->vk_semaphore, &current);
    return current;
}



VkSemaphore dvz_semaphore_handle(DvzSemaphore* semaphore)
{
    ANN(semaphore);
    return semaphore->vk_semaphore;
}



/**
 * Export a semaphore as a Unix file descriptor.
 *
 * @param semaphore semaphore to export
 * @param handle_type external handle type requested by the caller
 * @return file descriptor on success, -1 on failure or unsupported platforms
 */
DvzExternalHandle
dvz_semaphore_export(DvzSemaphore* semaphore, VkExternalSemaphoreHandleTypeFlags handle_type)
{
    ANN(semaphore);
    if (handle_type == 0)
    {
        return DVZ_EXTERNAL_HANDLE_INVALID;
    }

#if OS_UNIX
    VkSemaphoreGetFdInfoKHR fd_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = semaphore->vk_semaphore,
        .handleType = handle_type,
    };
    int fd = -1;
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);
    VkResult res = vkGetSemaphoreFdKHR(vkd, &fd_info, &fd);
    if (res != VK_SUCCESS)
    {
        log_warn("vkGetSemaphoreFdKHR failed for semaphore (%d)", res);
        return DVZ_EXTERNAL_HANDLE_INVALID;
    }
    return (DvzExternalHandle)fd;
#elif OS_WINDOWS
    VkSemaphoreGetWin32HandleInfoKHR handle_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
        .semaphore = semaphore->vk_semaphore,
        .handleType = handle_type,
    };
    HANDLE handle = NULL;
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);
    VkResult res = vkGetSemaphoreWin32HandleKHR(vkd, &handle_info, &handle);
    if (res != VK_SUCCESS || handle == NULL)
    {
        log_warn("vkGetSemaphoreWin32HandleKHR failed for semaphore (%d)", res);
        return DVZ_EXTERNAL_HANDLE_INVALID;
    }
    return (DvzExternalHandle)(intptr_t)handle;
#else
    (void)handle_type;
    return DVZ_EXTERNAL_HANDLE_INVALID;
#endif
}



int dvz_semaphore_export_fd(DvzSemaphore* semaphore, VkExternalSemaphoreHandleTypeFlags handle_type)
{
#if OS_UNIX
    return (int)dvz_semaphore_export(semaphore, handle_type);
#else
    (void)semaphore;
    (void)handle_type;
    return -1;
#endif
}



void dvz_semaphore_destroy(DvzSemaphore* semaphore)
{
    if (semaphore == NULL)
    {
        return;
    }
    if (!dvz_obj_is_created(&semaphore->obj))
    {
        log_trace("skip destruction of already-destroyed semaphore");
        return;
    }

    log_trace("destroying semaphore...");
    VkDevice vkd = dvz_device_handle(semaphore->device);
    ANNVK(vkd);

    if (semaphore->vk_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(vkd, semaphore->vk_semaphore, NULL);
        semaphore->vk_semaphore = VK_NULL_HANDLE;
    }
    dvz_obj_destroyed(&semaphore->obj);
}



/*************************************************************************************************/
/*  Submission                                                                                   */
/*************************************************************************************************/

/**
 * Return whether a submission wrapper has been initialized.
 *
 * @param submit the submission
 * @return true when the wrapper is initialized
 */
static bool _submit_initialized(DvzSubmit* submit)
{
    ANN(submit);
    return submit->info.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO_2 &&
           submit->info.pWaitSemaphoreInfos == submit->wait &&
           submit->info.pSignalSemaphoreInfos == submit->signal &&
           submit->info.pCommandBufferInfos == submit->cmds;
}



/**
 * Initialize or reset a submission.
 *
 * @param submit the submission
 */
void dvz_submit(DvzSubmit* submit)
{
    ANN(submit);
    dvz_memset(submit, sizeof(*submit), 0, sizeof(*submit));
    submit->info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit->info.pWaitSemaphoreInfos = submit->wait;
    submit->info.pSignalSemaphoreInfos = submit->signal;
    submit->info.pCommandBufferInfos = submit->cmds;
}



/**
 * Add a semaphore to wait on.
 *
 * @param submit the submission
 * @param semaphore the semaphore handle to wait on
 * @param value the timeline value, when relevant
 * @param stage the dependent pipeline stage mask
 */
void dvz_submit_wait(
    DvzSubmit* submit, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage)
{
    ANN(submit);
    ANNVK(semaphore);
    if (submit->info.waitSemaphoreInfoCount >= DVZ_MAX_SEMAPHORES)
    {
        log_error("too many wait semaphores in submit (max=%d)", DVZ_MAX_SEMAPHORES);
        return;
    }

    VkSemaphoreSubmitInfo* info = &submit->wait[submit->info.waitSemaphoreInfoCount++];
    info->sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info->semaphore = semaphore;
    info->value = value;
    info->stageMask = stage;
}



/**
 * Add a semaphore to signal.
 *
 * @param submit the submission
 * @param semaphore the semaphore handle to signal
 * @param value the timeline value, when relevant
 * @param stage the dependent pipeline stage mask
 */
void dvz_submit_signal(
    DvzSubmit* submit, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage)
{
    ANN(submit);
    ANNVK(semaphore);
    if (submit->info.signalSemaphoreInfoCount >= DVZ_MAX_SEMAPHORES)
    {
        log_error("too many signal semaphores in submit (max=%d)", DVZ_MAX_SEMAPHORES);
        return;
    }

    VkSemaphoreSubmitInfo* info = &submit->signal[submit->info.signalSemaphoreInfoCount++];
    info->sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info->semaphore = semaphore;
    info->value = value;
    info->stageMask = stage;
}



/**
 * Add a command buffer to a submission.
 *
 * @param submit the submission
 * @param cmd the command buffer handle
 */
void dvz_submit_command(DvzSubmit* submit, VkCommandBuffer cmd)
{
    ANN(submit);
    ANNVK(cmd);
    if (submit->info.commandBufferInfoCount >= DVZ_MAX_COMMANDS)
    {
        log_error("too many command buffers in submit (max=%d)", DVZ_MAX_COMMANDS);
        return;
    }

    VkCommandBufferSubmitInfo* info = &submit->cmds[submit->info.commandBufferInfoCount++];
    info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info->commandBuffer = cmd;
}



/**
 * Return the number of wait semaphores configured on a submission.
 *
 * @param submit the submission
 * @return the wait-semaphore count
 */
uint32_t dvz_submit_wait_count(DvzSubmit* submit)
{
    ANN(submit);
    return submit->info.waitSemaphoreInfoCount;
}



/**
 * Return the number of signal semaphores configured on a submission.
 *
 * @param submit the submission
 * @return the signal-semaphore count
 */
uint32_t dvz_submit_signal_count(DvzSubmit* submit)
{
    ANN(submit);
    return submit->info.signalSemaphoreInfoCount;
}



/**
 * Return the number of command buffers configured on a submission.
 *
 * @param submit the submission
 * @return the command-buffer count
 */
uint32_t dvz_submit_command_count(DvzSubmit* submit)
{
    ANN(submit);
    return submit->info.commandBufferInfoCount;
}



/**
 * Return whether a submission has no recorded waits, signals, or command buffers.
 *
 * @param submit the submission
 * @return true when the submission is empty
 */
bool dvz_submit_is_empty(DvzSubmit* submit)
{
    ANN(submit);
    return dvz_submit_wait_count(submit) == 0 && dvz_submit_signal_count(submit) == 0 &&
           dvz_submit_command_count(submit) == 0;
}



/**
 * Send a submission to a queue.
 *
 * @param submit the submission
 * @param queue the queue receiving the work
 * @param fence the optional fence signaled on completion
 * @return Vulkan result code cast to int32_t
 */
int32_t dvz_submit_send(DvzSubmit* submit, VkQueue queue, VkFence fence)
{
    ANN(submit);
    ANNVK(queue);
    if (!_submit_initialized(submit))
    {
        log_error("cannot send an uninitialized submit wrapper");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (submit->info.commandBufferInfoCount == 0)
    {
        log_error("cannot submit without command buffers");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return (int32_t)vkQueueSubmit2(queue, 1, &submit->info, fence);
}
