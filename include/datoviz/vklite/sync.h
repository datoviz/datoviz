/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Rendering                                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/commands.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzCommands DvzCommands;

typedef struct VkMemoryBarrier2 DvzBarrierMemory;
typedef struct VkBufferMemoryBarrier2 DvzBarrierBuffer;
typedef struct VkImageMemoryBarrier2 DvzBarrierImage;
typedef struct DvzBarriers DvzBarriers;
typedef struct DvzFence DvzFence;
typedef struct DvzSemaphore DvzSemaphore;
typedef struct DvzSubmit DvzSubmit;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BARRIERS   4
#define DVZ_MAX_SEMAPHORES 4
#define DVZ_MAX_COMMANDS   4



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/**
 * Mutable command-recording helper for vkCmdPipelineBarrier2 state.
 *
 * DvzBarriers is intentionally public and non-opaque: callers build barrier state incrementally,
 * inspect counts, and pass the recorded dependency info to command-encoding helpers. This type is
 * treated as a builder/config object rather than an owned resource wrapper.
 */
struct DvzBarriers
{
    VkDependencyInfo info;
    DvzBarrierMemory bmems[DVZ_MAX_BARRIERS];
    DvzBarrierBuffer bbufs[DVZ_MAX_BARRIERS];
    DvzBarrierImage bimg[DVZ_MAX_BARRIERS];
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Wrapper allocation                                                                           */
/*************************************************************************************************/

/**
 * Allocate an empty fence wrapper.
 *
 * @return allocated fence wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzFence* dvz_fence_create_wrapper(void);



/**
 * Free a fence wrapper allocated by dvz_fence_create_wrapper().
 *
 * @param fence fence wrapper to free
 */
DVZ_EXPORT void dvz_fence_free(DvzFence* fence);



/**
 * Allocate an empty semaphore wrapper.
 *
 * @return allocated semaphore wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSemaphore* dvz_semaphore_create_wrapper(void);



/**
 * Free a semaphore wrapper allocated by dvz_semaphore_create_wrapper().
 *
 * @param semaphore semaphore wrapper to free
 */
DVZ_EXPORT void dvz_semaphore_free(DvzSemaphore* semaphore);



/**
 * Allocate an empty submit wrapper.
 *
 * @return allocated submit wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSubmit* dvz_submit_create_wrapper(void);



/**
 * Free a submit wrapper allocated by dvz_submit_create_wrapper().
 *
 * @param submit submit wrapper to free
 */
DVZ_EXPORT void dvz_submit_free(DvzSubmit* submit);

/*************************************************************************************************/
/*  Memory barrier                                                                               */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bmem the memory barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_memory_stage(
    DvzBarrierMemory* bmem, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bmem the memory barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void
dvz_barrier_memory_access(DvzBarrierMemory* bmem, VkAccessFlags2 src, VkAccessFlags2 dst);



/*************************************************************************************************/
/*  Buffer barrier                                                                               */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bbuf the buffer barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_buffer_stage( //
    DvzBarrierBuffer* bbuf, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bbuf the buffer barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void dvz_barrier_buffer_access( //
    DvzBarrierBuffer* bbuf, VkAccessFlags2 src, VkAccessFlags2 dst);



/**
 * Set the queue family transfer.
 *
 * @param bbuf the buffer barrier
 * @param src the source queue family index
 * @param dst the destination queue family index
 */
DVZ_EXPORT void dvz_barrier_buffer_queue( //
    DvzBarrierBuffer* bbuf, uint32_t src, uint32_t dst);



/*************************************************************************************************/
/*  Image barrier                                                                                */
/*************************************************************************************************/

/**
 * Set the barrier stages.
 *
 * @param bimg the image barrier
 * @param src the source stages
 * @param dst the destination stages
 */
DVZ_EXPORT void dvz_barrier_image_stage( //
    DvzBarrierImage* bimg, VkPipelineStageFlags2 src, VkPipelineStageFlags2 dst);



/**
 * Set the barrier access.
 *
 * @param bimg the image barrier
 * @param src the source access
 * @param dst the destination access
 */
DVZ_EXPORT void dvz_barrier_image_access( //
    DvzBarrierImage* bimg, VkAccessFlags2 src, VkAccessFlags2 dst);



/**
 * Set the image layout change.
 *
 * @param bimg the image barrier
 * @param old the old image layout
 * @param new_layout the new image layout
 */
DVZ_EXPORT void dvz_barrier_image_layout( //
    DvzBarrierImage* bimg, VkImageLayout old, VkImageLayout new_layout);



/**
 * Set the queue family transfer.
 *
 * @param bimg the image barrier
 * @param src the source queue family index
 * @param dst the destination queue family index
 */
DVZ_EXPORT void dvz_barrier_image_queue( //
    DvzBarrierImage* bimg, uint32_t src, uint32_t dst);



/**
 * Set the barrier image aspect flags.
 *
 * @param bimg the image barrier
 * @param aspect the image aspect flags
 */
DVZ_EXPORT void dvz_barrier_image_aspect( //
    DvzBarrierImage* bimg, VkImageAspectFlags aspect);



/**
 * Set the MIP images of an image barrier.
 *
 * @param bimg the image barrier
 * @param base the base index
 * @param count the number of MIP images
 */
DVZ_EXPORT void dvz_barrier_image_mip( //
    DvzBarrierImage* bimg, uint32_t base, uint32_t count);



/**
 * Set the array layers of an image barrier.
 *
 * @param bimg the image barrier
 * @param base the base array layer index
 * @param count the number of array layers
 */
DVZ_EXPORT void dvz_barrier_image_layers( //
    DvzBarrierImage* bimg, uint32_t base, uint32_t count);



/*************************************************************************************************/
/*  Barriers                                                                                     */
/*************************************************************************************************/

/**
 * Create a set of barriers.
 *
 * @param barriers the set of barriers
 */
DVZ_EXPORT void dvz_barriers(DvzBarriers* barriers);



/**
 * Set the dependency flags of a set of barriers
 *
 * @param barriers the set of barriers
 * @param flags the dependency flags
 */
DVZ_EXPORT void dvz_barriers_flags(DvzBarriers* barriers, VkDependencyFlags flags);



/**
 * Add a memory barrier to a set of barriers
 *
 * Each barrier kind has capacity `DVZ_MAX_BARRIERS`. Returns NULL when the memory-barrier capacity
 * is exhausted.
 *
 * @param barriers the set of barriers
 * @return mutable barrier owned by `barriers`, or NULL when capacity is exhausted
 */
DVZ_EXPORT DvzBarrierMemory* dvz_barriers_memory(DvzBarriers* barriers);



/**
 * Add a buffer barrier to a set of barriers
 *
 * Each barrier kind has capacity `DVZ_MAX_BARRIERS`. Returns NULL when the buffer-barrier capacity
 * is exhausted.
 *
 * @param barriers the set of barriers
 * @param buffer buffer handle whose accesses are synchronized
 * @param offset byte offset of the synchronized range within `buffer`
 * @param size size of the synchronized range in bytes, or `VK_WHOLE_SIZE`
 * @return mutable barrier owned by `barriers`, or NULL when capacity is exhausted
 */
DVZ_EXPORT DvzBarrierBuffer* dvz_barriers_buffer(
    DvzBarriers* barriers, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);



/**
 * Add an image barrier to a set of barriers
 *
 * Each barrier kind has capacity `DVZ_MAX_BARRIERS`. Returns NULL when the image-barrier capacity
 * is exhausted.
 *
 * @param barriers the set of barriers
 * @param img live image handle whose known layout and access state will be transitioned
 * @return mutable barrier owned by `barriers`, or NULL when capacity is exhausted
 */
DVZ_EXPORT DvzBarrierImage* dvz_barriers_image(DvzBarriers* barriers, VkImage img);



/**
 * Record a set of barriers in a command buffer.
 *
 * @param cmds the command buffers
 * @param barriers the set of barriers
 */
DVZ_EXPORT void dvz_cmd_barriers(DvzCommands* cmds, DvzBarriers* barriers);



/**
 * Return the number of recorded memory barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return memory-barrier count
 */
DVZ_EXPORT uint32_t dvz_barriers_memory_count(DvzBarriers* barriers);



/**
 * Return the number of recorded buffer barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return buffer-barrier count
 */
DVZ_EXPORT uint32_t dvz_barriers_buffer_count(DvzBarriers* barriers);



/**
 * Return the number of recorded image barriers in a barrier set.
 *
 * @param barriers the barrier set
 * @return image-barrier count
 */
DVZ_EXPORT uint32_t dvz_barriers_image_count(DvzBarriers* barriers);



/**
 * Return the dependency flags configured on a barrier set.
 *
 * @param barriers the barrier set
 * @return dependency flags
 */
DVZ_EXPORT VkDependencyFlags dvz_barriers_dependency_flags(DvzBarriers* barriers);



/**
 * Return the maximum number of barriers supported per barrier type.
 *
 * @param barriers the barrier set
 * @return barrier capacity per barrier type
 */
DVZ_EXPORT uint32_t dvz_barriers_capacity(DvzBarriers* barriers);



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

/**
 * Initialize a fence (CPU-GPU synchronization).
 *
 * @param device logical device that owns the fence; must outlive `fence`
 * @param signaled whether the fence is created in the signaled state or not
 * @param[out] fence the created fence
 */
DVZ_EXPORT void dvz_fence(DvzDevice* device, bool signaled, DvzFence* fence);



/**
 * Wait on the GPU until a fence is signaled.
 *
 * @param fence the fence
 * @return whether the fence was signaled successfully
 */
DVZ_EXPORT bool dvz_fence_wait(DvzFence* fence);



/**
 * Return the Vulkan fence handle owned by a fence wrapper.
 *
 * @param fence the fence
 * @return borrowed Vulkan fence handle, or `VK_NULL_HANDLE` when not initialized
 */
DVZ_EXPORT VkFence dvz_fence_handle(DvzFence* fence);



/**
 * Return whether a fence is ready.
 *
 * @param fence the fence
 * @return true if the fence is signaled, false if it is unsignaled or the query failed
 */
DVZ_EXPORT bool dvz_fence_ready(DvzFence* fence);



/**
 * Reset a fence to the unsignaled state.
 *
 * @param fence the fence
 */
DVZ_EXPORT void dvz_fence_reset(DvzFence* fence);



/**
 * Destroy fence.
 *
 * @param fence the fence
 */
DVZ_EXPORT void dvz_fence_destroy(DvzFence* fence);



/*************************************************************************************************/
/*  Semaphore                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a semaphore (GPU-GPU synchronization).
 *
 * @param device logical device that owns the semaphore; must outlive `semaphore`
 * @param[out] semaphore initialized binary semaphore wrapper
 */
DVZ_EXPORT void dvz_semaphore(DvzDevice* device, DvzSemaphore* semaphore);



/**
 * Initialize a timeline semaphore (GPU-GPU synchronization).
 *
 * @param device logical device that owns the semaphore; must outlive `semaphore`
 * @param value the initial value
 * @param[out] semaphore initialized timeline semaphore wrapper
 * @param handle_type external semaphore handle type required for export (0 when unused)
 */
DVZ_EXPORT void dvz_semaphore_timeline(
    DvzDevice* device, uint64_t value, DvzSemaphore* semaphore,
    VkExternalSemaphoreHandleTypeFlags handle_type);



/**
 * Signal a timeline semaphore from the CPU.
 *
 * @param semaphore live timeline semaphore to signal
 * @param value monotonically increasing timeline value to signal
 */
DVZ_EXPORT void dvz_semaphore_signal(DvzSemaphore* semaphore, uint64_t value);



/**
 * Wait a timeline semaphore on the CPU.
 *
 * @param semaphore live timeline semaphore to wait on
 * @param value timeline value that must be reached
 */
DVZ_EXPORT void dvz_semaphore_wait(DvzSemaphore* semaphore, uint64_t value);



/**
 * Retrieve the current value of a timeline semaphore.
 *
 * @param semaphore live timeline semaphore to query
 * @return current timeline value, or 0 if the query fails
 */
DVZ_EXPORT uint64_t dvz_semaphore_query(DvzSemaphore* semaphore);



/**
 * Return the Vulkan semaphore handle owned by a semaphore wrapper.
 *
 * @param semaphore the semaphore
 * @return borrowed Vulkan semaphore handle, or `VK_NULL_HANDLE` when not initialized
 */
DVZ_EXPORT VkSemaphore dvz_semaphore_handle(DvzSemaphore* semaphore);



/**
 * Export a semaphore as a Unix file descriptor.
 *
 * @param semaphore semaphore to export
 * @param handle_type external handle type requested by the caller
 * @return owned file descriptor on success, -1 on failure or unsupported platforms; the caller
 * must close a successful descriptor
 */
DVZ_EXPORT int
dvz_semaphore_export_fd(DvzSemaphore* semaphore, VkExternalSemaphoreHandleTypeFlags handle_type);



/**
 * Export a semaphore as a native platform handle.
 *
 * @param semaphore semaphore to export
 * @param handle_type external handle type requested by the caller
 * @return native handle on success, `DVZ_EXTERNAL_HANDLE_INVALID` on failure
 */
DVZ_EXPORT DvzExternalHandle
dvz_semaphore_export(DvzSemaphore* semaphore, VkExternalSemaphoreHandleTypeFlags handle_type);



/**
 * Destroy semaphore.
 *
 * @param semaphore the semaphore
 */
DVZ_EXPORT void dvz_semaphore_destroy(DvzSemaphore* semaphore);



/*************************************************************************************************/
/*  Submission                                                                                   */
/*************************************************************************************************/


/**
 * Initialize or reset a submission.
 *
 * @param submit the submission
 */
DVZ_EXPORT void dvz_submit(DvzSubmit* submit);



/**
 * Add a semaphore to wait on.
 *
 * A submission stores at most `DVZ_MAX_SEMAPHORES` wait semaphores. Extra waits are ignored after
 * logging an error.
 *
 * @param submit the submission
 * @param semaphore the semaphore
 * @param value the value to wait on, if using a timeline semaphore
 * @param stage destination pipeline stage mask that waits for the semaphore
 */
DVZ_EXPORT void dvz_submit_wait(
    DvzSubmit* submit, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);



/**
 * Add a semaphore to signal.
 *
 * A submission stores at most `DVZ_MAX_SEMAPHORES` signal semaphores. Extra signals are ignored
 * after logging an error.
 *
 * @param submit the submission
 * @param semaphore the semaphore
 * @param value the value to signal, if using a timeline semaphore
 * @param stage pipeline stage mask associated with the signal operation
 */
DVZ_EXPORT void dvz_submit_signal(
    DvzSubmit* submit, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);



/**
 * Add a command buffer to the submission.
 *
 * A submission stores at most `DVZ_MAX_COMMANDS` command buffers. Extra command buffers are ignored
 * after logging an error.
 *
 * @param submit the submission
 * @param cmd the command buffer
 */
DVZ_EXPORT void dvz_submit_command(DvzSubmit* submit, VkCommandBuffer cmd);



/**
 * Return the number of wait semaphores configured on a submission.
 *
 * @param submit the submission
 * @return wait-semaphore count
 */
DVZ_EXPORT uint32_t dvz_submit_wait_count(DvzSubmit* submit);



/**
 * Return the number of signal semaphores configured on a submission.
 *
 * @param submit the submission
 * @return signal-semaphore count
 */
DVZ_EXPORT uint32_t dvz_submit_signal_count(DvzSubmit* submit);



/**
 * Return the number of command buffers configured on a submission.
 *
 * @param submit the submission
 * @return command-buffer count
 */
DVZ_EXPORT uint32_t dvz_submit_command_count(DvzSubmit* submit);



/**
 * Return whether a submission has no recorded waits, signals, or command buffers.
 *
 * @param submit the submission
 * @return true when the submission is empty
 */
DVZ_EXPORT bool dvz_submit_is_empty(DvzSubmit* submit);



/**
 * Send a submission to a queue.
 *
 * @param submit the submission
 * @param queue borrowed Vulkan queue on which to submit
 * @param fence optional borrowed fence signaled after completion, or `VK_NULL_HANDLE`
 * @return Vulkan result code cast to int32_t (VK_SUCCESS on success)
 */
DVZ_EXPORT int32_t dvz_submit_send(DvzSubmit* submit, VkQueue queue, VkFence fence);



EXTERN_C_OFF
