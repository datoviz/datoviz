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
#include "../_status.h"
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



static int test_app_status_line_combines_trace_and_fps(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzAppStatus status;
    _dvz_app_status_init(&status);
    _dvz_app_status_trace(&status, 149, 27, 12, false);
    _dvz_app_status_fps(&status, 123.4, 124, 1.005);

    char line[192] = {0};
    AT(_dvz_app_status_line(&status, line, sizeof(line)));
    AT(strstr(line, "frame 00000149 | unchanged | 27 cmds | 12 semantic") != NULL);
    AT(strstr(line, "FPS  123.4") != NULL);
    AT(strstr(line, "124 frames in 1.005 s") != NULL);
    return 0;
}


static int test_app_status_line_rejects_truncation(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzAppStatus status;
    _dvz_app_status_init(&status);
    _dvz_app_status_trace(&status, UINT64_MAX, UINT32_MAX, UINT32_MAX, true);
    _dvz_app_status_fps(&status, 123456789.0, UINT32_MAX, 123456789.0);

    char line[16] = {0};
    AT(!_dvz_app_status_line(&status, line, sizeof(line)));
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


static int test_app_trace_fingerprint_ignores_transient_pass_ids(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_render_pass(a, 100, 7, 5000));
    AT(dvz_drp2_stream_set_pipeline(a, 100, 42));
    AT(dvz_drp2_stream_draw(a, 100, 300, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(a, 100));

    AT(dvz_drp2_stream_begin_render_pass(b, 104, 8, 5000));
    AT(dvz_drp2_stream_set_pipeline(b, 104, 42));
    AT(dvz_drp2_stream_draw(b, 104, 300, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(b, 104));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa == fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_ignores_transient_pass_ids(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_command_encoder(a, 7));
    AT(dvz_drp2_stream_begin_render_pass(a, 100, 7, 5000));
    AT(dvz_drp2_stream_set_pipeline(a, 100, 42));
    AT(dvz_drp2_stream_set_vertex_buffer(a, 100, 0, 77, 0));
    AT(dvz_drp2_stream_draw(a, 100, 300, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(a, 100));
    AT(dvz_drp2_stream_finish_command_encoder(a, 7, 900));
    AT(dvz_drp2_stream_queue_submit(a, 900, 901));

    AT(dvz_drp2_stream_begin_command_encoder(b, 8));
    AT(dvz_drp2_stream_begin_render_pass(b, 104, 8, 5000));
    AT(dvz_drp2_stream_set_pipeline(b, 104, 42));
    AT(dvz_drp2_stream_set_vertex_buffer(b, 104, 0, 77, 0));
    AT(dvz_drp2_stream_draw(b, 104, 300, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(b, 104));
    AT(dvz_drp2_stream_finish_command_encoder(b, 8, 902));
    AT(dvz_drp2_stream_queue_submit(b, 902, 903));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "render#0 target=5000 clear=load depth=no area=(0,0 1x1)") == 1);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#0 pipeline=42") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "render#0 draw vertices=300 first=0 instances=1") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_keeps_draw_payload(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_render_pass(a, 100, 7, 5000));
    AT(dvz_drp2_stream_draw(a, 100, 300, 1, 0, 0));

    AT(dvz_drp2_stream_begin_render_pass(b, 100, 7, 5000));
    AT(dvz_drp2_stream_draw(b, 100, 301, 1, 0, 0));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(!_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "render#0 draw vertices=300 first=0 instances=1") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sb, "render#0 draw vertices=301 first=0 instances=1") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_snapshot_rejects_truncated_suffix(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    char long_format[256] = {0};
    for (uint32_t i = 0; i < sizeof(long_format) - 1; i++)
        long_format[i] = 'x';

    AT(dvz_drp2_stream_begin_render_pass(stream, 100, 7, 5000));
    AT(dvz_drp2_stream_set_index_buffer(
        stream, 100, UINT64_MAX, long_format, UINT64_MAX));

    DvzAppTraceSnapshot snapshot;
    _dvz_app_trace_snapshot_init(&snapshot);
    AT(!_dvz_app_trace_snapshot_build(&snapshot, stream));
    AT(snapshot.count == 0);
    AT(snapshot.lines == NULL);

    _dvz_app_trace_snapshot_destroy(&snapshot);
    dvz_drp2_stream_destroy(stream);
    return 0;
}


static int test_app_trace_snapshot_recovers_after_failed_build(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* bad = dvz_drp2_stream();
    DvzDrp2CommandStream* good = dvz_drp2_stream();
    ANN(bad);
    ANN(good);

    char long_format[256] = {0};
    for (uint32_t i = 0; i < sizeof(long_format) - 1; i++)
        long_format[i] = 'x';

    AT(dvz_drp2_stream_begin_render_pass(bad, 100, 7, 5000));
    AT(dvz_drp2_stream_set_index_buffer(bad, 100, UINT64_MAX, long_format, UINT64_MAX));

    AT(dvz_drp2_stream_begin_render_pass(good, 100, 7, 5000));
    AT(dvz_drp2_stream_draw(good, 100, 3, 1, 0, 0));

    DvzAppTraceSnapshot snapshot;
    _dvz_app_trace_snapshot_init(&snapshot);
    AT(!_dvz_app_trace_snapshot_build(&snapshot, bad));
    AT(snapshot.count == 0);
    AT(snapshot.lines == NULL);

    AT(_dvz_app_trace_snapshot_build(&snapshot, good));
    AT(snapshot.count == 2);
    AT(_dvz_app_trace_snapshot_line_count(
           &snapshot, "render#0 draw vertices=3 first=0 instances=1") == 1);

    _dvz_app_trace_snapshot_destroy(&snapshot);
    dvz_drp2_stream_destroy(bad);
    dvz_drp2_stream_destroy(good);
    return 0;
}



int test_app(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "app";
    TEST_SIMPLE(test_app_trace_mode_parsing);
    TEST_SIMPLE(test_app_trace_plan_normal_changed_after_open_line);
    TEST_SIMPLE(test_app_trace_plan_normal_unchanged_rewrites_in_place);
    TEST_SIMPLE(test_app_status_line_combines_trace_and_fps);
    TEST_SIMPLE(test_app_status_line_rejects_truncation);
    TEST_SIMPLE(test_app_trace_fingerprint_name_is_frame_stable);
    TEST_SIMPLE(test_app_trace_fingerprint_ignores_frame_handles_and_payloads);
    TEST_SIMPLE(test_app_trace_fingerprint_keeps_write_ranges);
    TEST_SIMPLE(test_app_trace_fingerprint_ignores_transient_pass_ids);
    TEST_SIMPLE(test_app_trace_snapshot_ignores_transient_pass_ids);
    TEST_SIMPLE(test_app_trace_snapshot_keeps_draw_payload);
    TEST_SIMPLE(test_app_trace_snapshot_rejects_truncated_suffix);
    TEST_SIMPLE(test_app_trace_snapshot_recovers_after_failed_build);
    return 0;
}
