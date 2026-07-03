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
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
#include "datoviz/runner/enums.h"
#include "datoviz/window/size.h"



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
typedef struct DvzWindowMetrics DvzWindowMetrics;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzHiDpiPolicy
{
    DVZ_HIDPI_AUTO = 0,
    DVZ_HIDPI_DISABLED,
    DVZ_HIDPI_FRAMEBUFFER,
    DVZ_HIDPI_NATIVE_WINDOW,
    DVZ_HIDPI_EXTERNAL,
} DvzHiDpiPolicy;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindowConfig
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    const char* title;
    bool resizable;
    bool visible;
    float user_scale;
    DvzHiDpiPolicy hidpi_policy;
};



struct DvzWindowMetrics
{
    DvzExtent logical_size;
    DvzExtent native_size;
    DvzExtent surface_size;
    DvzExtent render_size;

    DvzScaleXY content_scale;
    DvzScaleXY framebuffer_scale;
    DvzScaleXY device_scale;
    DvzScaleXY native_to_logical;

    DvzHiDpiPolicy active_hidpi_policy;
    uint64_t generation;
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
