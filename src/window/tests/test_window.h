/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing window module                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_window_headless(TstContext* suite, const TstCase* item);

int test_window_config_rejects_invalid_abi(TstContext* suite, const TstCase* item);

int test_window_resize_events(TstContext* suite, const TstCase* item);

int test_window_frame_requests(TstContext* suite, const TstCase* item);

int test_window_wait_hooks_headless(TstContext* suite, const TstCase* item);

int test_window_effective_scale_override(TstContext* suite, const TstCase* item);

int test_window_effective_scale_framebuffer_ratio(TstContext* suite, const TstCase* item);

int test_window_effective_scale_monitor(TstContext* suite, const TstCase* item);

int test_window_effective_scale_raw_dpi(TstContext* suite, const TstCase* item);

int test_window_fallback(TstContext* suite, const TstCase* item);

int test_window_wrap_create(TstContext* suite, const TstCase* item);

int test_window_wrap_attach_detach(TstContext* suite, const TstCase* item);

int test_window_required_extensions_headless(TstContext* suite, const TstCase* item);

int test_window_required_extensions_wrap(TstContext* suite, const TstCase* item);

int test_window_wrap_invalid_args(TstContext* suite, const TstCase* item);

int test_window_required_extensions_invalid_args(TstContext* suite, const TstCase* item);

int test_window_wrap_replace_surface(TstContext* suite, const TstCase* item);

int test_window_wrap_owned_surface_null_lifecycle(TstContext* suite, const TstCase* item);



int test_window(TstSuite* suite);
