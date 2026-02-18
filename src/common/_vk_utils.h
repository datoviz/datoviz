/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vulkan utils                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_log.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_VK_STR(r)                                                                             \
    case VK_##r:                                                                                  \
        str = #r;                                                                                 \
        break



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static inline int vk_result_check(VkResult res, const char* file, int line)
{
    const char* str = "UNKNOWN_ERROR";
    switch (res)
    {
        DVZ_VK_STR(NOT_READY);
        DVZ_VK_STR(TIMEOUT);
        DVZ_VK_STR(EVENT_SET);
        DVZ_VK_STR(EVENT_RESET);
        DVZ_VK_STR(INCOMPLETE);
        DVZ_VK_STR(ERROR_OUT_OF_HOST_MEMORY);
        DVZ_VK_STR(ERROR_OUT_OF_DEVICE_MEMORY);
        DVZ_VK_STR(ERROR_INITIALIZATION_FAILED);
        DVZ_VK_STR(ERROR_DEVICE_LOST);
        DVZ_VK_STR(ERROR_MEMORY_MAP_FAILED);
        DVZ_VK_STR(ERROR_LAYER_NOT_PRESENT);
        DVZ_VK_STR(ERROR_EXTENSION_NOT_PRESENT);
        DVZ_VK_STR(ERROR_FEATURE_NOT_PRESENT);
        DVZ_VK_STR(ERROR_INCOMPATIBLE_DRIVER);
        DVZ_VK_STR(ERROR_TOO_MANY_OBJECTS);
        DVZ_VK_STR(ERROR_FORMAT_NOT_SUPPORTED);
        DVZ_VK_STR(ERROR_SURFACE_LOST_KHR);
        DVZ_VK_STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        DVZ_VK_STR(SUBOPTIMAL_KHR);
        DVZ_VK_STR(ERROR_OUT_OF_DATE_KHR);
        DVZ_VK_STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        DVZ_VK_STR(ERROR_VALIDATION_FAILED_EXT);
        DVZ_VK_STR(ERROR_INVALID_SHADER_NV);
    default:
        break;
    }

    if (res != VK_SUCCESS)
    {
        log_error("VkResult is %s in %s at line %d", str, file, line);
        return 1;
    }
    return 0;
}



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define ANNVK(x) ASSERT((x) != VK_NULL_HANDLE)

#define VK_CHECK_RESULT(f)                                                                        \
    do                                                                                             \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        vk_result_check(res, __FILE__, __LINE__);                                                  \
    } while (0)

#define VK_RETURN_RESULT(f)                                                                       \
    int out = 0;                                                                                   \
    do                                                                                             \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        out = vk_result_check(res, __FILE__, __LINE__);                                            \
    } while (0)

