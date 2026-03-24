/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing canvas module                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_canvas_defaults(TstSuite* suite, TstItem* item);

int test_canvas_frame_pool(TstSuite* suite, TstItem* item);

int test_canvas_timings(TstSuite* suite, TstItem* item);
int test_canvas_offscreen_destroy_recreate(TstSuite* suite, TstItem* item);
int test_canvas_glfw_destroy_recreate(TstSuite* suite, TstItem* item);

int test_canvas_glfw(TstSuite* suite, TstItem* item);

int test_canvas_swapchain_failfast_slot_init(TstSuite* suite, TstItem* item);

int test_canvas_glfw_present_recovery(TstSuite* suite, TstItem* item);

int test_canvas_handle_refresh_order(TstSuite* suite, TstItem* item);

int test_canvas_video_wait_value_propagation(TstSuite* suite, TstItem* item);

int test_canvas_video_wait_handle_ready_on_first_start(TstSuite* suite, TstItem* item);

int test_canvas_video_wait_handle_export_fallback(TstSuite* suite, TstItem* item);

int test_canvas_video_wait_handle_export_fallback_after_recreate(TstSuite* suite, TstItem* item);

int test_canvas_video_sink_start_submit_integration(TstSuite* suite, TstItem* item);

int test_canvas_video_handle_refresh_after_recreate(TstSuite* suite, TstItem* item);

int test_canvas_video_sink_disable_rebuild(TstSuite* suite, TstItem* item);

int test_canvas_capture_api(TstSuite* suite, TstItem* item);

int test_canvas_device_lost_fatal_transition(TstSuite* suite, TstItem* item);

int test_canvas_glfw_wrap_surface_present_recovery(TstSuite* suite, TstItem* item);
int test_canvas_glfw_wrap_surface_resize_recreate_refreshes_state(TstSuite* suite, TstItem* item);



int test_canvas(TstSuite* suite);
