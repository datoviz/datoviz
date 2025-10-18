/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_HOST
#define DVZ_HEADER_HOST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan.h>

#include "_enums.h"
#include "_obj.h"
#include "_time_utils.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_INSTANCE_EXTENSIONS 16



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzHost DvzHost;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzHost
{
    DvzObject obj;
    uint32_t n_errors;

    uint32_t extension_count;
    const char* extensions[DVZ_MAX_INSTANCE_EXTENSIONS];

    // Global clock
    DvzClock clock;
    // bool is_running;

    // Vulkan objects.
    VkInstance instance;
    bool no_instance_destroy;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Containers.
    DvzContainer gpus;
};



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Initialize a host.
 *
 * This object represents a computer with one or multiple GPUs.
 * It holds the Vulkan instance and it is responsible for discovering the available GPUs.
 *
 * @returns a pointer to the instantiated host
 */
DvzHost* dvz_host(void);



/**
 * Request an instance extension.
 *
 * @param host the host
 * @param extension_name the extension name
 */
void dvz_host_extension(DvzHost* host, const char* extension_name);



/**
 * Add the extensions required by a backend.
 *
 * @param host the host
 * @param backend the backend
 */
void dvz_host_backend(DvzHost* host, DvzBackend backend);



/**
 * Create a host.
 *
 * This creates the Vulkan instance, unless host->instance has been manually set before (used with
 * Qt backend).
 *
 * @param host the host
 */
void dvz_host_create(DvzHost* host);



/**
 * Full synchronization on all GPUs.
 *
 * This function waits on all queues of all GPUs. The strongest, least efficient of the
 * synchronization methods.
 *
 * @param host the host
 */
void dvz_host_wait(DvzHost* host);



/**
 * Destroy the host.
 *
 * This function automatically destroys all objects created within the host.
 *
 * @param host the host to destroy
 */
int dvz_host_destroy(DvzHost* host);



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

/**
 * Initialize a GPU.
 *
 * A GPU object is the interface to one of the GPUs on the current system.
 *
 * @param host the host
 * @param idx the GPU index among the system's GPUs, or -1 to select the best one
 * @returns a pointer to the created GPU object
 */
DvzGpu* dvz_host_gpu(DvzHost* host, int32_t idx);



/**
 * Find the "best" GPU on the system.
 *
 * For now, this is just the discrete GPU with the most VRAM, or the GPU with the most VRAM if
 * there are no discrete GPUs.
 *
 * @param host the host
 * @returns a pointer to the best GPU object
 */
DvzGpu* dvz_gpu_best(DvzHost* host);



/**
 * Add a new Vulkan device extension.
 *
 * @param gpu the GPU
 * @param extension_name the extension name
 */
void dvz_gpu_extension(DvzGpu* gpu, const char* extension_name);



/**
 * Specify the export memory handle types.
 *
 * @param gpu the GPU
 * @param flags the flags
 */
void dvz_gpu_external(DvzGpu* gpu, VkExternalMemoryHandleTypeFlagsKHR flags);



/**
 * Make a renderpass for a GPU.
 *
 * Typically, the image layout is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for offscreen rendering,
 * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for desktop applications.
 *
 * @param gpu the GPU
 * @param clear_color the clear color
 * @param layout the Vulkan image layout
 * @returns a renderpass structure
 */
DvzRenderpass dvz_gpu_renderpass(DvzGpu* gpu, cvec4 clear_color, VkImageLayout layout);



/**
 * Request some features before creating the GPU instance.
 *
 * This function needs to be called before creating the GPU with ` dvz_gpu_create()`.
 *
 * @param gpu the GPU
 * @param requested_features the list of requested features
 */
void dvz_gpu_request_features(DvzGpu* gpu, VkPhysicalDeviceFeatures requested_features);



/**
 * Request a new Vulkan queue before creating the GPU.
 *
 * @param gpu the GPU
 * @param idx the queue index (should be regularly increasing: 0, 1, 2...)
 * @param type the queue type
 */
void dvz_gpu_queue(DvzGpu* gpu, uint32_t idx, DvzQueueType type);



/**
 * Create a GPU once the features and queues have been set up.
 *
 * @param gpu the GPU
 * @param surface the surface on which the GPU will need to render
 */
void dvz_gpu_create(DvzGpu* gpu, VkSurfaceKHR surface);



/**
 * Wait for a queue to be idle.
 *
 * This is one of the different GPU synchronization methods. It is not efficient as it waits until
 * the queue is idle.
 *
 * @param gpu the GPU
 * @param queue_idx the queue index
 */
void dvz_queue_wait(DvzGpu* gpu, uint32_t queue_idx);



/**
 * Full synchronization on a given GPU.
 *
 * This function waits on all queues of a given GPU.
 *
 * @param gpu the GPU
 */
void dvz_gpu_wait(DvzGpu* gpu);



/**
 * Destroy the resources associated to a GPU.
 *
 * @param gpu the GPU
 */
void dvz_gpu_destroy(DvzGpu* gpu);



EXTERN_C_OFF

#endif
