/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing DRP2                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_stream_empty(TstSuite* suite, TstItem* item);

int test_drp2_stream_append(TstSuite* suite, TstItem* item);

int test_drp2_stream_json(TstSuite* suite, TstItem* item);



int test_drp2_runtime_validate_render_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_duplicate_id(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_unknown_buffer_write(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_draw_without_vertex_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_finish_with_open_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_bad_readback_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_compute_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_dispatch_without_pipeline(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_dispatch_outside_compute_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_wrong_pipeline_type(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_finish_with_open_compute_pass(TstSuite* suite, TstItem* item);

int test_drp2_runtime_validate_indexed_render_stream(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_draw_indexed_without_index_buffer(TstSuite* suite, TstItem* item);

int test_drp2_runtime_rejects_wrong_index_buffer_usage(TstSuite* suite, TstItem* item);



int test_drp2(TstSuite* suite);
