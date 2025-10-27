/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <volk.h>

#include "datoviz/common/obj.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_SWAPCHAIN_IMAGES 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCommands DvzCommands;
typedef struct DvzDevice DvzDevice;
typedef struct DvzQueue DvzQueue;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCommands
{
    DvzObject obj;
    DvzDevice* device;
    DvzQueue* queue;

    uint32_t count;
    VkCommandBuffer cmds[DVZ_MAX_SWAPCHAIN_IMAGES];
    bool blocked[DVZ_MAX_SWAPCHAIN_IMAGES]; // if true, no need to refill it in the FRAME
};
