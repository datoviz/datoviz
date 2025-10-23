/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/obj.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_LAYERS     128
#define DVZ_MAX_EXTENSIONS 128



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInstance
{
    DvzObject obj;

    VkInstance vk_instance;
    uint32_t vk_version;

    // Instance creation structures.
    bool flags;
    VkInstanceCreateInfo info_inst;
    VkDebugUtilsMessengerCreateInfoEXT info_debug;
    VkValidationFeaturesEXT validation_features;

    uint32_t layer_count;
    char* layers[DVZ_MAX_LAYERS];

    uint32_t ext_count;
    char* extensions[DVZ_MAX_EXTENSIONS];

    char* name;
    uint32_t version;

    // Validation.
    VkDebugUtilsMessengerEXT debug_messenger;
    uint32_t n_errors;
};



struct DvzGpu
{
    VkPhysicalDevice pdevice;

    VkPhysicalDeviceProperties2 props;
    VkPhysicalDeviceVulkan11Properties props11;
    VkPhysicalDeviceVulkan12Properties props12;
    VkPhysicalDeviceVulkan13Properties props13;

    VkPhysicalDeviceMemoryProperties memprops;

    VkPhysicalDevice features;

    uint32_t ext_count;
    char* extensions[DVZ_MAX_EXTENSIONS];
};
