/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sync internals                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/obj.h"
#include "datoviz/vklite/sync.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFence
{
    DvzObject obj;
    DvzDevice* device;
    VkFence vk_fence;
};



struct DvzSemaphore
{
    DvzObject obj;
    DvzDevice* device;
    VkSemaphore vk_semaphore;
    uint64_t value;
};



struct DvzSubmit
{
    DvzDevice* device;
    VkSubmitInfo2 info;
    VkSemaphoreSubmitInfo wait[DVZ_MAX_SEMAPHORES];
    VkSemaphoreSubmitInfo signal[DVZ_MAX_SEMAPHORES];
    VkCommandBufferSubmitInfo cmds[DVZ_MAX_COMMANDS];
};
