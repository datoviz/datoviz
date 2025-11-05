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

#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzCommands DvzCommands;

typedef struct VkMemoryBarrier2 DvzBarrierMemory;
typedef struct VkBufferMemoryBarrier2 DvzBarrierBuffer;
typedef struct VkImageMemoryBarrier2 DvzBarrierImage;
typedef struct DvzBarriers DvzBarriers;
typedef struct DvzFence DvzFence;
typedef struct DvzSemaphore DvzSemaphore;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BARRIERS 4



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBarriers
{
    VkDependencyInfo info;
    DvzBarrierMemory bmems[DVZ_MAX_BARRIERS];
    DvzBarrierBuffer bbufs[DVZ_MAX_BARRIERS];
    DvzBarrierImage bimg[DVZ_MAX_BARRIERS];
};



struct DvzFence
{
    DvzObject obj;
    DvzDevice* device;
    VkFence vk_fence;
};



struct DvzSemaphore
{
    DvzObject obj;
    DvzDevice* device;
    VkSemaphore vk_semaphore;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Memory barrier                                                                               */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bmem the memory barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_memory_stage(
    DvzBarrierMemory* bmem, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bmem the memory barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void
dvz_barrier_memory_access(DvzBarrierMemory* bmem, VkAccessFlags2 src, VkAccessFlags2 dst);



/*************************************************************************************************/
/*  Buffer barrier                                                                               */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bbuf the buffer barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_buffer_stage( //
    DvzBarrierBuffer* bbuf, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bbuf the buffer barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void dvz_barrier_buffer_access( //
    DvzBarrierBuffer* bbuf, VkAccessFlags2 src, VkAccessFlags2 dst);



/**
 * Set the queue family transfer.
 *
 * @param bbuf the buffer barrier
 * @param src the source queue family index
 * @param dst the destination queue family index
 */
DVZ_EXPORT void dvz_barrier_buffer_queue( //
    DvzBarrierBuffer* bbuf, uint32_t src, uint32_t dst);



/*************************************************************************************************/
/*  Image barrier                                                                                */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bimg the image barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_image_stage( //
    DvzBarrierImage* bimg, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bimg the image barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void dvz_barrier_image_access( //
    DvzBarrierImage* bimg, VkAccessFlags2 src, VkAccessFlags2 dst);



/**
 * Set the image layout change.
 *
 * @param bimg the image barrier
 * @param old the old image layout
 * @param new the new image layout
 */
DVZ_EXPORT void dvz_barrier_image_layout( //
    DvzBarrierImage* bimg, VkImageLayout old, VkImageLayout new);



/**
 * Set the queue family transfer.
 *
 * @param bimg the image barrier
 * @param src the source queue family index
 * @param dst the destination queue family index
 */
DVZ_EXPORT void dvz_barrier_image_queue( //
    DvzBarrierImage* bimg, uint32_t src, uint32_t dst);



/**
 * Set the barrier image aspect flags.
 *
 * @param bimg the image barrier
 * @param aspect the image aspect flags
 */
DVZ_EXPORT void dvz_barrier_image_aspect( //
    DvzBarrierImage* bimg, VkImageAspectFlags aspect);



/**
 * Set the MIP images of an image barrier.
 *
 * @param bimg the image barrier
 * @param base the base index
 * @param count the number of MIP images
 */
DVZ_EXPORT void dvz_barrier_image_mip( //
    DvzBarrierImage* bimg, uint32_t base, uint32_t count);



/**
 * Set the array layers of an image barrier.
 *
 * @param bimg the image barrier
 * @param base the base array layer index
 * @param count the number of array layers
 */
DVZ_EXPORT void dvz_barrier_image_layers( //
    DvzBarrierImage* bimg, uint32_t base, uint32_t count);



/*************************************************************************************************/
/*  Barriers                                                                                     */
/*************************************************************************************************/

/**
 * Create a set of barriers.
 *
 * @param barriers the set of barriers
 */
DVZ_EXPORT void dvz_barriers(DvzBarriers* barriers);



/**
 * Set the dependency flags of a set of barriers
 *
 * @param barriers the set of barriers
 * @param flags the dependency flags
 */
DVZ_EXPORT void dvz_barriers_flags(DvzBarriers* barriers, VkDependencyFlags flags);



/**
 * Add a memory barrier to a set of barriers
 *
 * @param barriers the set of barriers
 * @returns the memory barrier
 */
DVZ_EXPORT DvzBarrierMemory* dvz_barriers_memory(DvzBarriers* barriers);



/**
 * Add a buffer barrier to a set of barriers
 *
 * @param barriers the set of barriers
 * @param buffer the buffer
 * @param offset the offset
 * @param size the size
 * @returns the buffer barrier
 */
DVZ_EXPORT DvzBarrierBuffer* dvz_barriers_buffer(
    DvzBarriers* barriers, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);



/**
 * Add an image barrier to a set of barriers
 *
 * @param barriers the set of barriers
 * @param img an image
 * @returns the image barrier
 */
DVZ_EXPORT DvzBarrierImage* dvz_barriers_image(DvzBarriers* barriers, VkImage img);



/**
 * Record a set of barriers in a command buffer.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param barriers the set of barriers
 */
DVZ_EXPORT void dvz_cmd_barriers(DvzCommands* cmds, uint32_t idx, DvzBarriers* barriers);



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

/**
 * Initialize a fence (CPU-GPU synchronization).
 *
 * @param device the device
 * @param signaled whether the fence is created in the signaled state or not
 * @param[out] the created fence
 */
DVZ_EXPORT void dvz_fence(DvzDevice* device, bool signaled, DvzFence* fence);



/**
 * Wait on the GPU until a fence is signaled.
 *
 * @param fence the fence
 */
DVZ_EXPORT void dvz_fence_wait(DvzFence* fence);



/**
 * Return whether a fence is ready.
 *
 * @param fence the fence
 */
DVZ_EXPORT bool dvz_fence_ready(DvzFence* fence);



/**
 * Rset the state of a fence.
 *
 * @param fence the fence
 */
DVZ_EXPORT void dvz_fence_reset(DvzFence* fence);



/**
 * Destroy fence.
 *
 * @param fence the fence
 */
DVZ_EXPORT void dvz_fence_destroy(DvzFence* fence);



/*************************************************************************************************/
/*  Semaphore                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a semaphore (GPU-GPU synchronization).
 *
 * @param device the device
 * @param[out] the created semaphore
 */
DVZ_EXPORT void dvz_semaphore(DvzDevice* device, DvzSemaphore* semaphore);



/**
 * Recreate a semaphore.
 *
 * @param semaphore the semaphore
 */
DVZ_EXPORT void dvz_semaphore_recreate(DvzSemaphore* semaphore);



/**
 * Destroy semaphore.
 *
 * @param semaphore the semaphore
 */
DVZ_EXPORT void dvz_semaphore_destroy(DvzSemaphore* semaphore);



EXTERN_C_OFF
