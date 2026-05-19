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

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../_status.h"
#include "../_trace.h"
#include "../../drp2/_stream.h"
#include "datoviz/app.h"
#include "datoviz/drp2/stream.h"
#include "test_app.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static void _test_restore_env(const char* name, const char* value)
{
    ANN(name);
    if (value != NULL)
        (void)setenv(name, value, 1);
    else
        (void)unsetenv(name);
}



static int test_app_config_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_schedule = getenv("DVZ_APP_SCHEDULE");
    const char* old_fps_cap = getenv("DVZ_FPS_CAP");
    char saved_schedule[64] = {0};
    char saved_fps_cap[64] = {0};
    if (old_schedule != NULL)
        dvz_snprintf(saved_schedule, sizeof(saved_schedule), "%s", old_schedule);
    if (old_fps_cap != NULL)
        dvz_snprintf(saved_fps_cap, sizeof(saved_fps_cap), "%s", old_fps_cap);
    (void)unsetenv("DVZ_APP_SCHEDULE");
    (void)unsetenv("DVZ_FPS_CAP");

    DvzAppConfig config = dvz_app_config();
    AT(config.instance_extension_count == 0);
    AT(config.instance_extensions == NULL);
    AT(!config.enable_canvas_extensions);
    AT(config.enable_glfw_extensions);
    AT(config.schedule_mode == DVZ_APP_SCHEDULE_ON_DEMAND);
    AT(config.fps_cap == 0);

    _test_restore_env("DVZ_APP_SCHEDULE", old_schedule != NULL ? saved_schedule : NULL);
    _test_restore_env("DVZ_FPS_CAP", old_fps_cap != NULL ? saved_fps_cap : NULL);
    return 0;
}



static int test_app_config_env_schedule(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_schedule = getenv("DVZ_APP_SCHEDULE");
    char saved_schedule[64] = {0};
    if (old_schedule != NULL)
        dvz_snprintf(saved_schedule, sizeof(saved_schedule), "%s", old_schedule);

    AT(setenv("DVZ_APP_SCHEDULE", "continuous", 1) == 0);
    DvzAppConfig config = dvz_app_config();
    AT(config.schedule_mode == DVZ_APP_SCHEDULE_CONTINUOUS);

    AT(setenv("DVZ_APP_SCHEDULE", "on_demand", 1) == 0);
    config = dvz_app_config();
    AT(config.schedule_mode == DVZ_APP_SCHEDULE_ON_DEMAND);

    _test_restore_env("DVZ_APP_SCHEDULE", old_schedule != NULL ? saved_schedule : NULL);
    return 0;
}



static int test_app_config_env_fps_cap(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_fps_cap = getenv("DVZ_FPS_CAP");
    char saved_fps_cap[64] = {0};
    if (old_fps_cap != NULL)
        dvz_snprintf(saved_fps_cap, sizeof(saved_fps_cap), "%s", old_fps_cap);

    AT(setenv("DVZ_FPS_CAP", "144.5", 1) == 0);
    DvzAppConfig config = dvz_app_config();
    AT(config.fps_cap == 144.5);

    _test_restore_env("DVZ_FPS_CAP", old_fps_cap != NULL ? saved_fps_cap : NULL);
    return 0;
}



static int test_app_trace_mode_parsing(TstContext* suite, const TstCase* item)
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



static int test_app_trace_plan_normal_changed_after_open_line(TstContext* suite, const TstCase* item)
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



static int test_app_trace_plan_normal_unchanged_rewrites_in_place(TstContext* suite, const TstCase* item)
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



static int test_app_status_line_combines_trace_and_fps(TstContext* suite, const TstCase* item)
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


static int test_app_status_line_rejects_truncation(TstContext* suite, const TstCase* item)
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


static int test_app_trace_fingerprint_name_is_frame_stable(TstContext* suite, const TstCase* item)
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
    TstContext* suite, const TstCase* item)
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


static int test_app_trace_fingerprint_keeps_write_ranges(TstContext* suite, const TstCase* item)
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


static int test_app_trace_fingerprint_keeps_texture_format(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_create_texture_2d_format_usage(a, 42, 64, 64, 37, 0x12));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(b, 42, 64, 64, 38, 0x12));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa != fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_fingerprint_keeps_pipeline_attachment_state(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_create_render_pipeline(a, 10, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(a, 1, 2));
    AT(dvz_drp2_stream_pipeline_set_color_target(a, 0, 37));
    AT(dvz_drp2_stream_pipeline_set_color_blend(a, 0, 1, 2, 3, 4, 5, 6, 0x0f));

    AT(dvz_drp2_stream_create_render_pipeline(b, 10, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_raster_state(b, 1, 2));
    AT(dvz_drp2_stream_pipeline_set_color_target(b, 0, 38));
    AT(dvz_drp2_stream_pipeline_set_color_blend(b, 0, 1, 2, 3, 4, 5, 6, 0x0f));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa != fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_fingerprint_keeps_render_attachment_ops(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_render_pass(a, 100, 7, 5000));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        a, 0, DVZ_DRP2_ATTACHMENT_LOAD_CLEAR, DVZ_DRP2_ATTACHMENT_STORE_STORE));

    AT(dvz_drp2_stream_begin_render_pass(b, 104, 8, 5000));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        b, 0, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_STORE));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa != fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_fingerprint_keeps_dynamic_offsets(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    uint64_t offsets_a[2] = {16, 32};
    uint64_t offsets_b[2] = {16, 48};
    AT(dvz_drp2_stream_set_bind_group_dynamic(a, 100, 0, 77, 2, offsets_a));
    AT(dvz_drp2_stream_set_bind_group_dynamic(b, 104, 0, 77, 2, offsets_b));

    uint64_t fa = 0;
    uint64_t fb = 0;
    AT(_dvz_app_trace_fingerprint(a, &fa));
    AT(_dvz_app_trace_fingerprint(b, &fb));
    AT(fa != fb);

    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}


static int test_app_trace_fingerprint_bounds_fixed_labels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "client"));
    AT(stream->count == 1);

    DvzDrp2Command* command = &stream->commands[0];
    ANN(command);
    dvz_memset(
        command->u.handshake.name, sizeof(command->u.handshake.name), 'x',
        sizeof(command->u.handshake.name));

    uint64_t fingerprint = 0;
    AT(_dvz_app_trace_fingerprint(stream, &fingerprint));
    AT(fingerprint != 0);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


static int test_app_trace_fingerprint_ignores_transient_pass_ids(TstContext* suite, const TstCase* item)
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



static int test_app_trace_snapshot_ignores_transient_pass_ids(TstContext* suite, const TstCase* item)
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



static int test_app_trace_snapshot_keeps_draw_payload(TstContext* suite, const TstCase* item)
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


static int test_app_trace_snapshot_normalizes_scoped_edl_resources(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_create_texture_2d_format_usage(a, 27, 800, 600, 126, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 27, "fig0_p0.edl.depth_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(a, 28, 800, 600, 44, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 28, "fig0_p0.edl.color_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5017, 5016, 28, 26));
    AT(dvz_drp2_stream_set_label(a, 5017, "_bg_edl_28_27_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 28, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(a, 27, 1));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5017));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_create_texture_2d_format_usage(b, 29, 800, 600, 126, 0x14));
    AT(dvz_drp2_stream_set_label(
        b, 29, "fig0_p0.edl.depth_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(b, 30, 800, 600, 44, 0x14));
    AT(dvz_drp2_stream_set_label(
        b, 30, "fig0_p0.edl.color_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5021, 5016, 30, 26));
    AT(dvz_drp2_stream_set_label(b, 5021, "_bg_edl_30_29_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 104, 8, 30, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(b, 29, 1));
    AT(dvz_drp2_stream_set_bind_group(b, 104, 0, 5021));
    AT(dvz_drp2_stream_draw(b, 104, 3, 1, 0, 0));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "+ texture id=fig0_p0.edl.color size=800x600x1 usage=0x14") == 0);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa,
           "render#0 target=fig0_p0.edl.color clear=yes depth=yes "
           "depth_target=fig0_p0.edl.depth area=(0,0 1x1)") == 1);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#0 bind[0]=_bg_edl_resolve") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_normalizes_scoped_ssao_resources(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 34, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        a, 34, "fig0_p0.gbuffer.normal_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(a, 33, 1));
    AT(dvz_drp2_stream_set_label(
        a, 33, "fig0_p0.gbuffer.depth_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5034));
    AT(dvz_drp2_stream_set_label(a, 5034, "_bg_ssao_34_33_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 100));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 101, 7, 36, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        a, 36, "fig0_p0.ssao.blur_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_set_bind_group(a, 101, 0, 5035));
    AT(dvz_drp2_stream_set_label(a, 5035, "_bg_ssao_blur_35_34_33_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 101));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 102, 7, 5000, 0, 0, 0, 1, 0, 0, 1, 1, false));
    AT(dvz_drp2_stream_set_label(a, 5000, "rt"));
    AT(dvz_drp2_stream_set_bind_group(a, 102, 0, 5036));
    AT(dvz_drp2_stream_set_label(a, 5036, "_bg_ssao_composite_36_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 102));

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 200, 8, 39, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 39, "fig0_p0.gbuffer.normal_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(b, 38, 1));
    AT(dvz_drp2_stream_set_label(
        b, 38, "fig0_p0.gbuffer.depth_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_set_bind_group(b, 200, 0, 5037));
    AT(dvz_drp2_stream_set_label(b, 5037, "_bg_ssao_39_38_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 200));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 201, 8, 41, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 41, "fig0_p0.ssao.blur_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_set_bind_group(b, 201, 0, 5038));
    AT(dvz_drp2_stream_set_label(b, 5038, "_bg_ssao_blur_40_39_38_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 201));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 202, 8, 5000, 0, 0, 0, 1, 0, 0, 1, 1, false));
    AT(dvz_drp2_stream_set_label(b, 5000, "rt"));
    AT(dvz_drp2_stream_set_bind_group(b, 202, 0, 5039));
    AT(dvz_drp2_stream_set_label(b, 5039, "_bg_ssao_composite_41_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 202));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#0 bind[0]=_bg_ssao") == 1);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#1 bind[0]=_bg_ssao_blur") == 1);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#2 bind[0]=_bg_ssao_composite") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_ignores_transient_scoped_creates(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_create_texture_2d_format_usage(a, 34, 100, 100, 126, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 34, "fig0_p0.gbuffer.normal_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5034, 5019, 34, 26));
    AT(dvz_drp2_stream_set_label(a, 5034, "_bg_ssao_34_33_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 34, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5034));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_set_label(
        b, 35, "fig0_p0.gbuffer.normal_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_set_label(b, 5035, "_bg_ssao_35_33_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 101, 8, 35, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_bind_group(b, 101, 0, 5035));
    AT(dvz_drp2_stream_draw(b, 101, 3, 1, 0, 0));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "+ texture id=fig0_p0.gbuffer.normal size=100x100x1 usage=0x14") == 0);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "+ bind-group id=_bg_ssao") == 0);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#0 bind[0]=_bg_ssao") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_keeps_scoped_edl_draw_payload(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 28, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        a, 28, "fig0_p0.edl.color_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(a, 27, 1));
    AT(dvz_drp2_stream_set_label(
        a, 27, "fig0_p0.edl.depth_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 104, 8, 30, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 30, "fig0_p0.edl.color_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(b, 29, 1));
    AT(dvz_drp2_stream_set_label(
        b, 29, "fig0_p0.edl.depth_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_draw(b, 104, 4, 1, 0, 0));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(!_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "render#0 draw vertices=3 first=0 instances=1") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sb, "render#0 draw vertices=4 first=0 instances=1") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_rejects_truncated_suffix(TstContext* suite, const TstCase* item)
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


static int test_app_trace_snapshot_recovers_after_failed_build(TstContext* suite, const TstCase* item)
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
    TST_MODULE(suite, tags);
    TST_CASE(test_app_config_defaults);
    TST_CASE(test_app_config_env_schedule);
    TST_CASE(test_app_config_env_fps_cap);
    TST_CASE(test_app_trace_mode_parsing);
    TST_CASE(test_app_trace_plan_normal_changed_after_open_line);
    TST_CASE(test_app_trace_plan_normal_unchanged_rewrites_in_place);
    TST_CASE(test_app_status_line_combines_trace_and_fps);
    TST_CASE(test_app_status_line_rejects_truncation);
    TST_CASE(test_app_trace_fingerprint_name_is_frame_stable);
    TST_CASE(test_app_trace_fingerprint_ignores_frame_handles_and_payloads);
    TST_CASE(test_app_trace_fingerprint_keeps_write_ranges);
    TST_CASE(test_app_trace_fingerprint_keeps_texture_format);
    TST_CASE(test_app_trace_fingerprint_keeps_pipeline_attachment_state);
    TST_CASE(test_app_trace_fingerprint_keeps_render_attachment_ops);
    TST_CASE(test_app_trace_fingerprint_keeps_dynamic_offsets);
    TST_CASE(test_app_trace_fingerprint_bounds_fixed_labels);
    TST_CASE(test_app_trace_fingerprint_ignores_transient_pass_ids);
    TST_CASE(test_app_trace_snapshot_ignores_transient_pass_ids);
    TST_CASE(test_app_trace_snapshot_keeps_draw_payload);
    TST_CASE(test_app_trace_snapshot_normalizes_scoped_edl_resources);
    TST_CASE(test_app_trace_snapshot_normalizes_scoped_ssao_resources);
    TST_CASE(test_app_trace_snapshot_ignores_transient_scoped_creates);
    TST_CASE(test_app_trace_snapshot_keeps_scoped_edl_draw_payload);
    TST_CASE(test_app_trace_snapshot_rejects_truncated_suffix);
    TST_CASE(test_app_trace_snapshot_recovers_after_failed_build);
    return 0;
}
