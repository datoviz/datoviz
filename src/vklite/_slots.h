/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Slots internals                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/obj.h"
#include "datoviz/vklite/slots.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSlots
{
    DvzObject obj;
    DvzDevice* device;

    uint32_t set_count;
    uint32_t binding_counts[DVZ_MAX_SETS];
    VkDescriptorSetLayoutBinding bindings[DVZ_MAX_SETS][DVZ_MAX_BINDINGS];

    uint32_t push_count;
    VkPushConstantRange pushs[DVZ_MAX_PUSH_CONSTANTS];

    VkDescriptorSetLayout set_layouts[DVZ_MAX_BINDINGS];

    VkPipelineLayout pipeline_layout;
};
