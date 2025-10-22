/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan validation                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"

#include <vulkan/vulkan.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Ignore some validation warnings.
static const char* DVZ_VALIDATION_IGNORES[] = {

    // HACK: hide harmless warning message on Ubuntu:
    // validation layer: /usr/lib/i386-linux-gnu/libvulkan_radeon.so: wrong ELF class: ELFCLASS32
    "ELFCLASS32",

    "BestPractices-vkBindMemory-small-dedicated-allocation",
    "BestPractices-vkAllocateMemory-small-allocation",
    "BestPractices-vkCreateCommandPool-command-buffer-reset",
    "BestPractices-vkCreateInstance-specialuse-extension",

    // prevent unnecessary error messages when quickly resizing a window (race condition, fix to be
    // done probably in the validation layers)
    // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/624
    "VUID-VkSwapchainCreateInfoKHR-imageExtent",

    // https://github.com/datoviz/datoviz/issues/17#issuecomment-849213008
    // https://www.gitmemory.com/issue/KhronosGroup/Vulkan-ValidationLayers/2729/824406355
    // "invalid layer manifest file",

    "but only supports loader interface version 4",

    "VkImageMemoryBarrier is being submitted with oldLayout VK_IMAGE_LAYOUT_UNDEFINED",

    // HACK: see https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/7348
    "vkQueueSubmit():  Hazard WRITE_AFTER_READ for entry 0, VkCommandBuffer",
};
static const VkValidationFeatureEnableEXT DVZ_VALIDATION_FEATURES[] = {
    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};

static const VkDebugUtilsMessageSeverityFlagsEXT DVZ_VALIDATION_SEVERITY =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

static const VkDebugUtilsMessageTypeFlagsEXT DVZ_VALIDATION_MESSAGE_TYPES =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static inline int _log_level(VkDebugUtilsMessageSeverityFlagBitsEXT sev)
{
    switch (sev)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return LOG_TRACE;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return LOG_DEBUG;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return LOG_WARN;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return LOG_ERROR;
        break;
    default:
        break;
    }
    return LOG_ERROR;
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, //
    void* pUserData) // NOTE: this needs to be a pointer to uint32_t n_errors
{
    // Hide long list of extensions.
    if (strstr(pCallbackData->pMessage, "Extension: VK_") != NULL)
        return VK_FALSE;

    int level = _log_level(messageSeverity);

    // NOTE: force TRACE level if ignored message.
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(DVZ_VALIDATION_IGNORES); i++)
    {
        if (strstr(pCallbackData->pMessage, DVZ_VALIDATION_IGNORES[i]) != NULL)
        {
            level = LOG_TRACE;
            break;
        }
    }

    log_log(level, __FILENAME__, __LINE__, "validation layer: %s", pCallbackData->pMessage);
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT && pUserData != NULL)
    {
        uint32_t* n_errors = (uint32_t*)pUserData;
        if (n_errors != NULL)
        {
            (*n_errors)++;
        }
    }
    return VK_FALSE;
}



static void _fill_info_debug(VkDebugUtilsMessengerCreateInfoEXT* info_debug)
{
    ANN(info_debug);
    info_debug->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info_debug->flags = 0;
    info_debug->messageSeverity = DVZ_VALIDATION_SEVERITY;
    info_debug->messageType = DVZ_VALIDATION_MESSAGE_TYPES;
    info_debug->pfnUserCallback = debug_callback;
}



static void _fill_validation_features(VkValidationFeaturesEXT* features)
{
    ANN(features);
    features->sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features->enabledValidationFeatureCount = DVZ_ARRAY_COUNT(DVZ_VALIDATION_FEATURES);
    features->pEnabledValidationFeatures = DVZ_VALIDATION_FEATURES;
}
