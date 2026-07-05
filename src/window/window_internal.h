/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window host                                                                                  */
/*************************************************************************************************/

#pragma once


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/input/pointer.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#include "datoviz/window/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WINDOW_BACKEND_INIT_CAP  4
#define DVZ_WINDOW_INSTANCE_INIT_CAP 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindowBackendSlot DvzWindowBackendSlot;
typedef struct DvzWindowWrapBackendState DvzWindowWrapBackendState;
typedef struct DvzWindowScaleInputs DvzWindowScaleInputs;
typedef struct DvzWindowMetricsInputs DvzWindowMetricsInputs;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindowBackendSlot
{
    DvzWindowBackend backend;
    bool available;
    bool probed;
};



struct DvzWindowWrapBackendState
{
    uint32_t extension_count;
    char** extensions;
};



struct DvzWindowScaleInputs
{
    float window_scale_x;
    float window_scale_y;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t window_width;
    uint32_t window_height;
    float monitor_scale_x;
    float monitor_scale_y;
    uint32_t monitor_pixel_width;
    uint32_t monitor_pixel_height;
    uint32_t monitor_width_mm;
    uint32_t monitor_height_mm;
    float override_scale;
};



struct DvzWindowMetricsInputs
{
    DvzExtent requested_logical_size;
    DvzExtent native_size;
    DvzExtent framebuffer_size;
    DvzScaleXY content_scale;
    DvzHiDpiPolicy requested_policy;
    uint64_t previous_generation;
};



struct DvzWindow
{
    DvzWindowHost* host;
    DvzWindowBackendSlot* backend_slot;
    void* backend_handle;
    void* backend_payload;
    DvzInputRouter* router;
    DvzPointerGestureHandler* gesture_handler;
    DvzWindowConfig config;
    DvzWindowSurface surface;
    DvzWindowMetrics metrics;
    DvzWindowGlfwInputCallbacks glfw_input_callbacks;
    void* glfw_input_user_data;
    bool backend_owns_surface;
    char title[DVZ_WINDOW_TITLE_MAX];
    bool frame_pending;
    void* user_data;
};



struct DvzWindowHost
{
    DvzWindow** windows;
    uint32_t window_count;
    uint32_t window_capacity;

    DvzWindowBackendSlot* backends;
    uint32_t backend_count;
    uint32_t backend_capacity;
    DvzWindowWrapBackendState wrap_state;
};



void _dvz_window_effective_content_scale(
    const DvzWindowScaleInputs* inputs, float* out_x, float* out_y);

void _dvz_window_metrics_resolve(
    const DvzWindowMetricsInputs* inputs, DvzWindowMetrics* out);

void _dvz_window_backend_emit_metrics(DvzWindow* window, const DvzWindowMetrics* metrics);
