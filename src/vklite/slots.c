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

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/slots.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_slots(DvzDevice* device, DvzSlots* slots)
{
    ANN(device);
    ANN(slots);

    slots->device = device;
    dvz_obj_init(&slots->obj);
}



void dvz_slots_binding(
    DvzSlots* slots, uint32_t set, uint32_t binding, VkShaderStageFlags stages,
    VkDescriptorType type, uint32_t count)
{
    ANN(slots);
    ASSERT(set < DVZ_MAX_SETS);
    ASSERT(binding < DVZ_MAX_BINDINGS);
    slots->set_count = MAX(set + 1, slots->set_count);
    slots->binding_counts[set] = MAX(binding + 1, slots->binding_counts[set]);

    slots->bindings[set][binding].binding = binding;
    slots->bindings[set][binding].descriptorCount = count;
    slots->bindings[set][binding].stageFlags = stages;
    slots->bindings[set][binding].descriptorType = type;
}



void dvz_slots_push(
    DvzSlots* slots, VkShaderStageFlagBits stages, VkDeviceSize offset, VkDeviceSize size)
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
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vkd, &info, NULL, &slots->set_layouts[set]));
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
    VK_RETURN_RESULT(vkCreatePipelineLayout(vkd, &info, NULL, &slots->pipeline_layout));
    if (out == 0)
    {
        log_trace("pipeline layout created");
        dvz_obj_created(&slots->obj);
    }

    return out;
}



VkPipelineLayout dvz_slots_handle(DvzSlots* slots)
{
    ANN(slots);
    return slots->pipeline_layout;
}



void dvz_slots_destroy(DvzSlots* slots)
{
    ANN(slots);
    ANN(slots->device);

    VkDevice vkd = dvz_device_handle(slots->device);
    ANNVK(vkd);

    log_trace("destroying %d descriptor set layout(s)", slots->set_count);
    for (uint32_t set = 0; set < slots->set_count; set++)
    {
        vkDestroyDescriptorSetLayout(vkd, slots->set_layouts[set], NULL);
    }

    log_trace("destroying the pipeline layout...");
    vkDestroyPipelineLayout(vkd, slots->pipeline_layout, NULL);
    log_trace("pipeline layout destroyed");

    dvz_obj_destroyed(&slots->obj);
}
