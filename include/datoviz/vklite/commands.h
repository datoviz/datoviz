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
#include "datoviz/common/types.h"



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
 * @param device logical device that owns the command pool and buffers; must outlive `cmds`
 * @param queue queue whose family selects the command pool and on which submissions are made;
 * must outlive `cmds`
 * @param count the number of command buffers to create
 * @param[out] cmds the created command buffers
 */
DVZ_EXPORT void
dvz_commands(DvzDevice* device, DvzQueue* queue, uint32_t count, DvzCommands* cmds);



/**
 * Allocate a single primary command buffer from the device command pool of a queue family.
 *
 * @param device logical device that owns the selected command pool
 * @param queue_family queue family index used to select the command pool
 * @return owned command buffer, or `VK_NULL_HANDLE` on failure; free it with
 * dvz_command_buffer_free()
 */
DVZ_EXPORT VkCommandBuffer dvz_command_buffer_alloc(DvzDevice* device, uint32_t queue_family);



/**
 * Free a single command buffer from the device command pool of a queue family.
 *
 * @param device logical device whose command pool owns `cmd`
 * @param queue_family queue family index used to select the command pool
 * @param cmd command buffer to free
 */
DVZ_EXPORT void
dvz_command_buffer_free(DvzDevice* device, uint32_t queue_family, VkCommandBuffer cmd);



/**
 * Return the Vulkan handle of the currently-selected command buffers.
 *
 * @param cmds the set of command buffers
 * @return borrowed handle of the currently selected command buffer
 */
DVZ_EXPORT VkCommandBuffer dvz_commands_handle(DvzCommands* cmds);



/**
 * Return the number of command buffers managed by a wrapper.
 *
 * @param cmds the set of command buffers
 * @return number of command buffers owned or wrapped by `cmds`
 */
DVZ_EXPORT uint32_t dvz_commands_count(DvzCommands* cmds);



/**
 * Set the current command buffer index.
 *
 * @param cmds the set of command buffers
 * @param current command-buffer index in `[0, dvz_commands_count(cmds))`
 */
DVZ_EXPORT void dvz_commands_current(DvzCommands* cmds, uint32_t current);



/**
 * Start recording the currently selected owned command buffer.
 *
 * This operation is rejected for an externally owned command buffer wrapped as already recording.
 *
 * @param cmds the set of command buffers
 * @return 0 on success, non-zero on Vulkan or state failure
 */
DVZ_EXPORT int dvz_cmd_begin_result(DvzCommands* cmds);



/**
 * Start recording the currently selected owned command buffer.
 *
 * This convenience wrapper logs failures from dvz_cmd_begin_result(). It must not be used with an
 * externally owned command buffer wrapped as already recording.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_begin(DvzCommands* cmds);



/**
 * Stop recording the currently selected owned command buffer.
 *
 * This operation is rejected for an externally owned command buffer wrapped as already recording.
 *
 * @param cmds the set of command buffers
 * @return 0 on success, non-zero on Vulkan or state failure
 */
DVZ_EXPORT int dvz_cmd_end_result(DvzCommands* cmds);



/**
 * Stop recording the currently selected owned command buffer.
 *
 * This convenience wrapper logs failures from dvz_cmd_end_result(). It must not be used with an
 * externally owned command buffer wrapped as already recording.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_end(DvzCommands* cmds);



/**
 * Reset the currently selected owned command buffer.
 *
 * Do not call this on a borrowed recording command buffer.
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
 * @param device logical device associated with `vk_cmd`; must outlive `cmds`
 * @param vk_cmd externally owned command buffer whose owner permits recording-control operations
 * @param[out] cmds wrapper initialized around `vk_cmd`; it does not acquire ownership of the handle
 */
DVZ_EXPORT void dvz_commands_wrap(DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds);


/**
 * Wrap an externally-owned Vulkan command buffer that is already recording.
 *
 * The returned wrapper may be passed to vklite command-recording helpers only. Calls that would
 * begin, end, reset, submit, or destroy the borrowed command buffer are rejected before touching
 * Vulkan.
 *
 * @param device logical device associated with `vk_cmd`; must outlive `cmds`
 * @param vk_cmd externally owned command buffer that is already in the recording state
 * @param[out] cmds wrapper initialized around `vk_cmd`; it does not acquire ownership of the handle
 */
DVZ_EXPORT void
dvz_commands_wrap_borrowed_recording(DvzDevice* device, VkCommandBuffer vk_cmd, DvzCommands* cmds);



/**
 * Detach a borrowed recording command buffer from a reusable wrapper.
 *
 * This operation only accepts wrappers initialized by dvz_commands_wrap_borrowed_recording(). It
 * clears all borrowed device and command-buffer references without ending, resetting, submitting,
 * freeing, or otherwise touching the Vulkan command buffer. Owned and recording-control wrappers
 * are rejected without mutation.
 *
 * @param cmds borrowed recording commands wrapper to detach
 * @returns DVZ_OK on success or DVZ_ERROR when the wrapper is null or not borrowed-recording
 */
DVZ_EXPORT DvzResult dvz_commands_unwrap(DvzCommands* cmds);



EXTERN_C_OFF
