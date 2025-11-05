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

#include "datoviz/vklite/descriptors.h"
#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_descriptors(DvzSlots* slots, DvzDescriptors* descriptors)
{
    ANN(slots);
    ANN(descriptors);

    DvzDevice* device = slots->device;
    ANN(device);

    descriptors->device = device;
    descriptors->slots = slots;

    VkDescriptorSetAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = device->dpool;
    info.descriptorSetCount = slots->set_count;
    info.pSetLayouts = slots->set_layouts;

    log_trace("allocate descriptor sets");
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device->vk_device, &info, descriptors->vk_descriptors));
}



void dvz_descriptors_buffer(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkBuffer vk_buffer, DvzSize offset, DvzSize size)
{
    ANN(descriptors);

    DvzDevice* device = descriptors->device;
    ANN(device);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);

    VkDescriptorBufferInfo buf_info = {0};
    buf_info.buffer = vk_buffer;
    buf_info.offset = offset;
    buf_info.range = size;

    VkWriteDescriptorSet dsw = {0};
    dsw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsw.descriptorType = slots->bindings[set][binding].descriptorType;
    dsw.dstSet = descriptors->vk_descriptors[set];
    dsw.dstBinding = binding;
    dsw.dstArrayElement = array_idx;
    dsw.descriptorCount = 1;
    dsw.pBufferInfo = &buf_info;

    vkUpdateDescriptorSets(device->vk_device, 1, &dsw, 0, NULL);
}



void dvz_descriptors_image(
    DvzDescriptors* descriptors, uint32_t set, uint32_t binding, uint32_t array_idx,
    VkImageLayout layout, VkImageView view, VkSampler sampler)
{
    ANN(descriptors);

    DvzDevice* device = descriptors->device;
    ANN(device);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);

    VkDescriptorImageInfo img_info = {0};
    img_info.imageLayout = layout;
    img_info.imageView = view;
    img_info.sampler = sampler;

    VkWriteDescriptorSet dsw = {0};
    dsw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsw.dstSet = descriptors->vk_descriptors[set];
    dsw.dstBinding = binding;
    dsw.dstArrayElement = array_idx;
    dsw.descriptorCount = 1;
    dsw.descriptorType = slots->bindings[set][binding].descriptorType;
    dsw.pImageInfo = &img_info;

    vkUpdateDescriptorSets(device->vk_device, 1, &dsw, 0, NULL);
}



void dvz_cmd_bind_descriptors(
    DvzCommands* cmds, uint32_t idx, VkPipelineBindPoint bind_point, DvzDescriptors* descriptors,
    uint32_t first_set, uint32_t set_count, uint32_t dynamic_count, uint32_t* dynamic_idxs)
{
    ANN(cmds);
    ANN(descriptors);

    DvzSlots* slots = descriptors->slots;
    ANN(slots);

    vkCmdBindDescriptorSets(
        cmds->cmds[idx], bind_point, slots->pipeline_layout, //
        first_set, set_count, &descriptors->vk_descriptors[first_set], dynamic_count,
        dynamic_idxs);
}
