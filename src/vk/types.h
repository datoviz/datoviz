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
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/obj.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>



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
    char** layers;

    uint32_t ext_count;
    char** extensions;

    char* name;
    uint32_t version;

    // Validation.
    VkDebugUtilsMessengerEXT debug_messenger;
    uint32_t n_errors;
};
