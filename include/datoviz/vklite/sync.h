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

typedef struct DvzBarrierMemory DvzBarrierMemory;
typedef struct DvzBarrierBuffer DvzBarrierBuffer;
typedef struct DvzBarrierImage DvzBarrierImage;
typedef struct DvzBarriers DvzBarriers;


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



EXTERN_C_ON

/*************************************************************************************************/
/*  Memory barrier                                                                               */
/*************************************************************************************************/

/**
 * Create a memory barrier.
 *
 * @param bmem the memory barrier
 */
DVZ_EXPORT void dvz_barrier_memory(DvzBarrierMemory* bmem);



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
 * Create a buffer memory barrier.
 *
 * @param bbuf the buffer barrier
 * @param buffer the buffer
 * @param offset the offset
 * @param size the size
 */
DVZ_EXPORT void dvz_barrier_buffer( //
    DvzBarrierBuffer* bbuf, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);



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
 * Create an image memory barrier.
 *
 * @param bimg the image barrier
 * @param img the image
 */
DVZ_EXPORT void dvz_barrier_image( //
    DvzBarrierImage* bimg, VkImage img);



/**
 * Set the barrier stages.
 *
 * @param bimg the image barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_image_stage( //
    DvzBarrierBuffer* bimg, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bimg the image barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void dvz_barrier_image_access( //
    DvzBarrierBuffer* bimg, VkAccessFlags2 src, VkAccessFlags2 dst);



/**
 * Set the image layout change.
 *
 * @param bimg the image barrier
 * @param old the old image layout
 * @param new the new image layout
 */
DVZ_EXPORT void dvz_barrier_image_layout( //
    DvzBarrierBuffer* bimg, VkImageLayout old, VkImageLayout new);



/**
 * Set the queue family transfer.
 *
 * @param bimg the image barrier
 * @param src the source queue family index
 * @param dst the destination queue family index
 */
DVZ_EXPORT void dvz_barrier_image_queue( //
    DvzBarrierBuffer* bimg, uint32_t src, uint32_t dst);



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
 * @param bmem a memory barrier
 */
DVZ_EXPORT void dvz_barriers_memory(DvzBarriers* barriers, DvzBarrierMemory* bmem);



/**
 * Add a buffer barrier to a set of barriers
 *
 * @param barriers the set of barriers
 * @param bbuf a buffer barrier
 */
DVZ_EXPORT void dvz_barriers_buffer(DvzBarriers* barriers, DvzBarrierBuffer* bbuf);



/**
 * Add an image barrier to a set of barriers
 *
 * @param barriers the set of barriers
 * @param bimg an image barrier
 */
DVZ_EXPORT void dvz_barriers_image(DvzBarriers* barriers, DvzBarrierImage* bimg);



/**
 * Record a set of barriers in a command buffer.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param barriers the set of barriers
 */
DVZ_EXPORT void dvz_cmd_barriers(DvzCommands* cmds, uint32_t idx, DvzBarriers* barriers);



EXTERN_C_OFF
