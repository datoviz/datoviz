/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal device                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "obj.h"
#include "datoviz/vk/device.h"
#include "_gpu.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDevice
{
    DvzObject obj;
    bool is_heap_allocated;
    DvzGpu* gpu;

    DvzQueues queues;
    VkDeviceCreateInfo info;

    uint32_t req_extension_count;
    char* req_extensions[DVZ_MAX_REQ_EXTENSIONS];

    VkPhysicalDeviceFeatures2 features;
    VkPhysicalDeviceVulkan11Features features11;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesKHR present_mode_fifo_latest_ready;
#endif

    VkDevice vk_device;
    VkCommandPool cpools[DVZ_MAX_QUEUE_FAMILIES];
    VkDescriptorPool dpool;
};



/*************************************************************************************************/
/*  Internal device API                                                                          */
/*************************************************************************************************/

void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device);
void dvz_device_request_queues(DvzDevice* device, uint32_t family, uint32_t count);
int dvz_device_build(DvzDevice* device);
void dvz_device_request_canvas_extensions(DvzDevice* device);
bool dvz_device_request_extension(DvzDevice* device, const char* extension);
VkPhysicalDeviceFeatures* dvz_device_request_features10(DvzDevice* device);
VkPhysicalDeviceVulkan11Features* dvz_device_request_features11(DvzDevice* device);
VkPhysicalDeviceVulkan12Features* dvz_device_request_features12(DvzDevice* device);
VkPhysicalDeviceVulkan13Features* dvz_device_request_features13(DvzDevice* device);
