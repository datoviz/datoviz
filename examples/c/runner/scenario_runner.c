/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Native runner support for portable C scenarios. */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "runner/scenario_runner.h"

#include "_compat.h"
#include "_time_utils.h"
#include "datoviz/scene.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define RUNNER_DEFAULT_FRAMES 120u
#define RUNNER_PATH_SIZE      1024u
#define RUNNER_PROGRESS_WIDTH 32u
#define RUNNER_NS_PER_SEC     1000000000ull



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct RunnerFrameState
{
    const DvzScenarioSpec* spec;
    DvzScenarioContext* ctx;
    void* user;
} RunnerFrameState;


typedef struct RunnerEventState
{
    const DvzScenarioSpec* spec;
    DvzScenarioContext* ctx;
    void* user;
} RunnerEventState;


typedef struct RunnerProgressState
{
    uint32_t frame_count;
    uint32_t rendered;
} RunnerProgressState;


typedef struct RunnerViewFrameState
{
    const DvzScenarioSpec* spec;
    DvzScenarioContext* ctx;
    void* user;
    RunnerProgressState progress;
} RunnerViewFrameState;


typedef struct RunnerRequirementName
{
    uint64_t bit;
    const char* name;
} RunnerRequirementName;



/*************************************************************************************************/
/*  Requirement table                                                                            */
/*************************************************************************************************/

static const RunnerRequirementName REQ_NAMES[] = {
    {DVZ_SCENARIO_REQ_POINT_VISUAL, "point"},
    {DVZ_SCENARIO_REQ_MARKER_VISUAL, "marker"},
    {DVZ_SCENARIO_REQ_MESH_VISUAL, "mesh"},
    {DVZ_SCENARIO_REQ_IMAGE_VISUAL, "image"},
    {DVZ_SCENARIO_REQ_TEXT_VISUAL, "text"},
    {DVZ_SCENARIO_REQ_SCENE_BUFFERS, "scene-buffers"},
    {DVZ_SCENARIO_REQ_STORAGE_BUFFERS, "storage-buffers"},
    {DVZ_SCENARIO_REQ_SCENE_COMPUTE, "scene-compute"},
    {DVZ_SCENARIO_REQ_QUERY_READBACK, "query-readback"},
    {DVZ_SCENARIO_REQ_FRAME_CALLBACKS, "frame-callbacks"},
    {DVZ_SCENARIO_REQ_NATIVE_CAPTURE, "native-capture"},
    {DVZ_SCENARIO_REQ_NATIVE_VIEW, "native-view"},
    {DVZ_SCENARIO_REQ_CONTROLLER, "controller"},
    {DVZ_SCENARIO_REQ_PANZOOM, "panzoom"},
    {DVZ_SCENARIO_REQ_ARCBALL, "arcball"},
};

static const uint32_t REQ_NAME_COUNT = sizeof(REQ_NAMES) / sizeof(REQ_NAMES[0]);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _parse_u32(const char* text, uint32_t* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;

    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0'))
        return false;
    if (value > UINT32_MAX)
        value = UINT32_MAX;

    *out = (uint32_t)value;
    return true;
}



static uint32_t _frames_after(int argc, char** argv, int index, uint32_t fallback)
{
    uint32_t out = 0;
    if (index + 1 < argc && _parse_u32(argv[index + 1], &out))
        return out != 0 ? out : fallback;
    return fallback;
}



static const char* _capture_extension(DvzRunnerCaptureKind kind)
{
    switch (kind)
    {
    case DVZ_RUNNER_CAPTURE_VIDEO:
        return ".mp4";
    case DVZ_RUNNER_CAPTURE_PNG:
        return ".png";
    case DVZ_RUNNER_CAPTURE_DVZR:
        return ".dvzr";
    case DVZ_RUNNER_CAPTURE_NONE:
    default:
        return NULL;
    }
}



static uint32_t _capture_flags(DvzRunnerCaptureKind kind)
{
    switch (kind)
    {
    case DVZ_RUNNER_CAPTURE_VIDEO:
        return DVZ_APP_CAPTURE_VIDEO;
    case DVZ_RUNNER_CAPTURE_PNG:
        return DVZ_APP_CAPTURE_PNG;
    case DVZ_RUNNER_CAPTURE_DVZR:
        return DVZ_APP_CAPTURE_DVZR;
    case DVZ_RUNNER_CAPTURE_NONE:
    default:
        return DVZ_APP_CAPTURE_NONE;
    }
}



static const char* _presentation_label(DvzRunnerPresentation presentation)
{
    switch (presentation)
    {
    case DVZ_RUNNER_PRESENT_GLFW:
        return "glfw";
    case DVZ_RUNNER_PRESENT_OFFSCREEN:
        return "offscreen";
    case DVZ_RUNNER_PRESENT_BROWSER:
        return "browser";
    default:
        return "unknown";
    }
}



static const char* _capture_label(DvzRunnerCaptureKind kind)
{
    switch (kind)
    {
    case DVZ_RUNNER_CAPTURE_VIDEO:
        return "video";
    case DVZ_RUNNER_CAPTURE_PNG:
        return "PNG";
    case DVZ_RUNNER_CAPTURE_DVZR:
        return "DVZR";
    case DVZ_RUNNER_CAPTURE_NONE:
    default:
        return "capture";
    }
}



static uint64_t _known_requirement_mask(void)
{
    uint64_t mask = 0;
    for (uint32_t i = 0; i < REQ_NAME_COUNT; i++)
        mask |= REQ_NAMES[i].bit;
    return mask;
}



static uint64_t _effective_requirements(
    const DvzScenarioSpec* spec, const DvzRunnerConfig* config)
{
    uint64_t requirements = spec != NULL ? spec->requirements : 0;
    if (spec != NULL && spec->frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->post_frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->native_view != NULL)
        requirements |= DVZ_SCENARIO_REQ_NATIVE_VIEW;
    if (config != NULL && config->capture_kind != DVZ_RUNNER_CAPTURE_NONE)
        requirements |= DVZ_SCENARIO_REQ_NATIVE_CAPTURE;
    return requirements;
}



static void _print_requirements(FILE* stream, uint64_t requirements)
{
    ANN(stream);

    bool first = true;
    for (uint32_t i = 0; i < REQ_NAME_COUNT; i++)
    {
        if ((requirements & REQ_NAMES[i].bit) == 0)
            continue;
        fprintf(stream, "%s%s", first ? "" : ",", REQ_NAMES[i].name);
        first = false;
    }
    if (first)
        fprintf(stream, "none");
}



static int _validate_requirements(const DvzScenarioSpec* spec, const DvzRunnerConfig* config)
{
    ANN(spec);
    ANN(config);

    if (config->presentation == DVZ_RUNNER_PRESENT_BROWSER)
    {
        fprintf(stderr,
                "scenario_runner: native runner cannot use presentation '%s' for scenario '%s'\n",
                _presentation_label(config->presentation), spec->id != NULL ? spec->id : "?");
        return -1;
    }

    const uint64_t requirements = _effective_requirements(spec, config);
    const uint64_t unknown = requirements & ~_known_requirement_mask();
    if (unknown != 0)
    {
        fprintf(stderr, "scenario_runner: scenario '%s' has unknown requirement bits 0x%llx\n",
                spec->id != NULL ? spec->id : "?", (unsigned long long)unknown);
        return -1;
    }

    if (requirements != 0)
    {
        fprintf(stdout, "scenario_runner: %s requirements: ", spec->id != NULL ? spec->id : "?");
        _print_requirements(stdout, requirements);
        fprintf(stdout, "\n");
    }
    return 0;
}



static void _print_progress(uint32_t completed, uint32_t total)
{
    if (total == 0)
        return;

    const uint32_t filled = (uint32_t)(((uint64_t)completed * RUNNER_PROGRESS_WIDTH) / total);
    fprintf(stdout, "\rscenario_runner: [");
    for (uint32_t i = 0; i < RUNNER_PROGRESS_WIDTH; i++)
        fputc(i < filled ? '#' : '.', stdout);
    fprintf(stdout, "] %u/%u frames", completed, total);
    fflush(stdout);
}



static DvzScenarioPointerType _scenario_pointer_type(DvzPointerEventType type)
{
    switch (type)
    {
    case DVZ_POINTER_EVENT_RELEASE:
        return DVZ_SCENARIO_POINTER_RELEASE;
    case DVZ_POINTER_EVENT_PRESS:
        return DVZ_SCENARIO_POINTER_PRESS;
    case DVZ_POINTER_EVENT_MOVE:
        return DVZ_SCENARIO_POINTER_MOVE;
    case DVZ_POINTER_EVENT_CLICK:
        return DVZ_SCENARIO_POINTER_CLICK;
    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        return DVZ_SCENARIO_POINTER_DOUBLE_CLICK;
    case DVZ_POINTER_EVENT_DRAG_START:
        return DVZ_SCENARIO_POINTER_DRAG_START;
    case DVZ_POINTER_EVENT_DRAG:
        return DVZ_SCENARIO_POINTER_DRAG;
    case DVZ_POINTER_EVENT_DRAG_STOP:
        return DVZ_SCENARIO_POINTER_DRAG_STOP;
    case DVZ_POINTER_EVENT_WHEEL:
        return DVZ_SCENARIO_POINTER_WHEEL;
    case DVZ_POINTER_EVENT_NONE:
    case DVZ_POINTER_EVENT_ALL:
    default:
        return DVZ_SCENARIO_POINTER_NONE;
    }
}



static void _timer_callback(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    RunnerFrameState* state = (RunnerFrameState*)user_data;
    if (state == NULL || state->spec == NULL || state->ctx == NULL || state->spec->frame == NULL)
        return;

    state->ctx->time = t;
    state->ctx->dt = dt;
    state->spec->frame(state->ctx, state->user);
    state->ctx->frame_index++;
}



static void _runner_event(DvzInputRouter* router, const DvzInputEvent* input, void* user_data)
{
    (void)router;
    RunnerEventState* state = (RunnerEventState*)user_data;
    if (
        state == NULL || state->spec == NULL || state->ctx == NULL ||
        state->spec->event == NULL || input == NULL)
        return;

    DvzScenarioEvent event = {0};
    switch (input->type)
    {
    case DVZ_INPUT_EVENT_POINTER:
    {
        const DvzPointerEvent* pointer = &input->content.pointer;
        event.kind = DVZ_SCENARIO_EVENT_POINTER;
        event.content.pointer.type = _scenario_pointer_type(pointer->type);
        event.content.pointer.x = pointer->pos[0];
        event.content.pointer.y = pointer->pos[1];
        event.content.pointer.dx = pointer->type == DVZ_POINTER_EVENT_WHEEL
                                       ? pointer->content.w.dir[0]
                                       : 0.0f;
        event.content.pointer.dy = pointer->type == DVZ_POINTER_EVENT_WHEEL
                                       ? pointer->content.w.dir[1]
                                       : 0.0f;
        event.content.pointer.content_scale = pointer->content_scale;
        event.content.pointer.button = (uint32_t)pointer->button;
        event.content.pointer.modifiers = pointer->mods >= 0 ? (uint32_t)pointer->mods : 0;
        event.content.pointer.timestamp_ns = pointer->timestamp_ns;
        break;
    }
    case DVZ_INPUT_EVENT_KEYBOARD:
    {
        const DvzKeyboardEvent* key = &input->content.keyboard;
        event.kind = DVZ_SCENARIO_EVENT_KEY;
        event.content.key.type = (uint32_t)key->type;
        event.content.key.key = (uint32_t)key->key;
        event.content.key.modifiers = key->mods >= 0 ? (uint32_t)key->mods : 0;
        break;
    }
    case DVZ_INPUT_EVENT_RESIZE:
    {
        const DvzInputResizeEvent* resize = &input->content.resize;
        event.kind = DVZ_SCENARIO_EVENT_RESIZE;
        event.content.resize.framebuffer_width = resize->framebuffer_width;
        event.content.resize.framebuffer_height = resize->framebuffer_height;
        event.content.resize.window_width = resize->window_width;
        event.content.resize.window_height = resize->window_height;
        event.content.resize.content_scale_x = resize->content_scale_x;
        event.content.resize.content_scale_y = resize->content_scale_y;
        break;
    }
    case DVZ_INPUT_EVENT_SCALE:
    case DVZ_INPUT_EVENT_NONE:
    default:
        return;
    }

    state->spec->event(state->ctx, &event, state->user);
}



static void _view_frame(DvzView* view, void* user_data)
{
    (void)view;
    RunnerViewFrameState* state = (RunnerViewFrameState*)user_data;
    if (state == NULL)
        return;

    if (state->spec != NULL && state->spec->post_frame != NULL && state->ctx != NULL)
        state->spec->post_frame(state->ctx, state->user);

    RunnerProgressState* progress = &state->progress;
    if (progress->frame_count != 0)
    {
        if (progress->rendered < progress->frame_count)
            progress->rendered++;
        _print_progress(progress->rendered, progress->frame_count);
    }
}



static void _run_paced(DvzApp* app, uint32_t frame_count, double fps)
{
    ANN(app);
    if (frame_count == 0)
    {
        dvz_app_run(app, 0);
        return;
    }
    if (fps <= 0)
    {
        dvz_app_run(app, frame_count);
        return;
    }

    const uint64_t period_ns = (uint64_t)((double)RUNNER_NS_PER_SEC / fps);
    uint64_t next_ns = dvz_time_monotonic_ns();
    for (uint32_t frame = 0; frame < frame_count; frame++)
    {
        const uint64_t now = dvz_time_monotonic_ns();
        if (next_ns > now)
        {
            const uint64_t sleep_ns = next_ns - now;
            const uint64_t sleep_us = sleep_ns / 1000u;
            if (sleep_us > 0)
            {
                const int bounded_us =
                    sleep_us > (uint64_t)INT32_MAX ? INT32_MAX : (int)sleep_us;
                dvz_sleep_us(bounded_us);
            }
        }

        dvz_app_run(app, 1);

        const uint64_t after = dvz_time_monotonic_ns();
        next_ns += period_ns;
        if (next_ns + period_ns < after)
            next_ns = after + period_ns;
    }
}



static int _connect_controller_bindings(DvzScenarioContext* ctx, DvzView* view)
{
    if (ctx == NULL || view == NULL)
        return -1;
    if (ctx->controller_binding_count == 0)
        return 0;

    DvzInputRouter* router = dvz_view_input(view);
    if (router == NULL)
        return -1;

    for (uint32_t i = 0; i < ctx->controller_binding_count; i++)
    {
        DvzPanel* panel = ctx->controller_bindings[i].panel;
        if (panel == NULL)
            return -1;
        if (dvz_panel_connect_input(panel, router) != 0)
            return -1;
    }
    return 0;
}



static void _print_usage(const DvzScenarioSpec* spec)
{
    const char* exe = spec != NULL && spec->id != NULL ? spec->id : "scenario";
    fprintf(stdout, "usage: %s [mode] [frames]\n", exe);
    fprintf(stdout, "modes:\n");
    fprintf(stdout, "  --live                 show a paced GLFW window (default)\n");
    fprintf(stdout, "  --live-record N        show GLFW and record paced offscreen video\n");
    fprintf(stdout, "  --offscreen-record N   record unpaced offscreen video\n");
    fprintf(stdout, "  --png                  write one offscreen PNG\n");
    fprintf(stdout, "  --dvzr N               record a DVZR stream offscreen\n");
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRunnerConfig dvz_runner_config(const DvzScenarioSpec* spec)
{
    const char* basename = spec != NULL && spec->id != NULL ? spec->id : "scenario";
    DvzRunnerConfig config = {
        .presentation = DVZ_RUNNER_PRESENT_GLFW,
        .capture_kind = DVZ_RUNNER_CAPTURE_NONE,
        .width = spec != NULL ? spec->width : 0,
        .height = spec != NULL ? spec->height : 0,
        .frame_count = 0,
        .fps = spec != NULL && spec->fps > 0 ? spec->fps : 60.0,
        .capture = dvz_app_capture_config_from_env(basename),
        .print_progress = true,
        .pace_wall_time = false,
    };
    config.capture.flags = DVZ_APP_CAPTURE_NONE;
    return config;
}



bool dvz_runner_capture_path(
    const DvzAppCaptureConfig* capture, DvzRunnerCaptureKind kind, char* out, size_t out_size,
    bool display)
{
    if (capture == NULL || out == NULL || out_size == 0)
        return false;

    const char* extension = _capture_extension(kind);
    if (extension == NULL)
        return false;

    const char* directory =
        capture->directory != NULL && capture->directory[0] != '\0' ? capture->directory : ".";
    const char* basename =
        capture->basename != NULL && capture->basename[0] != '\0' ? capture->basename : "capture";

    int rc = 0;
    const size_t dir_len = strlen(directory);
    if (strcmp(directory, ".") == 0)
        rc = dvz_snprintf(out, out_size, "%s%s%s", display ? "./" : "", basename, extension);
    else if (dir_len > 0 && directory[dir_len - 1] == '/')
        rc = dvz_snprintf(out, out_size, "%s%s%s", directory, basename, extension);
    else
        rc = dvz_snprintf(out, out_size, "%s/%s%s", directory, basename, extension);

    return rc >= 0 && (size_t)rc < out_size;
}



int dvz_scenario_bind_controller(
    DvzScenarioContext* ctx, DvzPanel* panel, DvzController* controller, DvzDimMask dims)
{
    if (ctx == NULL || panel == NULL || controller == NULL)
        return -1;

    for (uint32_t i = 0; i < ctx->controller_binding_count; i++)
    {
        DvzScenarioControllerBinding* binding = &ctx->controller_bindings[i];
        if (binding->panel == panel && binding->controller == controller && binding->dims == dims)
            return dvz_panel_bind_controller(panel, controller, dims);
    }

    if (ctx->controller_binding_count >= DVZ_SCENARIO_MAX_CONTROLLER_BINDINGS)
        return -1;
    if (dvz_panel_bind_controller(panel, controller, dims) != 0)
        return -1;

    DvzScenarioControllerBinding* binding =
        &ctx->controller_bindings[ctx->controller_binding_count++];
    binding->panel = panel;
    binding->controller = controller;
    binding->dims = dims;
    return 0;
}



DvzPanzoom* dvz_scenario_panzoom(
    DvzScenarioContext* ctx, DvzPanel* panel, const DvzPanzoomDesc* desc, DvzDimMask dims)
{
    if (ctx == NULL || ctx->scene == NULL || panel == NULL)
        return NULL;

    DvzController* controller = dvz_panzoom(ctx->scene, desc);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    if (panzoom == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, dims) != 0)
        return NULL;
    return panzoom;
}



bool dvz_scenario_panel_pointer_position(
    const DvzPanel* panel, const DvzScenarioPointerEvent* event, double* out_x, double* out_y)
{
    if (panel == NULL || event == NULL || out_x == NULL || out_y == NULL)
        return false;

    DvzRect rect = {0};
    if (!dvz_panel_inner_rect_px(panel, &rect) || rect.width <= 0.0f || rect.height <= 0.0f)
        return false;

    float x = event->x;
    float y = event->y;

    x -= rect.x;
    y -= rect.y;
    if (x < 0.0f || x >= rect.width || y < 0.0f || y >= rect.height)
        return false;

    *out_x = (double)x;
    *out_y = (double)y;
    return true;
}



int dvz_scenario_panel_query(
    DvzPanel* panel, double x, double y, const DvzQueryRequest* request)
{
    if (panel == NULL || request == NULL)
        return -1;
    return dvz_panel_query(panel, x, y, request);
}



int dvz_scenario_run_native(const DvzScenarioSpec* spec, const DvzRunnerConfig* config)
{
    if (spec == NULL || spec->init == NULL || config == NULL)
        return -1;

    int ret = -1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* view = NULL;
    DvzView* capture_view = NULL;
    DvzAnimation* timer = NULL;
    bool capture_started = false;
    void* user = NULL;
    RunnerFrameState frame_state = {0};
    RunnerEventState event_state = {0};
    RunnerViewFrameState view_frame_state = {0};
    DvzRunnerConfig resolved = *config;
    DvzScenarioContext ctx = {0};

    if (resolved.width == 0)
        resolved.width = spec->width;
    if (resolved.height == 0)
        resolved.height = spec->height;
    if (resolved.fps <= 0)
        resolved.fps = spec->fps > 0 ? spec->fps : 60.0;
    resolved.capture.flags = _capture_flags(resolved.capture_kind);
    if (resolved.capture.fps <= 0)
        resolved.capture.fps = resolved.fps;
    if (_validate_requirements(spec, &resolved) != 0)
        goto cleanup;

    ctx.width = resolved.width;
    ctx.height = resolved.height;

    scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "scenario_runner: dvz_scene() failed\n");
        goto cleanup;
    }
    ctx.scene = scene;
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, resolved.fps);

    if (!spec->init(&ctx, &user) || ctx.figure == NULL)
    {
        fprintf(stderr, "scenario_runner: scenario init failed\n");
        goto cleanup;
    }

    if (spec->frame != NULL)
    {
        frame_state.spec = spec;
        frame_state.ctx = &ctx;
        frame_state.user = user;
        timer = dvz_anim_timer(scene, 0.0, _timer_callback, &frame_state);
        if (timer == NULL)
        {
            fprintf(stderr, "scenario_runner: dvz_anim_timer() failed\n");
            goto cleanup;
        }
        dvz_anim_start(timer, 0.0);
    }

    DvzAppConfig app_config = dvz_app_config();
    if (resolved.presentation == DVZ_RUNNER_PRESENT_GLFW)
        app_config.fps_cap = resolved.fps;
    app = dvz_app_with_config(scene, &app_config);
    if (app == NULL)
    {
        fprintf(stderr, "scenario_runner: dvz_app() failed\n");
        goto cleanup;
    }

    if (resolved.presentation == DVZ_RUNNER_PRESENT_GLFW)
    {
        const char* title = spec->title != NULL ? spec->title : spec->id;
        view = dvz_view_glfw(app, ctx.figure, ctx.width, ctx.height, title);
    }
    else
    {
        view = dvz_view_offscreen(app, ctx.figure, ctx.width, ctx.height);
    }
    if (view == NULL)
    {
        fprintf(stderr, "scenario_runner: view creation failed\n");
        goto cleanup;
    }

    if (resolved.presentation == DVZ_RUNNER_PRESENT_GLFW &&
        _connect_controller_bindings(&ctx, view) != 0)
    {
        fprintf(stderr, "scenario_runner: controller input connection failed\n");
        goto cleanup;
    }

    if (spec->event != NULL)
    {
        DvzInputRouter* router = dvz_view_input(view);
        if (router == NULL)
        {
            fprintf(stderr, "scenario_runner: event bridge requires view input\n");
            goto cleanup;
        }
        event_state.spec = spec;
        event_state.ctx = &ctx;
        event_state.user = user;
        dvz_input_subscribe_event(router, _runner_event, &event_state);
    }

    if (spec->native_view != NULL && !spec->native_view(&ctx, app, view, user))
    {
        fprintf(stderr, "scenario_runner: native view setup failed\n");
        goto cleanup;
    }

    capture_view = view;
    if (resolved.presentation == DVZ_RUNNER_PRESENT_GLFW &&
        resolved.capture_kind == DVZ_RUNNER_CAPTURE_VIDEO)
    {
        capture_view = dvz_view_offscreen(app, ctx.figure, ctx.width, ctx.height);
        if (capture_view == NULL)
        {
            fprintf(stderr, "scenario_runner: offscreen capture view creation failed\n");
            goto cleanup;
        }
        fprintf(stdout, "scenario_runner: showing GLFW view and recording offscreen view\n");
    }

    if (resolved.frame_count > 0 && resolved.print_progress)
    {
        view_frame_state.progress.frame_count = resolved.frame_count;
    }
    if (spec->post_frame != NULL || view_frame_state.progress.frame_count != 0)
    {
        view_frame_state.spec = spec;
        view_frame_state.ctx = &ctx;
        view_frame_state.user = user;
        dvz_view_set_frame_callback(capture_view, _view_frame, &view_frame_state);
    }

    if (resolved.capture_kind != DVZ_RUNNER_CAPTURE_NONE)
    {
        char path[RUNNER_PATH_SIZE] = {0};
        if (!dvz_runner_capture_path(
                &resolved.capture, resolved.capture_kind, path, sizeof(path), true))
        {
            fprintf(stderr, "scenario_runner: capture path is too long\n");
            goto cleanup;
        }
        fprintf(stdout, "scenario_runner: %s output path: %s\n",
                _capture_label(resolved.capture_kind), path);

        if (dvz_view_capture_start(capture_view, &resolved.capture) != 0)
        {
            fprintf(stderr, "scenario_runner: failed to start %s capture: %s\n",
                    _capture_label(resolved.capture_kind), path);
            goto cleanup;
        }
        capture_started = true;
    }

    if (resolved.frame_count > 0 && resolved.print_progress)
        _print_progress(0, resolved.frame_count);
    if (resolved.pace_wall_time)
        _run_paced(app, resolved.frame_count, resolved.fps);
    else
        dvz_app_run(app, resolved.frame_count);
    if (resolved.frame_count > 0 && resolved.print_progress)
        fprintf(stdout, "\n");

    if (capture_started)
    {
        if (dvz_view_capture_stop(capture_view) != 0)
        {
            fprintf(stderr, "scenario_runner: failed to stop capture\n");
            capture_started = false;
            goto cleanup;
        }
        capture_started = false;
    }
    ret = 0;

cleanup:
    if (capture_started && capture_view != NULL)
        (void)dvz_view_capture_stop(capture_view);
    if (timer != NULL)
        dvz_anim_stop(timer);
    if (spec != NULL && spec->destroy != NULL)
        spec->destroy(&ctx, user);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}



int dvz_scenario_run_native_cli(const DvzScenarioSpec* spec, int argc, char** argv)
{
    DvzRunnerConfig config = dvz_runner_config(spec);

    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if (arg == NULL)
            continue;

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            _print_usage(spec);
            return 0;
        }
        else if (strcmp(arg, "--live") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_GLFW;
            config.capture_kind = DVZ_RUNNER_CAPTURE_NONE;
            config.frame_count = 0;
        }
        else if (strcmp(arg, "--live-record") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_GLFW;
            config.capture_kind = DVZ_RUNNER_CAPTURE_VIDEO;
            config.frame_count = _frames_after(argc, argv, i, RUNNER_DEFAULT_FRAMES);
            config.pace_wall_time = true;
        }
        else if (strcmp(arg, "--offscreen-record") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_OFFSCREEN;
            config.capture_kind = DVZ_RUNNER_CAPTURE_VIDEO;
            config.frame_count = _frames_after(argc, argv, i, RUNNER_DEFAULT_FRAMES);
        }
        else if (strcmp(arg, "--png") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_OFFSCREEN;
            config.capture_kind = DVZ_RUNNER_CAPTURE_PNG;
            config.frame_count = 1;
        }
        else if (strcmp(arg, "--dvzr") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_OFFSCREEN;
            config.capture_kind = DVZ_RUNNER_CAPTURE_DVZR;
            config.frame_count = _frames_after(argc, argv, i, RUNNER_DEFAULT_FRAMES);
        }
        else
        {
            uint32_t frames = 0;
            if (_parse_u32(arg, &frames))
                config.frame_count = frames;
            else
            {
                fprintf(stderr, "scenario_runner: unknown argument '%s'\n", arg);
                _print_usage(spec);
                return -1;
            }
        }
    }

    if (config.presentation == DVZ_RUNNER_PRESENT_GLFW && config.frame_count > 0)
        config.pace_wall_time = true;

    return dvz_scenario_run_native(spec, &config);
}
