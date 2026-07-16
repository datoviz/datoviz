/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpuInfo DvzGpuInfo;

EXTERN_C_ON



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGpuInfo
{
    uint32_t index;
    char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    VkPhysicalDeviceType device_type;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t vendor_id;
    uint32_t device_id;
    DvzQueueCaps queue_caps;
};



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

/**
 * Return the number of detected physical GPUs.
 *
 * @param instance the instance
 * @return the number of detected GPUs
 */
DVZ_EXPORT uint32_t dvz_instance_gpu_count(DvzInstance* instance);



/**
 * Return a GPU descriptor snapshot for a given GPU index.
 *
 * @param instance the instance
 * @param gpu_index selected GPU index in the instance
 * @param[out] out_info descriptor output
 * @return true when the descriptor was populated
 */
DVZ_EXPORT bool
dvz_instance_gpu_info(DvzInstance* instance, uint32_t gpu_index, DvzGpuInfo* out_info);



/**
 * Resolve the Vulkan physical device handle for a selected GPU index.
 *
 * @param instance the instance
 * @param gpu_index selected GPU index
 * @param[out] out_pdevice resolved Vulkan physical device
 * @return true when the physical device handle was resolved
 */
DVZ_EXPORT bool dvz_instance_gpu_handle(
    DvzInstance* instance, uint32_t gpu_index, VkPhysicalDevice* out_pdevice);



EXTERN_C_OFF
