/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream tests                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int test_stream_attach_video(TstContext* suite, const TstCase* item);



int test_stream_attach_rolls_back_failed_create(TstContext* suite, const TstCase* item);



int test_stream_start_rollback_on_sink_failure(TstContext* suite, const TstCase* item);



int test_stream_submit_returns_first_error(TstContext* suite, const TstCase* item);



int test_stream_update_restart_failure_stops_stream(TstContext* suite, const TstCase* item);



int test_stream_attach_sink_name_prefers_requested_then_auto(TstContext* suite, const TstCase* item);



int test_stream(TstSuite* suite);
