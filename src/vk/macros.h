/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan macros                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz/common/macros.h"

#include <vulkan/vulkan.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define STR(r)                                                                                    \
    case VK_##r:                                                                                  \
        str = #r;                                                                                 \
        break
#define noop

static inline int check_result(VkResult res)
{
    char* str = "UNKNOWN_ERROR";
    switch (res)
    {
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
    default:
        noop;
    }
    if (res != VK_SUCCESS)
    {
        log_error("VkResult is %s in %s at line %d", str, __FILE__, __LINE__);
        return 1;
    }
    return 0;
}

#define VK_CHECK_RESULT(f)                                                                        \
    {                                                                                             \
        VkResult res = (f);                                                                       \
        check_result(res);                                                                        \
    }
