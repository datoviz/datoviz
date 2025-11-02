/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Slots                                                                                        */
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

typedef struct DvzSlots DvzSlots;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize pipeline slots (aka Vulkan descriptor set layout).
 *
 * @param device the device
 * @param[out] slots the created slots
 */
DVZ_EXPORT void dvz_slots(DvzDevice* device, DvzSlots* slots);



/**
 * Set the slots descriptor.
 *
 * @param slots the slots
 * @param set the set index
 * @param binding the binding index within that set
 * @param array_count the number of descriptor if using descriptor arrays
 * @param stages the shader stages to enable
 * @param type the descriptor type for that slot
 */
DVZ_EXPORT void dvz_slots_binding(
    DvzSlots* slots, uint32_t set, uint32_t binding, uint32_t array_count,
    VkShaderStageFlags stages, VkDescriptorType type);



/**
 * Set up push constants.
 *
 * @param slots the slots
 * @param stages the slots stages that will access the push constant
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 */
DVZ_EXPORT void
dvz_slots_push(DvzSlots* slots, VkShaderStageFlagBits stages, DvzSize offset, DvzSize size);



/**
 * Create the slots after they have been set up.
 *
 * @param slots the slots
 * @returns the Vulkan creation result code
 */
DVZ_EXPORT int dvz_slots_create(DvzSlots* slots);



/**
 * Return the pipeline layout Vulkan handle.
 *
 * @param slots the slots
 * @returns the pipeline layout
 */
DVZ_EXPORT VkPipelineLayout dvz_slots_handle(DvzSlots* slots);



/**
 * Destroy the slots.
 *
 * @param slots the slots
 */
DVZ_EXPORT void dvz_slots_destroy(DvzSlots* slots);



EXTERN_C_OFF
