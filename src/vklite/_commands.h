/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Commands internals                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/obj.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCommands
{
    DvzObject obj;
    DvzDevice* device;
    DvzQueue* queue;

    uint32_t count;
    uint32_t current;
    VkCommandBuffer cmds[DVZ_MAX_SWAPCHAIN_IMAGES];
    bool blocked[DVZ_MAX_SWAPCHAIN_IMAGES];
};
