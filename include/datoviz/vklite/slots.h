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

#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzSlots DvzSlots;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BINDINGS       16
#define DVZ_MAX_PUSH_CONSTANTS 8
#define DVZ_MAX_SETS           4


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty slots wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_slots(), configure, call dvz_slots_create() once, then
 * destroy before any recreate and free only if this wrapper came from
 * dvz_slots_create_wrapper().
 *
 * @return allocated slots wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSlots* dvz_slots_create_wrapper(void);



/**
 * Initialize pipeline slots (aka Vulkan descriptor set layout).
 *
 * This prepares the wrapper for configuration. Call dvz_slots_create() once
 * after declaring bindings and push-constant ranges. Recreating live slots
 * requires dvz_slots_destroy() first.
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
 * This function creates the wrapped Vulkan descriptor-set layouts and pipeline
 * layout exactly once per live wrapper. Call dvz_slots_destroy() before
 * attempting to create them again.
 *
 * @param slots the slots
 * @return 0 on success, non-zero on Vulkan or Datoviz state failure
 */
DVZ_EXPORT int dvz_slots_create(DvzSlots* slots);



/**
 * Return the pipeline layout Vulkan handle.
 *
 * @param slots the slots
 * @return borrowed pipeline-layout handle, or `VK_NULL_HANDLE` when not created
 */
DVZ_EXPORT VkPipelineLayout dvz_slots_handle(DvzSlots* slots);



/**
 * Create a pipeline layout that combines two existing descriptor set layouts.
 *
 * The resulting pipeline layout covers set 0 (from layout0) and set 1 (from layout1).
 * The returned VkPipelineLayout must be destroyed by the caller with vkDestroyPipelineLayout.
 *
 * @param device the device
 * @param layout0 descriptor set layout for set 0
 * @param layout1 descriptor set layout for set 1
 * @return the combined pipeline layout, or VK_NULL_HANDLE on failure
 */
DVZ_EXPORT VkPipelineLayout dvz_slots_combined_pipeline_layout(
    DvzDevice* device, VkDescriptorSetLayout layout0, VkDescriptorSetLayout layout1);



/**
 * Return the device that owns a slots wrapper.
 *
 * @param slots the slots
 * @return borrowed owning device, valid at least as long as the slots wrapper
 */
DVZ_EXPORT DvzDevice* dvz_slots_device(DvzSlots* slots);



/**
 * Return the number of descriptor sets configured on a slots wrapper.
 *
 * @param slots the slots
 * @return the descriptor-set count
 */
DVZ_EXPORT uint32_t dvz_slots_set_count(DvzSlots* slots);



/**
 * Return the number of bindings configured for a descriptor set.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @return the binding count for that set
 */
DVZ_EXPORT uint32_t dvz_slots_binding_count(DvzSlots* slots, uint32_t set);



/**
 * Return the number of configured push-constant ranges.
 *
 * @param slots the slots
 * @return the push-constant range count
 */
DVZ_EXPORT uint32_t dvz_slots_push_count(DvzSlots* slots);



/**
 * Return the descriptor type configured for a slot binding.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @param binding the binding index within the set
 * @return the descriptor type
 */
DVZ_EXPORT VkDescriptorType dvz_slots_descriptor_type(DvzSlots* slots, uint32_t set, uint32_t binding);



/**
 * Return the descriptor-set layout handle for a set.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @return borrowed descriptor-set layout; `set` must be less than `DVZ_MAX_SETS`
 */
DVZ_EXPORT VkDescriptorSetLayout dvz_slots_set_layout(DvzSlots* slots, uint32_t set);



/**
 * Destroy the slots.
 *
 * This releases the wrapped Vulkan layouts and returns the wrapper to a
 * reusable initialized state.
 *
 * @param slots the slots
 */
DVZ_EXPORT void dvz_slots_destroy(DvzSlots* slots);



/**
 * Free a slots wrapper allocated by dvz_slots_create_wrapper().
 *
 * @param slots slots wrapper to free
 */
DVZ_EXPORT void dvz_slots_free(DvzSlots* slots);



EXTERN_C_OFF
