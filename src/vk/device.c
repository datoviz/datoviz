/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "_assertions.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "macros.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)
{
    ANN(gpu);
    ANN(device);

    /*
    recreate the underlying vulkan logical device if called multiple times (destroying the old one
    first), which allows to change the queues before
    portability : if gpu supported extensions
    include VK_KHR_portability_subset, must be included in the requested extensions

*/
    dvz_obj_init(&device->obj);
}



void dvz_device_request_queue(DvzDevice* device, uint32_t family, uint32_t count) { ANN(device); }



void dvz_device_request_queues(DvzDevice* device, DvzQueues* queues) { ANN(device); }



int dvz_device_create(DvzDevice* device)
{
    ANN(device);
    dvz_obj_created(&device->obj);
    return 0;
}



DvzQueue* dvz_device_queue_from_idx(DvzDevice* device, uint32_t family, uint32_t idx)
{
    ANN(device);
    // loop over queues and return the first that matches for family and idx
    return NULL;
}



DvzQueue* dvz_device_queue_for_role(DvzDevice* device, DvzQueueRole role)
{
    ANN(device);
    // return device->queues_for_roles[(uint32_t)role];
    return NULL;
}



void dvz_device_destroy(DvzDevice* device)
{
    ANN(device);
    dvz_obj_destroyed(&device->obj);
}
