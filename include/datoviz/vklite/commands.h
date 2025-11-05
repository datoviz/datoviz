/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
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
    uint32_t current;
    VkCommandBuffer cmds[DVZ_MAX_SWAPCHAIN_IMAGES];
    bool blocked[DVZ_MAX_SWAPCHAIN_IMAGES]; // if true, no need to refill it in the FRAME
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Create a set of command buffers.
 *
 * The status is INIT when the command buffers are initialized, and CREATED when they are filled.
 *
 * !!! note
 *     We use the following convention in vklite and elsewhere in datoviz: the queue #0 is the MAIN
 *     queue with support for graphics, compute, and transfers.
 *
 * @param device the device
 * @param queue the queue
 * @param count the number of command buffers to create
 * @param[out] cmds the create command buffers
 */
DVZ_EXPORT void
dvz_commands(DvzDevice* device, DvzQueue* queue, uint32_t count, DvzCommands* cmds);



/**
 * Return the Vulkan handle of the currently-selected command buffers.
 *
 * @param cmds the set of command buffers
 * @returns the command buffer Vulkan handle
 */
DVZ_EXPORT VkCommandBuffer dvz_commands_handle(DvzCommands* cmds);



/**
 * Set the current command buffer index.
 *
 * @param cmds the set of command buffers
 * @param current the current command buffer index
 */
DVZ_EXPORT void dvz_commands_current(DvzCommands* cmds, uint32_t current);



/**
 * Start recording a command buffer.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_begin(DvzCommands* cmds);



/**
 * Stop recording a command buffer.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_end(DvzCommands* cmds);



/**
 * Reset a command buffer.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_reset(DvzCommands* cmds);



/**
 * Free a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_free(DvzCommands* cmds);



/**
 * Submit a command buffer on its queue.
 *
 * This function blocks the queue so it is not optimal.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_submit(DvzCommands* cmds);



/**
 * Destroy a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_commands_destroy(DvzCommands* cmds);



EXTERN_C_OFF
