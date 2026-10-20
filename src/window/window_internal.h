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
    uint32_t refresh_rate_hz;
    DvzHiDpiPolicy requested_policy;
    uint64_t previous_generation;
};



typedef struct DvzHostInputSubscription
{
    DvzCallbackId id;
    DvzHostInputCallback callback;
    void* user_data;
} DvzHostInputSubscription;


typedef bool (*DvzHostInputDispatch)(DvzWindow*, const DvzHostInputEvent*, void*);


struct DvzWindow
{
    DvzWindowHost* host;
    uint32_t backend_index;
    void* backend_handle;
    void* backend_payload;
    DvzInputRouter* router;
    DvzPointerGestureHandler* gesture_handler;
    DvzWindowConfig config;
    DvzWindowSurface surface;
    DvzWindowMetrics metrics;
    DvzWindowGlfwInputCallbacks glfw_input_callbacks;
    void* glfw_input_user_data;
    int glfw_mods;
    DvzHostInputState input_state;
    bool input_keys[349];
    bool input_cancelled_keys[349];
    uint32_t input_cancelled_buttons;
    bool input_dispatching;
    DvzHostInputSubscription* input_subscriptions;
    uint32_t input_subscription_count;
    uint64_t input_next_id;
    DvzHostInputDispatch input_dispatch;
    uint32_t (*input_pending)(void*);
    void (*input_mode)(bool, void*);
    void* input_dispatch_data;
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
void _dvz_window_apply_metrics(DvzWindow* window, const DvzWindowMetrics* metrics);

void _dvz_window_glfw_shutdown(void);
void _dvz_window_glfw_refresh_input(DvzWindow* window);


/**
 * Fit native content bounds to a monitor workarea, accounting for window decorations.
 *
 * @param bounds in/out x, y, width, height in native window coordinates
 * @param workarea monitor x, y, width, height in the same coordinates
 * @param frame left, top, right, bottom decoration widths
 */
void _dvz_window_fit_workarea(
    int32_t bounds[4], const int32_t workarea[4], const int32_t frame[4]);

EXTERN_C_ON
void _dvz_window_set_host_input_dispatch(
    DvzWindow* window, DvzHostInputDispatch callback, uint32_t (*pending)(void*),
    void (*mode)(bool, void*), void* user_data);
bool _dvz_window_cancelled_release(DvzWindow* window, bool keyboard, int token);
EXTERN_C_OFF
