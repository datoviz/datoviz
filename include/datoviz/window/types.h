/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window types                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/runner/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WINDOW_TITLE_MAX      256
#define DVZ_WINDOW_DEFAULT_WIDTH  1280
#define DVZ_WINDOW_DEFAULT_HEIGHT 720
#define DVZ_WINDOW_DEFAULT_TITLE  "Datoviz"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInputRouter DvzInputRouter;
typedef struct DvzWindowHost DvzWindowHost;
typedef struct DvzWindow DvzWindow;
typedef struct DvzWindowSurface DvzWindowSurface;
typedef struct DvzWindowConfig DvzWindowConfig;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindowConfig
{
    uint32_t width;
    uint32_t height;
    const char* title;
    bool resizable;
    float user_scale;
};



struct DvzWindowSurface
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    float scale_x;
    float scale_y;
};
