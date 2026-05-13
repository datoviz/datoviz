/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App tests                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "../_trace.h"
#include "datoviz/drp2/stream.h"
#include "test_app.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static int test_app_trace_mode_parsing(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    AT(_dvz_app_trace_mode_from_env(NULL) == DVZ_APP_TRACE_NONE);
    AT(_dvz_app_trace_mode_from_env("0") == DVZ_APP_TRACE_NONE);
    AT(_dvz_app_trace_mode_from_env("false") == DVZ_APP_TRACE_NONE);
    AT(_dvz_app_trace_mode_from_env("1") == DVZ_APP_TRACE_NORMAL);
    AT(_dvz_app_trace_mode_from_env("true") == DVZ_APP_TRACE_NORMAL);
    AT(_dvz_app_trace_mode_from_env("normal") == DVZ_APP_TRACE_NORMAL);
    AT(_dvz_app_trace_mode_from_env("full") == DVZ_APP_TRACE_FULL);
    return 0;
}



static int test_app_trace_plan_normal_changed_after_open_line(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzAppTracePlan plan = _dvz_app_trace_plan(DVZ_APP_TRACE_NORMAL, true, true);
    AT(plan.event_kind == DVZ_APP_TRACE_EVENT_CHANGED);
    AT(plan.prepend_newline);
    AT(!plan.rewrite_in_place);
    AT(!plan.status_line_open_after);
    return 0;
}



static int test_app_trace_plan_normal_unchanged_rewrites_in_place(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzAppTracePlan plan = _dvz_app_trace_plan(DVZ_APP_TRACE_NORMAL, false, false);
    AT(plan.event_kind == DVZ_APP_TRACE_EVENT_UNCHANGED);
    AT(!plan.prepend_newline);
    AT(plan.rewrite_in_place);
    AT(plan.status_line_open_after);
    return 0;
}



static int test_app_trace_status_line_uses_carriage_return_and_clear(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    char line[96] = {0};
    AT(_dvz_app_trace_status_line(149, 27, line, sizeof(line)));
    AT(strncmp(line, "\r\x1b[2Kframe 00000149 | unchanged | 27 cmds", 42) == 0);
    return 0;
}


static int test_app_trace_fingerprint_name_is_frame_stable(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    char name[32] = {0};
    AT(_dvz_app_trace_fingerprint_name(name, sizeof(name)));
    AT(strcmp(name, "live_frame") == 0);
    AT(strstr(name, "000") == NULL);
    AT(strstr(name, "frame_") == NULL);
    return 0;
}


static int test_app_trace_fingerprint_ignores_frame_handles_and_payloads(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_write_buffer(a, 42, 8, 4, "AAAA"));
    AT(dvz_drp2_stream_finish_command_encoder(a, 7, 100));
    AT(dvz_drp2_stream_queue_submit(a, 100, 200));

    AT(dvz_drp2_stream_write_buffer(b, 42, 8, 4, "BBBB"));
    AT(dvz_drp2_stream_finish_command_encoder(b, 7, 101));
    AT(dvz_drp2_stream_queue_submit(b, 101, 201));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa == fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_fingerprint_keeps_write_ranges(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_write_buffer(a, 42, 8, 4, "AAAA"));
    AT(dvz_drp2_stream_write_buffer(b, 42, 12, 4, "AAAA"));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa != fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



int test_app(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "app";
    TEST_SIMPLE(test_app_trace_mode_parsing);
    TEST_SIMPLE(test_app_trace_plan_normal_changed_after_open_line);
    TEST_SIMPLE(test_app_trace_plan_normal_unchanged_rewrites_in_place);
    TEST_SIMPLE(test_app_trace_status_line_uses_carriage_return_and_clear);
    TEST_SIMPLE(test_app_trace_fingerprint_name_is_frame_stable);
    TEST_SIMPLE(test_app_trace_fingerprint_ignores_frame_handles_and_payloads);
    TEST_SIMPLE(test_app_trace_fingerprint_keeps_write_ranges);
    return 0;
}
