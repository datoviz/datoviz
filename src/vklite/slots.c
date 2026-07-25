/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stddef.h>
#include <volk.h>

#include "_alloc.h"
#include "_vk_utils.h"
#include "_assertions.h"
#include "_log.h"
#include "_slots.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a slots wrapper owns live Vulkan handles.
 *
 * @param slots the slots wrapper
 * @return whether at least one descriptor-set layout or pipeline layout exists
 */
static bool _slots_has_handles(DvzSlots* slots)
{
    ANN(slots);
    if (slots->pipeline_layout != VK_NULL_HANDLE)
        return true;
    for (uint32_t set = 0; set < slots->set_count; set++)
    {
        if (slots->set_layouts[set] != VK_NULL_HANDLE)
            return true;
    }
    return false;
}



/**
 * Release all Vulkan handles currently owned by a slots wrapper.
 *
 * @param slots the slots wrapper
 */
static void _slots_release_handles(DvzSlots* slots)
{
    ANN(slots);
    ANN(slots->device);
    VkDevice vkd = dvz_device_handle(slots->device);
    ANNVK(vkd);

    log_trace("destroying %d descriptor set layout(s)", slots->set_count);
    for (uint32_t set = 0; set < slots->set_count; set++)
    {
        if (slots->set_layouts[set] != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(vkd, slots->set_layouts[set], NULL);
            slots->set_layouts[set] = VK_NULL_HANDLE;
        }
    }

    log_trace("destroying the pipeline layout...");
    if (slots->pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(vkd, slots->pipeline_layout, NULL);
        slots->pipeline_layout = VK_NULL_HANDLE;
    }
    log_trace("pipeline layout destroyed");
}



/**
 * Return whether all configured push-constant ranges fit the physical-device limit.
 *
 * @param slots the slots wrapper
 * @return whether every push-constant range is valid
 */
static bool _slots_push_ranges_valid(DvzSlots* slots)
{
    ANN(slots);
    ANN(slots->device);

    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(dvz_device_physical_device(slots->device), &props);
    uint64_t limit = props.limits.maxPushConstantsSize;

    for (uint32_t i = 0; i < slots->push_count; i++)
    {
        uint64_t offset = slots->pushs[i].offset;
        uint64_t size = slots->pushs[i].size;
        if (offset > limit || size > limit - offset)
        {
            uint64_t end = offset > UINT64_MAX - size ? UINT64_MAX : offset + size;
            log_error(
                "push constant range %" PRIu64 "..%" PRIu64
                " exceeds device limit %" PRIu64,
                offset, end, limit);
            return false;
        }
    }
    return true;
}



/**
 * Allocate an empty slots wrapper.
 *
 * @return allocated slots wrapper, or NULL on allocation failure
 */
DvzSlots* dvz_slots_create_wrapper(void)
{
    DvzSlots* slots = (DvzSlots*)dvz_calloc(1, sizeof(DvzSlots));
    ANN(slots);
    return slots;
}



void dvz_slots(DvzDevice* device, DvzSlots* slots)
{
    ANN(device);
    ANN(slots);
    dvz_memset(slots, sizeof(*slots), 0, sizeof(*slots));
    slots->device = device;
    dvz_obj_init(&slots->obj);
}



void dvz_slots_binding(
    DvzSlots* slots, uint32_t set, uint32_t binding, uint32_t array_count,
    VkShaderStageFlags stages, VkDescriptorType type)
{
    ANN(slots);
    ASSERT(set < DVZ_MAX_SETS);
    ASSERT(binding < DVZ_MAX_BINDINGS);
    slots->set_count = DVZ_MAX(set + 1, slots->set_count);
    slots->binding_counts[set] = DVZ_MAX(binding + 1, slots->binding_counts[set]);

    slots->bindings[set][binding].binding = binding;
    slots->bindings[set][binding].descriptorCount = array_count;
    slots->bindings[set][binding].stageFlags = stages;
    slots->bindings[set][binding].descriptorType = type;
}



void dvz_slots_push(DvzSlots* slots, VkShaderStageFlagBits stages, DvzSize offset, DvzSize size)
{
    ANN(slots);
    if (slots->push_count >= 1)
    {
        log_warn("only one push constant is supported for now");
        return;
    }
    slots->pushs[0].offset = offset;
    slots->pushs[0].size = size;
    slots->pushs[0].stageFlags = stages;
    slots->push_count = 1;
}



DvzResult dvz_cmd_push_constants(
    DvzCommands* cmds, DvzSlots* slots, VkShaderStageFlags stages, DvzSize offset, DvzSize size,
    const void* data)
{
    if (
        cmds == NULL || slots == NULL || data == NULL || stages == 0 || size == 0 ||
        offset > UINT32_MAX || size > UINT32_MAX || offset % 4 != 0 || size % 4 != 0 ||
        slots->pipeline_layout == VK_NULL_HANDLE)
    {
        return DVZ_ERROR;
    }

    bool covered = false;
    for (uint32_t i = 0; i < slots->push_count; i++)
    {
        const VkPushConstantRange* range = &slots->pushs[i];
        uint64_t range_end = (uint64_t)range->offset + range->size;
        uint64_t update_end = (uint64_t)offset + size;
        if (
            offset >= range->offset && update_end <= range_end &&
            (range->stageFlags & stages) == stages)
        {
            covered = true;
            break;
        }
    }
    if (!covered)
        return DVZ_ERROR;

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    if (cmd == VK_NULL_HANDLE)
        return DVZ_ERROR;
    vkCmdPushConstants(
        cmd, slots->pipeline_layout, stages, (uint32_t)offset, (uint32_t)size, data);
    return DVZ_OK;
}



int dvz_slots_create(DvzSlots* slots)
{
    ANN(slots);
    ANN(slots->device);

    VkDevice vkd = dvz_device_handle(slots->device);
    ANNVK(vkd);

    // Descriptor set layout.

    // Go through all sets.
    for (uint32_t set = 0; set < slots->set_count; set++)
    {
        uint32_t binding_count = slots->binding_counts[set];
        ASSERT(binding_count <= DVZ_MAX_BINDINGS);

        // Create descriptor set layout.
        VkDescriptorSetLayoutCreateInfo info = {0};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = binding_count;
        info.pBindings = (const VkDescriptorSetLayoutBinding*)&slots->bindings[set];

        log_trace(
            "creating descriptor set layout for set #%d with %d bindings", set, binding_count);
        VkResult res = vkCreateDescriptorSetLayout(vkd, &info, NULL, &slots->set_layouts[set]);
        if (vk_result_check(res, __FILE__, __LINE__) != 0)
        {
            _slots_release_handles(slots);
            return 1;
        }
    }

    if (!_slots_push_ranges_valid(slots))
    {
        _slots_release_handles(slots);
        return 1;
    }

    // Pipeline layout.
    VkPipelineLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = slots->set_count;
    info.pSetLayouts = slots->set_layouts;

    // Push constants.
    info.pushConstantRangeCount = slots->push_count;
    info.pPushConstantRanges = slots->pushs;

    log_trace("creating pipeline layout...");
    VkResult res = vkCreatePipelineLayout(vkd, &info, NULL, &slots->pipeline_layout);
    if (vk_result_check(res, __FILE__, __LINE__) != 0)
    {
        _slots_release_handles(slots);
        return 1;
    }

    log_trace("pipeline layout created");
    dvz_obj_created(&slots->obj);
    return 0;
}



VkPipelineLayout dvz_slots_handle(DvzSlots* slots)
{
    ANN(slots);
    return slots->pipeline_layout;
}



VkPipelineLayout dvz_slots_combined_pipeline_layout(
    DvzDevice* device, VkDescriptorSetLayout layout0, VkDescriptorSetLayout layout1)
{
    ANN(device);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    VkDescriptorSetLayout set_layouts[2] = {layout0, layout1};
    VkPipelineLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 2;
    info.pSetLayouts = set_layouts;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult res = vkCreatePipelineLayout(vkd, &info, NULL, &layout);
    if (vk_result_check(res, __FILE__, __LINE__) != 0)
        return VK_NULL_HANDLE;
    return layout;
}



/**
 * Return the device that owns a slots wrapper.
 *
 * @param slots the slots
 * @return the owning device
 */
DvzDevice* dvz_slots_device(DvzSlots* slots)
{
    ANN(slots);
    return slots->device;
}



/**
 * Return the number of descriptor sets configured on a slots wrapper.
 *
 * @param slots the slots
 * @return the descriptor-set count
 */
uint32_t dvz_slots_set_count(DvzSlots* slots)
{
    ANN(slots);
    return slots->set_count;
}



/**
 * Return the number of bindings configured for a descriptor set.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @return the binding count for that set
 */
uint32_t dvz_slots_binding_count(DvzSlots* slots, uint32_t set)
{
    ANN(slots);
    ASSERT(set < DVZ_MAX_SETS);
    return slots->binding_counts[set];
}



/**
 * Return the number of configured push-constant ranges.
 *
 * @param slots the slots
 * @return the push-constant range count
 */
uint32_t dvz_slots_push_count(DvzSlots* slots)
{
    ANN(slots);
    return slots->push_count;
}



/**
 * Return the descriptor type configured for a slot binding.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @param binding the binding index within the set
 * @return the descriptor type
 */
VkDescriptorType dvz_slots_descriptor_type(DvzSlots* slots, uint32_t set, uint32_t binding)
{
    ANN(slots);
    ASSERT(set < DVZ_MAX_SETS);
    ASSERT(binding < DVZ_MAX_BINDINGS);
    return slots->bindings[set][binding].descriptorType;
}



/**
 * Return the descriptor-set layout handle for a set.
 *
 * @param slots the slots
 * @param set the descriptor-set index
 * @return the descriptor-set layout handle
 */
VkDescriptorSetLayout dvz_slots_set_layout(DvzSlots* slots, uint32_t set)
{
    ANN(slots);
    ASSERT(set < DVZ_MAX_SETS);
    return slots->set_layouts[set];
}



void dvz_slots_destroy(DvzSlots* slots)
{
    ANN(slots);
    if (!dvz_obj_is_created(&slots->obj))
    {
        log_trace("skip destruction of already-destroyed slots");
        return;
    }

    _slots_release_handles(slots);
    dvz_obj_destroyed(&slots->obj);
}



/**
 * Free a slots wrapper allocated by dvz_slots_create_wrapper().
 *
 * @param slots slots wrapper to free
 */
void dvz_slots_free(DvzSlots* slots)
{
    if (slots == NULL)
    {
        return;
    }
    dvz_free(slots);
}
