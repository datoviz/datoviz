/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Compute internals                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "obj.h"
#include "datoviz/vklite/compute.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCompute
{
    DvzObject obj;
    DvzDevice* device;

    VkShaderModule shader;
    VkPipelineLayout layout;

    VkSpecializationMapEntry spec_entries[DVZ_MAX_SPEC_CONST];
    VkSpecializationInfo spec_info;
    unsigned char spec_data[DVZ_MAX_SPEC_CONST_SIZE];

    VkPipeline vk_pipeline;
};
