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
#include "datoviz/vk/vulkan.h"

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
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty commands wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_commands(), record or wrap existing command buffers,
 * destroy when done, and free only if this wrapper came from
 * dvz_commands_create_wrapper().
 *
 * @return allocated commands wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzCommands* dvz_commands_create_wrapper(void);



/**
 * Free a commands wrapper allocated by dvz_commands_create_wrapper().
 *
 * @param cmds commands wrapper to free
 */
DVZ_EXPORT void dvz_commands_free(DvzCommands* cmds);



/**
 * Create a set of command buffers.
 *
 * The status is INIT when the command buffers are initialized, and CREATED when they are filled.
 * Reinitializing a live wrapper requires dvz_commands_destroy() first.
 *
 * @param device the device
 * @param queue the queue
 * @param count the number of command buffers to create
 * @param[out] cmds the created command buffers
 */
DVZ_EXPORT void
dvz_commands(DvzDevice* device, DvzQueue* queue, uint32_t count, DvzCommands* cmds);



/**
 * Allocate a single primary command buffer from the device command pool of a queue family.
 *
 * @param device the device
 * @param queue_family queue family index used to select the command pool
 * @returns the allocated command buffer, or VK_NULL_HANDLE on failure
 */
DVZ_EXPORT VkCommandBuffer dvz_command_buffer_alloc(DvzDevice* device, uint32_t queue_family);



/**
 * Free a single command buffer from the device command pool of a queue family.
 *
 * @param device the device
 * @param queue_family queue family index used to select the command pool
 * @param cmd command buffer to free
 */
DVZ_EXPORT void
dvz_command_buffer_free(DvzDevice* device, uint32_t queue_family, VkCommandBuffer cmd);



/**
 * Return the Vulkan handle of the currently-selected command buffers.
 *
 * @param cmds the set of command buffers
 * @returns the command buffer Vulkan handle
 */
DVZ_EXPORT VkCommandBuffer dvz_commands_handle(DvzCommands* cmds);



/**
 * Return the number of command buffers managed by a wrapper.
 *
 * @param cmds the set of command buffers
 * @returns the command-buffer count
 */
DVZ_EXPORT uint32_t dvz_commands_count(DvzCommands* cmds);



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
 * @return 0 on success, non-zero on Vulkan or state failure
 */
DVZ_EXPORT int dvz_cmd_begin_result(DvzCommands* cmds);



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
 * @return 0 on success, non-zero on Vulkan or state failure
 */
DVZ_EXPORT int dvz_cmd_end_result(DvzCommands* cmds);



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
 * Release the command buffers back to the Vulkan command pool.
 *
 * The wrapper itself remains alive; call dvz_commands_destroy() to also
 * tear down the underlying pool and dvz_commands_free() to reclaim the heap
 * wrapper allocation.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_release(DvzCommands* cmds);



/**
 * Submit a command buffer on its queue.
 *
 * This function blocks the queue so it is not optimal.
 *
 * @param cmds the set of command buffers
 * @return 0 on success, non-zero on Vulkan or state failure
 */
DVZ_EXPORT int dvz_cmd_submit_result(DvzCommands* cmds);



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
 * This releases the wrapped Vulkan command buffers and returns the wrapper to
 * a reusable initialized state.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_commands_destroy(DvzCommands* cmds);



/**
 * Wrap an existing Vulkan command buffer in a DvzCommands struct.
 *
 * The wrapped command buffer remains externally owned. This helper is intended for command buffers
 * whose owner grants recording-control operations such as begin, end, or reset. Queue submission is
 * not supported for wrappers created by this function because no queue is supplied.
 *
 * @param device the device
 * @param vk_cmd the Vulkan command buffer
 * @param[out] cmds the created command buffers
 */
DVZ_EXPORT void dvz_commands_wrap(DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds);


/**
 * Wrap an externally-owned Vulkan command buffer that is already recording.
 *
 * The returned wrapper may be passed to vklite command-recording helpers only. Calls that would
 * begin, end, reset, submit, or destroy the borrowed command buffer are rejected before touching
 * Vulkan.
 *
 * @param device the device
 * @param vk_cmd the borrowed recording Vulkan command buffer
 * @param[out] cmds the created command buffers
 */
DVZ_EXPORT void
dvz_commands_wrap_borrowed_recording(DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds);



EXTERN_C_OFF
