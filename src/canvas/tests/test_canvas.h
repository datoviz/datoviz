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

int test_canvas_defaults(TstContext* suite, const TstCase* item);

int test_canvas_config_rejects_invalid_abi(TstContext* suite, const TstCase* item);

int test_canvas_configure_gpu_ctx(TstContext* suite, const TstCase* item);

int test_canvas_frame_format(TstContext* suite, const TstCase* item);

int test_canvas_frame_pool(TstContext* suite, const TstCase* item);

int test_canvas_frame_slot_count_resolution(TstContext* suite, const TstCase* item);

int test_canvas_fifo_latest_fallback(TstContext* suite, const TstCase* item);

int test_canvas_timings(TstContext* suite, const TstCase* item);
int test_canvas_offscreen_destroy_recreate(TstContext* suite, const TstCase* item);
int test_canvas_glfw_destroy_recreate(TstContext* suite, const TstCase* item);

int test_canvas_glfw(TstContext* suite, const TstCase* item);

int test_canvas_swapchain_failfast_slot_init(TstContext* suite, const TstCase* item);

int test_canvas_glfw_present_recovery(TstContext* suite, const TstCase* item);

int test_canvas_glfw_pre_submit_failure_recovery(TstContext* suite, const TstCase* item);

int test_canvas_glfw_acquire_error_recovery(TstContext* suite, const TstCase* item);

int test_canvas_glfw_submit_error_recovery(TstContext* suite, const TstCase* item);

int test_canvas_glfw_auto_format_stable(TstContext* suite, const TstCase* item);

int test_canvas_glfw_present_semaphore_reuse(TstContext* suite, const TstCase* item);

int test_canvas_glfw_one_frame_slot(TstContext* suite, const TstCase* item);

int test_canvas_glfw_two_frame_slots(TstContext* suite, const TstCase* item);

int test_canvas_handle_refresh_order(TstContext* suite, const TstCase* item);

int test_canvas_video_wait_value_propagation(TstContext* suite, const TstCase* item);

int test_canvas_video_wait_handle_ready_on_first_start(TstContext* suite, const TstCase* item);

int test_canvas_video_wait_handle_export_fallback(TstContext* suite, const TstCase* item);

int test_canvas_video_wait_handle_export_fallback_after_recreate(TstContext* suite, const TstCase* item);

int test_canvas_video_sink_start_submit_integration(TstContext* suite, const TstCase* item);

int test_canvas_video_handle_refresh_after_recreate(TstContext* suite, const TstCase* item);

int test_canvas_video_sink_disable_rebuild(TstContext* suite, const TstCase* item);

int test_canvas_capture_api(TstContext* suite, const TstCase* item);

int test_canvas_device_lost_fatal_transition(TstContext* suite, const TstCase* item);

int test_canvas_glfw_wrap_surface_present_recovery(TstContext* suite, const TstCase* item);
int test_canvas_glfw_wrap_surface_resize_recreate_refreshes_state(TstContext* suite, const TstCase* item);



int test_canvas(TstSuite* suite);
