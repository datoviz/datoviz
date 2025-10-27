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

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCommands DvzCommands;
typedef struct DvzDevice DvzDevice;
typedef struct DvzQueue DvzQueue;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



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
void dvz_commands(DvzDevice* device, DvzQueue* queue, uint32_t count, DvzCommands* cmds);



/**
 * Start recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to begin recording on
 */
DVZ_EXPORT void dvz_cmd_begin(DvzCommands* cmds, uint32_t idx);



/**
 * Stop recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to stop the recording on
 */
DVZ_EXPORT void dvz_cmd_end(DvzCommands* cmds, uint32_t idx);



/**
 * Reset a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to reset
 */
DVZ_EXPORT void dvz_cmd_reset(DvzCommands* cmds, uint32_t idx);



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
 * @param idx the index of the command buffer to submit
 */
DVZ_EXPORT void dvz_cmd_submit(DvzCommands* cmds, uint32_t idx);



/**
 * Destroy a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_commands_destroy(DvzCommands* cmds);



EXTERN_C_OFF
