/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Descriptors                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/slots.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzSlots DvzSlots;
typedef struct DvzDescriptors DvzDescriptors;
typedef struct DvzCommands DvzCommands;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty descriptor wrapper.
 *
 * This wrapper is intentionally lightweight: it refers to descriptor sets allocated from the
 * device-owned descriptor pool rather than owning an independent Vulkan object with a separate
 * destroy path.
 *
 * @return allocated descriptor wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzDescriptors* dvz_descriptors_create_wrapper(void);



/**
 * Initialize and allocate descriptors.
 *
 * This is a one-shot pool-backed allocation helper. The wrapper does not own the descriptor pool
 * and does not provide an independent Vulkan destroy entry point. Descriptor sets remain valid
 * only while the parent device/pool stays alive, and callers must not allocate into the same
 * wrapper twice without discarding it and starting from a fresh wrapper. This
 * intentionally differs from the heavier create/destroy/free wrappers used for
 * buffers, images, pipelines, and samplers.
 *
 * @param slots the slots
 * @param[out] descriptors the created descriptors
 */
DVZ_EXPORT void dvz_descriptors(DvzSlots* slots, DvzDescriptors* descriptors);



/**
 * Return the number of descriptor sets allocated by the wrapper.
 *
 * @param descriptors the descriptors
 * @returns the descriptor set count
 */
DVZ_EXPORT uint32_t dvz_descriptors_set_count(DvzDescriptors* descriptors);



/**
 * Return a Vulkan descriptor-set handle by set index.
 *
 * @param descriptors the descriptors
 * @param set the descriptor set index
 * @returns the descriptor set handle
 */
DVZ_EXPORT VkDescriptorSet dvz_descriptors_handle(DvzDescriptors* descriptors, uint32_t set);



/**
 * Bind a buffer.
 *
 * @param descriptors the descriptors
 * @param set the descriptor set index
 * @param binding the descriptor binding index
 * @param array_idx the array index
 * @param buffer the buffer
 * @param offset the offset, in bytes
 * @param size the size, in bytes
 */
DVZ_EXPORT void dvz_descriptors_buffer(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkBuffer buffer, DvzSize offset, DvzSize size);



/**
 * Bind an image.
 *
 * @param descriptors the descriptors
 * @param set the descriptor set index
 * @param binding the descriptor binding index
 * @param array_idx the array index
 * @param layout the image layout
 * @param view the image view
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_descriptors_image(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkImageLayout layout, VkImageView view, VkSampler sampler);



/**
 * Bind descriptors in a command buffer.
 *
 * @param cmds the commands
 * @param idx the command index
 * @param bind_point graphics or compute pipeline
 * @param descriptors the descriptors
 * @param first_set the index of the first set to bind within the descriptors
 * @param set_count the number of sets to bind
 * @param dynamic_count the number of dynamic uniforms
 * @param dynamic_idxs the indices of the dynamic uniforms
 */
DVZ_EXPORT void dvz_cmd_bind_descriptors(
    DvzCommands* cmds, VkPipelineBindPoint bind_point, DvzDescriptors* descriptors,
    uint32_t first_set, uint32_t set_count, uint32_t dynamic_count, uint32_t* dynamic_idxs);



/**
 * Free a descriptor wrapper allocated by dvz_descriptors_create().
 *
 * This releases the CPU-side wrapper and returns its Vulkan descriptor sets to the parent
 * device-owned descriptor pool. The wrapper must be freed before the parent device is destroyed.
 *
 * @param descriptors descriptor wrapper to free
 */
DVZ_EXPORT void dvz_descriptors_free(DvzDescriptors* descriptors);



EXTERN_C_OFF
