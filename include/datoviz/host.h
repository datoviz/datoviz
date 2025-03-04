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

    // Backend
    DvzBackend backend;

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
 * @param backend the backend
 * @returns a pointer to the instantiated host
 */
DvzHost* dvz_host(DvzBackend backend);



/**
 * Request an instance extension.
 *
 * @param host the host
 * @param extension_name the extension name
 */
void dvz_host_extension(DvzHost* host, const char* extension_name);



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



EXTERN_C_OFF

#endif
