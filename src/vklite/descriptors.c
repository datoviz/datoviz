/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Descriptors                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <volk.h>

#include "_vk_utils.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_descriptors.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/descriptors.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty descriptor wrapper.
 *
 * @return allocated descriptor wrapper, or NULL on allocation failure
 */
DvzDescriptors* dvz_descriptors_create(void)
{
    DvzDescriptors* descriptors = (DvzDescriptors*)dvz_calloc(1, sizeof(DvzDescriptors));
    ANN(descriptors);
    return descriptors;
}



void dvz_descriptors(DvzSlots* slots, DvzDescriptors* descriptors)
{
    ANN(slots);
    ANN(descriptors);

    dvz_memset(descriptors, sizeof(DvzDescriptors), 0, sizeof(DvzDescriptors));

    DvzDevice* device = dvz_slots_device(slots);
    ANN(device);

    descriptors->device = device;
    descriptors->slots = slots;
    VkDevice vkd = dvz_device_handle(device);
    VkDescriptorPool dpool = dvz_device_descriptor_pool(device);
    ANNVK(vkd);
    ANNVK(dpool);

    VkDescriptorSetAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = dpool;
    info.descriptorSetCount = dvz_slots_set_count(slots);
    ASSERT(info.descriptorSetCount <= DVZ_MAX_SETS);

    VkDescriptorSetLayout set_layouts[DVZ_MAX_SETS] = {0};
    for (uint32_t set = 0; set < info.descriptorSetCount; set++)
    {
        set_layouts[set] = dvz_slots_set_layout(slots, set);
    }
    info.pSetLayouts = set_layouts;

    log_trace("allocate descriptor sets");
    VK_CHECK_RESULT(vkAllocateDescriptorSets(vkd, &info, descriptors->vk_descriptors));
}



/**
 * Return the number of descriptor sets allocated by the wrapper.
 *
 * @param descriptors the descriptors
 * @return the descriptor set count
 */
uint32_t dvz_descriptors_set_count(DvzDescriptors* descriptors)
{
    ANN(descriptors);
    ANN(descriptors->slots);
    return dvz_slots_set_count(descriptors->slots);
}



/**
 * Return a Vulkan descriptor-set handle by set index.
 *
 * @param descriptors the descriptors
 * @param set the descriptor set index
 * @return the descriptor set handle
 */
VkDescriptorSet dvz_descriptors_handle(DvzDescriptors* descriptors, uint32_t set)
{
    ANN(descriptors);
    ASSERT(set < dvz_descriptors_set_count(descriptors));
    return descriptors->vk_descriptors[set];
}



void dvz_descriptors_buffer(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkBuffer vk_buffer, DvzSize offset, DvzSize size)
{
    ANN(descriptors);

    DvzDevice* device = descriptors->device;
    ANN(device);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);

    VkDescriptorBufferInfo buf_info = {0};
    buf_info.buffer = vk_buffer;
    buf_info.offset = offset;
    buf_info.range = size;

    VkWriteDescriptorSet dsw = {0};
    dsw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsw.descriptorType = dvz_slots_descriptor_type(slots, set, binding);
    dsw.dstSet = dvz_descriptors_handle(descriptors, set);
    dsw.dstBinding = binding;
    dsw.dstArrayElement = array_idx;
    dsw.descriptorCount = 1;
    dsw.pBufferInfo = &buf_info;

    vkUpdateDescriptorSets(vkd, 1, &dsw, 0, NULL);
}



void dvz_descriptors_image(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkImageLayout layout, VkImageView view, VkSampler sampler)
{
    ANN(descriptors);

    DvzDevice* device = descriptors->device;
    ANN(device);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);

    VkDescriptorImageInfo img_info = {0};
    img_info.imageLayout = layout;
    img_info.imageView = view;
    img_info.sampler = sampler;

    VkWriteDescriptorSet dsw = {0};
    dsw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsw.dstSet = dvz_descriptors_handle(descriptors, set);
    dsw.dstBinding = binding;
    dsw.dstArrayElement = array_idx;
    dsw.descriptorCount = 1;
    dsw.descriptorType = dvz_slots_descriptor_type(slots, set, binding);
    dsw.pImageInfo = &img_info;

    vkUpdateDescriptorSets(vkd, 1, &dsw, 0, NULL);
}



void dvz_cmd_bind_descriptors(
    DvzCommands* cmds, VkPipelineBindPoint bind_point, DvzDescriptors* descriptors,
    uint32_t first_set, uint32_t set_count, uint32_t dynamic_count, uint32_t* dynamic_idxs)
{
    ANN(cmds);
    ANN(descriptors);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);
    ASSERT(first_set + set_count <= dvz_descriptors_set_count(descriptors));

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    vkCmdBindDescriptorSets(
        cmd, bind_point, dvz_slots_handle(slots), //
        first_set, set_count, &descriptors->vk_descriptors[first_set], dynamic_count,
        dynamic_idxs);
}



/**
 * Free a descriptor wrapper allocated by dvz_descriptors_create().
 *
 * @param descriptors descriptor wrapper to free
 */
void dvz_descriptors_free(DvzDescriptors* descriptors)
{
    if (descriptors == NULL)
    {
        return;
    }
    dvz_free(descriptors);
}
