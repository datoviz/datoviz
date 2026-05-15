/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Descriptor internals                                                                         */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan_core.h>

#include "datoviz/vklite/descriptors.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDescriptors
{
    DvzSlots* slots;
    DvzDevice* device;
    VkDescriptorPool vk_pool;
    uint32_t set_count;
    VkDescriptorSet vk_descriptors[DVZ_MAX_SETS];
};
