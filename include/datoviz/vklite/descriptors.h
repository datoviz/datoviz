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

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzSlots DvzSlots;
typedef struct DvzDescriptors DvzDescriptors;
typedef struct DvzCommands DvzCommands;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize and allocate descriptors.
 *
 * @param slots the slots
 * @param[out] descriptors the created descriptors
 */
DVZ_EXPORT void dvz_descriptors(DvzSlots* slots, DvzDescriptors* descriptors);



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
    DvzCommands* cmds, uint32_t idx, VkPipelineBindPoint bind_point, DvzDescriptors* descriptors,
    uint32_t first_set, uint32_t set_count, uint32_t dynamic_count, uint32_t* dynamic_idxs);



EXTERN_C_OFF
