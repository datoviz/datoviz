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

#include <cglm/affine.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define RUNNER_DEFAULT_FRAMES 120u
#define RUNNER_BENCHMARK_DEFAULT_FRAMES 600u
#define RUNNER_BENCHMARK_MAX_WARMUP     200u
#define RUNNER_PATH_SIZE      1024u
#define RUNNER_PROGRESS_WIDTH 32u
#define RUNNER_NS_PER_SEC     1000000000ull
#define RUNNER_SEQUENCE_CAPTURE_MAX_ATTEMPTS 4u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct RunnerFrameState
{
    const DvzScenarioSpec* spec;
    const DvzRunnerConfig* config;
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
    {DVZ_SCENARIO_REQ_PIXEL_VISUAL, "pixel"},
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
    {DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES, "continuous-frames"},
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



static bool _parse_size(const char* text, uint32_t* out_width, uint32_t* out_height)
{
    if (text == NULL || out_width == NULL || out_height == NULL)
        return false;

    const char* sep = strchr(text, 'x');
    if (sep == NULL)
        sep = strchr(text, 'X');
    if (sep == NULL || sep == text || sep[1] == '\0')
        return false;

    char left[32] = {0};
    const size_t n = (size_t)(sep - text);
    if (n >= sizeof(left))
        return false;
    dvz_memcpy(left, sizeof(left), text, n);

    uint32_t width = 0;
    uint32_t height = 0;
    if (!_parse_u32(left, &width) || !_parse_u32(sep + 1, &height))
        return false;
    if (width == 0 || height == 0)
        return false;

    *out_width = width;
    *out_height = height;
    return true;
}



static bool _parse_float(const char* text, float* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;

    char* end = NULL;
    float value = strtof(text, &end);
    if (end == text || (end != NULL && *end != '\0') || value <= 0.0f)
        return false;

    *out = value;
    return true;
}



static bool _parse_axis(const char* text, float out[3])
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (strcmp(text, "x") == 0 || strcmp(text, "X") == 0)
        out[0] = 1.0f;
    else if (strcmp(text, "y") == 0 || strcmp(text, "Y") == 0)
        out[1] = 1.0f;
    else if (strcmp(text, "z") == 0 || strcmp(text, "Z") == 0)
        out[2] = 1.0f;
    else
        return false;
    return true;
}



static bool _parse_preview_phase_policy(const char* text, DvzScenarioPreviewPhasePolicy* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;
    if (strcmp(text, "seamless-loop") == 0)
        *out = DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP;
    else if (strcmp(text, "endpoint") == 0)
        *out = DVZ_SCENARIO_PREVIEW_PHASE_ENDPOINT;
    else
        return false;
    return true;
}



static char* _runner_strtok(char* str, const char* delimiters, char** context)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return strtok_s(str, delimiters, context);
#else
    return strtok_r(str, delimiters, context);
#endif
}



static bool _parse_preview_timeline(const char* text, DvzScenarioPreviewTimeline* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;

    DvzScenarioPreviewTimeline timeline = {0};
    char buffer[RUNNER_PATH_SIZE] = {0};
    const size_t len = strlen(text);
    if (len >= sizeof(buffer))
        return false;
    memcpy(buffer, text, len + 1);

    char* save_segment = NULL;
    char* segment = _runner_strtok(buffer, ",", &save_segment);
    while (segment != NULL)
    {
        if (timeline.segment_count >= DVZ_SCENARIO_MAX_PREVIEW_SEGMENTS)
            return false;

        char* id = segment;
        char* kind = strchr(id, ':');
        if (kind == NULL)
            return false;
        *kind++ = '\0';
        char* frames_text = strchr(kind, ':');
        if (frames_text == NULL)
            return false;
        *frames_text++ = '\0';

        uint32_t frames = 0;
        if (id[0] == '\0' || kind[0] == '\0' || !_parse_u32(frames_text, &frames) || frames == 0)
            return false;

        DvzScenarioPreviewSegment* item = &timeline.segments[timeline.segment_count++];
        snprintf(item->id, sizeof(item->id), "%s", id);
        snprintf(item->kind, sizeof(item->kind), "%s", kind);
        item->frames = frames;

        segment = _runner_strtok(NULL, ",", &save_segment);
    }
    if (timeline.segment_count == 0)
        return false;

    *out = timeline;
    return true;
}



static bool _resolve_preview_timeline(DvzRunnerConfig* config)
{
    if (config == NULL || config->preview_timeline.segment_count == 0)
        return true;

    uint32_t frame_total = 0;
    for (uint32_t i = 0; i < config->preview_timeline.segment_count; i++)
    {
        DvzScenarioPreviewSegment* item = &config->preview_timeline.segments[i];
        if (item->frames == 0)
            return false;
        item->start_frame = frame_total;
        if (UINT32_MAX - frame_total < item->frames)
            return false;
        frame_total += item->frames;
    }

    if (config->preview_frame_count == 0)
        config->preview_frame_count = frame_total;
    return config->preview_frame_count == frame_total;
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
    if (spec != NULL && spec->continuous_frames)
        requirements |= DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES;
    if (spec != NULL && spec->frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->post_frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->native_view != NULL)
        requirements |= DVZ_SCENARIO_REQ_NATIVE_VIEW;
    if (config != NULL && config->preview_motion != DVZ_SCENARIO_MOTION_NONE)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
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


static bool _runner_png_sequence_path(
    const DvzAppCaptureConfig* capture, uint32_t frame, char* out, size_t out_size, bool display);



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



static bool _preview_motion_active(const DvzRunnerConfig* config)
{
    return config != NULL && config->preview_motion != DVZ_SCENARIO_MOTION_NONE;
}



static DvzVisual*
_resolve_visual_target(const DvzScenarioContext* ctx, const char* target)
{
    if (ctx == NULL)
        return NULL;

    if (target != NULL && target[0] != '\0')
    {
        if (strcmp(target, "primary") == 0 && ctx->primary_visual != NULL)
            return ctx->primary_visual;
        for (uint32_t i = 0; i < ctx->visual_target_count; i++)
        {
            const DvzScenarioVisualTarget* item = &ctx->visual_targets[i];
            if (item->name != NULL && strcmp(item->name, target) == 0)
                return item->visual;
        }
        return NULL;
    }

    if (ctx->primary_visual != NULL)
        return ctx->primary_visual;
    if (ctx->visual_target_count == 1)
        return ctx->visual_targets[0].visual;
    return NULL;
}



static bool
_preview_motion_resolves(const DvzScenarioContext* ctx, const DvzRunnerConfig* config)
{
    if (!_preview_motion_active(config))
        return true;

    switch (config->preview_motion)
    {
    case DVZ_SCENARIO_MOTION_VISUAL_SPIN:
        return _resolve_visual_target(ctx, config->preview_motion_target) != NULL;

    case DVZ_SCENARIO_MOTION_NONE:
    default:
        return true;
    }
}



static void _apply_preview_motion(DvzScenarioContext* ctx, const DvzRunnerConfig* config)
{
    if (ctx == NULL || !ctx->preview_mode || !_preview_motion_active(config))
        return;

    switch (config->preview_motion)
    {
    case DVZ_SCENARIO_MOTION_VISUAL_SPIN:
    {
        DvzVisual* visual = _resolve_visual_target(ctx, config->preview_motion_target);
        if (visual == NULL)
            return;
        const double phase = dvz_scenario_preview_cycles(
            ctx, config->preview_motion_cycles, config->preview_motion_phase_policy);
        mat4 transform = GLM_MAT4_IDENTITY_INIT;
        vec3 axis = {
            config->preview_motion_axis[0],
            config->preview_motion_axis[1],
            config->preview_motion_axis[2],
        };
        glm_rotate_make(transform, (float)(6.283185307179586 * phase), axis);
        (void)dvz_visual_set_transform(visual, transform);
        return;
    }

    case DVZ_SCENARIO_MOTION_NONE:
    default:
        return;
    }
}



static void _update_preview_timeline(DvzScenarioContext* ctx, const DvzRunnerConfig* config)
{
    if (ctx == NULL)
        return;

    ctx->preview_segment_id = NULL;
    ctx->preview_segment_kind = NULL;
    ctx->preview_segment_index = 0;
    ctx->preview_segment_count = 0;
    ctx->preview_segment_frame_index = 0;
    ctx->preview_segment_frame_count = 0;
    ctx->preview_segment_phase = 0.0;
    ctx->preview_global_phase =
        dvz_scenario_preview_phase(ctx, DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP);

    if (
        config == NULL || !ctx->preview_mode ||
        config->preview_timeline.segment_count == 0 || ctx->preview_frame_count == 0)
        return;

    const uint64_t stride = ctx->preview_sample_stride > 0 ? ctx->preview_sample_stride : 1;
    const uint64_t frame = (ctx->preview_frame_index / stride) % ctx->preview_frame_count;
    ctx->preview_segment_count = config->preview_timeline.segment_count;

    for (uint32_t i = 0; i < config->preview_timeline.segment_count; i++)
    {
        const DvzScenarioPreviewSegment* item = &config->preview_timeline.segments[i];
        const uint64_t start = item->start_frame;
        const uint64_t end = start + item->frames;
        if (frame < start || frame >= end)
            continue;

        const uint64_t local = frame - start;
        ctx->preview_segment_id = item->id;
        ctx->preview_segment_kind = item->kind;
        ctx->preview_segment_index = i;
        ctx->preview_segment_frame_index = local;
        ctx->preview_segment_frame_count = item->frames;
        ctx->preview_segment_phase = item->frames > 0 ? (double)local / (double)item->frames : 0.0;
        return;
    }
}



static void
_timer_callback(DvzAnimation* animation, double t, double dt, uint64_t tick, void* user_data)
{
    (void)animation;
    (void)tick;
    RunnerFrameState* state = (RunnerFrameState*)user_data;
    if (state == NULL || state->spec == NULL || state->ctx == NULL)
        return;

    state->ctx->time = t;
    state->ctx->dt = dt;
    _update_preview_timeline(state->ctx, state->config);
    if (state->spec->frame != NULL)
        state->spec->frame(state->ctx, state->user);
    _apply_preview_motion(state->ctx, state->config);
    state->ctx->frame_index++;
}


/**
 * Convert one raw input pointer position to portable scenario coordinates.
 *
 * @param router input router carrying resize state
 * @param state runner event state
 * @param pointer raw input pointer event
 * @param out_x output figure-layout x coordinate
 * @param out_y output figure-layout y coordinate
 */
static void _runner_pointer_to_figure(
    const DvzInputRouter* router, const RunnerEventState* state, const DvzPointerEvent* pointer,
    float* out_x, float* out_y)
{
    ANN(pointer);
    ANN(out_x);
    ANN(out_y);

    *out_x = pointer->pos[0];
    *out_y = pointer->pos[1];
    if (state == NULL || state->ctx == NULL || state->ctx->figure == NULL)
        return;

    DvzInputResizeEvent resize = {0};
    const bool has_resize = router != NULL && dvz_input_router_last_resize(router, &resize);
    const bool has_event_window =
        isfinite(pointer->window_size[0]) && isfinite(pointer->window_size[1]) &&
        pointer->window_size[0] > 0.0f && pointer->window_size[1] > 0.0f;
    const float window_width =
        has_event_window ? pointer->window_size[0] :
        has_resize && resize.window_width > 0 ? (float)resize.window_width :
                                                (float)state->ctx->logical_width;
    const float window_height =
        has_event_window ? pointer->window_size[1] :
        has_resize && resize.window_height > 0 ? (float)resize.window_height :
                                                 (float)state->ctx->logical_height;
    const float content_scale_x =
        has_resize && resize.content_scale_x > 0.0f ? resize.content_scale_x :
                                                       pointer->content_scale;
    const float content_scale_y =
        has_resize && resize.content_scale_y > 0.0f ? resize.content_scale_y :
                                                       pointer->content_scale;
    (void)dvz_figure_window_to_layout(
        state->ctx->figure, pointer->pos[0], pointer->pos[1], window_width, window_height,
        content_scale_x, content_scale_y, out_x, out_y);
}



static void _runner_event(DvzInputRouter* router, const DvzInputEvent* input, void* user_data)
{
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
        _runner_pointer_to_figure(
            router, state, pointer, &event.content.pointer.x, &event.content.pointer.y);
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
    fprintf(stdout, "  --benchmark N          benchmark N unpaced GLFW frames after warmup\n");
    fprintf(stdout, "  --live-record N        show GLFW and record paced offscreen video\n");
    fprintf(stdout, "  --video N              record unpaced offscreen video\n");
    fprintf(stdout, "  --offscreen-record N   record unpaced offscreen video\n");
    fprintf(stdout, "  --png                  write one offscreen PNG\n");
    fprintf(stdout, "  --dvzr N               record a DVZR stream offscreen\n");
    fprintf(stdout, "  --preview              enable deterministic gallery preview motion\n");
    fprintf(stdout, "  --preview-frame N      preview frame index for one-frame captures\n");
    fprintf(stdout, "  --preview-sequence     capture preview frames from one persistent run\n");
    fprintf(stdout, "  --preview-frames N     total preview frame count\n");
    fprintf(stdout, "  --preview-fps FPS      preview frame rate for deterministic frame timing\n");
    fprintf(stdout, "  --preview-sample-stride N internal preview frames per captured frame\n");
    fprintf(stdout, "  --preview-time-scale S multiply preview time without changing playback FPS\n");
    fprintf(stdout, "  --preview-timeline SPEC comma-separated preview segments id:kind:frames\n");
    fprintf(stdout, "  --preview-motion KIND  preview motion recipe: none, visual-spin\n");
    fprintf(stdout, "  --preview-motion-target NAME visual target name, default primary or sole target\n");
    fprintf(stdout, "  --preview-motion-axis x|y|z visual-spin axis\n");
    fprintf(stdout, "  --preview-motion-cycles N visual-spin cycles across preview\n");
    fprintf(stdout, "  --preview-motion-phase POLICY seamless-loop or endpoint\n");
    fprintf(stdout, "size and scale:\n");
    fprintf(stdout, "  --size WxH             exact framebuffer/output pixels\n");
    fprintf(stdout, "  --logical-size WxH     logical layout size in pixels\n");
    fprintf(stdout, "  --device-scale S       physical pixels per logical pixel\n");
    fprintf(stdout, "  --user-scale S         UI-like scene quantity scale\n");
    fprintf(stdout, "  --render-scale S       reserved render-quality scale\n");
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
        .logical_width = spec != NULL ? spec->width : 0,
        .logical_height = spec != NULL ? spec->height : 0,
        .framebuffer_width = spec != NULL ? spec->width : 0,
        .framebuffer_height = spec != NULL ? spec->height : 0,
        .device_scale = 1.0f,
        .user_scale = 1.0f,
        .render_scale = 1.0f,
        .width = spec != NULL ? spec->width : 0,
        .height = spec != NULL ? spec->height : 0,
        .frame_count = 0,
        .fps = spec != NULL && spec->fps > 0 ? spec->fps : 60.0,
        .capture = dvz_app_capture_config_from_env(basename),
        .print_progress = true,
        .pace_wall_time = false,
        .benchmark = false,
        .preview_mode = false,
        .preview_sequence = false,
        .preview_frame_index = 0,
        .preview_frame_count = 0,
        .preview_sample_stride = 0,
        .preview_fps = 0.0,
        .preview_time_scale = 0.0,
        .preview_motion = DVZ_SCENARIO_MOTION_NONE,
        .preview_motion_target = NULL,
        .preview_motion_axis = {0.0f, 1.0f, 0.0f},
        .preview_motion_cycles = 1.0,
        .preview_motion_phase_policy = DVZ_SCENARIO_PREVIEW_PHASE_ENDPOINT,
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


static bool _runner_png_sequence_path(
    const DvzAppCaptureConfig* capture, uint32_t frame, char* out, size_t out_size, bool display)
{
    if (capture == NULL || out == NULL || out_size == 0)
        return false;

    const char* directory =
        capture->directory != NULL && capture->directory[0] != '\0' ? capture->directory : ".";
    const char* basename =
        capture->basename != NULL && capture->basename[0] != '\0' ? capture->basename : "frame";

    int rc = 0;
    const size_t dir_len = strlen(directory);
    if (strcmp(directory, ".") == 0)
        rc = dvz_snprintf(out, out_size, "%s%s_%04u.png", display ? "./" : "", basename, frame);
    else if (dir_len > 0 && directory[dir_len - 1] == '/')
        rc = dvz_snprintf(out, out_size, "%s%s_%04u.png", directory, basename, frame);
    else
        rc = dvz_snprintf(out, out_size, "%s/%s_%04u.png", directory, basename, frame);

    return rc >= 0 && (size_t)rc < out_size;
}


static bool _runner_capture_png_sequence_frame(
    DvzApp* app, DvzView* view, const DvzAppCaptureConfig* capture, uint32_t frame)
{
    ANN(app);
    ANN(view);
    ANN(capture);

    char path[RUNNER_PATH_SIZE] = {0};
    if (!_runner_png_sequence_path(capture, frame, path, sizeof(path), false))
    {
        fprintf(stderr, "scenario_runner: PNG sequence path is too long\n");
        return false;
    }

    for (uint32_t attempt = 0; attempt < RUNNER_SEQUENCE_CAPTURE_MAX_ATTEMPTS; attempt++)
    {
        dvz_app_run(app, 1);
        if (dvz_view_capture_png(view, path) == 0)
        {
            dvz_fprintf(stdout, "datoviz: saved %s\n", path);
            return true;
        }
    }

    fprintf(stderr, "scenario_runner: failed to write PNG sequence frame: %s\n", path);
    return false;
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

    if (resolved.device_scale <= 0.0f)
        resolved.device_scale = 1.0f;
    if (resolved.user_scale <= 0.0f)
        resolved.user_scale = 1.0f;
    if (resolved.render_scale <= 0.0f)
        resolved.render_scale = 1.0f;
    if (resolved.logical_width == 0)
        resolved.logical_width = resolved.width != 0 ? resolved.width : spec->width;
    if (resolved.logical_height == 0)
        resolved.logical_height = resolved.height != 0 ? resolved.height : spec->height;
    if (resolved.framebuffer_width == 0 && resolved.logical_width > 0)
        resolved.framebuffer_width =
            (uint32_t)((float)resolved.logical_width * resolved.device_scale + 0.5f);
    if (resolved.framebuffer_height == 0 && resolved.logical_height > 0)
        resolved.framebuffer_height =
            (uint32_t)((float)resolved.logical_height * resolved.device_scale + 0.5f);
    if (resolved.logical_width == 0 && resolved.framebuffer_width > 0)
        resolved.logical_width =
            (uint32_t)((float)resolved.framebuffer_width / resolved.device_scale + 0.5f);
    if (resolved.logical_height == 0 && resolved.framebuffer_height > 0)
        resolved.logical_height =
            (uint32_t)((float)resolved.framebuffer_height / resolved.device_scale + 0.5f);
    resolved.width = resolved.logical_width;
    resolved.height = resolved.logical_height;
    if (resolved.fps <= 0)
        resolved.fps = spec->fps > 0 ? spec->fps : 60.0;
    if (resolved.preview_sequence)
    {
        resolved.preview_mode = true;
        if (resolved.capture_kind == DVZ_RUNNER_CAPTURE_NONE)
            resolved.capture_kind = DVZ_RUNNER_CAPTURE_PNG;
        if (resolved.capture_kind != DVZ_RUNNER_CAPTURE_PNG)
        {
            fprintf(stderr, "scenario_runner: --preview-sequence requires PNG capture\n");
            goto cleanup;
        }
        if (resolved.preview_frame_count == 0)
            resolved.preview_frame_count =
                resolved.frame_count != 0 ? resolved.frame_count : RUNNER_DEFAULT_FRAMES;
        resolved.frame_count = resolved.preview_frame_count;
    }
    if (resolved.preview_mode && resolved.preview_frame_count == 0)
        resolved.preview_frame_count = resolved.frame_count != 0 ? resolved.frame_count : 1;
    if (resolved.preview_mode && resolved.preview_fps <= 0.0)
        resolved.preview_fps = resolved.fps;
    if (resolved.preview_mode && resolved.preview_sample_stride == 0)
        resolved.preview_sample_stride = 1;
    if (resolved.preview_mode && resolved.preview_time_scale <= 0.0)
        resolved.preview_time_scale = 1.0;
    if (resolved.preview_mode && !_resolve_preview_timeline(&resolved))
    {
        fprintf(stderr, "scenario_runner: preview timeline must match preview frame count\n");
        goto cleanup;
    }
    resolved.capture.flags = _capture_flags(resolved.capture_kind);
    if (resolved.capture.fps <= 0)
        resolved.capture.fps = resolved.fps;
    if (_validate_requirements(spec, &resolved) != 0)
        goto cleanup;

    ctx.logical_width = resolved.logical_width;
    ctx.logical_height = resolved.logical_height;
    ctx.framebuffer_width = resolved.framebuffer_width;
    ctx.framebuffer_height = resolved.framebuffer_height;
    ctx.device_scale = resolved.device_scale;
    ctx.user_scale = resolved.user_scale;
    ctx.render_scale = resolved.render_scale;
    ctx.presentation = resolved.presentation;
    ctx.width = resolved.logical_width;
    ctx.height = resolved.logical_height;
    ctx.preview_mode = resolved.preview_mode;
    ctx.preview_frame_index = resolved.preview_frame_index;
    ctx.preview_frame_count = resolved.preview_frame_count;
    ctx.preview_sample_stride = resolved.preview_sample_stride;
    ctx.preview_fps = resolved.preview_fps;
    ctx.preview_time_scale = resolved.preview_time_scale;
    _update_preview_timeline(&ctx, &resolved);

    scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "scenario_runner: dvz_scene() failed\n");
        goto cleanup;
    }
    ctx.scene = scene;
    const bool deterministic_clock =
        resolved.presentation != DVZ_RUNNER_PRESENT_GLFW ||
        resolved.capture_kind != DVZ_RUNNER_CAPTURE_NONE;
    dvz_scene_set_clock_mode(
        scene, deterministic_clock ? DVZ_SCENE_CLOCK_FIXED_STEP : DVZ_SCENE_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, resolved.fps);

    if (!spec->init(&ctx, &user) || ctx.figure == NULL)
    {
        fprintf(stderr, "scenario_runner: scenario init failed\n");
        goto cleanup;
    }
    if (!_preview_motion_resolves(&ctx, &resolved))
    {
        fprintf(stderr, "scenario_runner: preview motion target could not be resolved\n");
        goto cleanup;
    }

    if (spec->frame != NULL || spec->continuous_frames || _preview_motion_active(&resolved))
    {
        frame_state.spec = spec;
        frame_state.config = &resolved;
        frame_state.ctx = &ctx;
        frame_state.user = user;
        DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
        timer_desc.callback = _timer_callback;
        timer_desc.user_data = &frame_state;
        timer = dvz_anim_timer(scene, &timer_desc);
        if (timer == NULL)
        {
            fprintf(stderr, "scenario_runner: dvz_anim_timer() failed\n");
            goto cleanup;
        }
        dvz_anim_start(timer, 0.0);
    }

    DvzAppConfig app_config = dvz_app_config();
    /* Scenario FPS controls scene, preview, and capture timing. Rendering remains uncapped unless
       DvzAppConfig, such as the DVZ_FPS_CAP override, requests a scheduler cap. */
    app = dvz_app_with_config(scene, &app_config);
    if (app == NULL)
    {
        fprintf(stderr, "scenario_runner: dvz_app() failed\n");
        goto cleanup;
    }

    if (resolved.presentation == DVZ_RUNNER_PRESENT_GLFW)
    {
        const char* title = spec->title != NULL ? spec->title : spec->id;
        DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_WINDOW);
        desc.size_policy = DVZ_VIEW_SIZE_HOST_LOGICAL_PX;
        desc.size_width = ctx.logical_width;
        desc.size_height = ctx.logical_height;
        desc.size_requested_device_scale = ctx.device_scale;
        desc.device_scale = ctx.device_scale;
        desc.user_scale = ctx.user_scale;
        desc.render_scale = ctx.render_scale;
        desc.title = title;
        view = dvz_view(app, ctx.figure, &desc);
    }
    else
    {
        DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
        desc.size_policy = DVZ_VIEW_SIZE_FRAMEBUFFER_PX;
        desc.size_width = ctx.framebuffer_width;
        desc.size_height = ctx.framebuffer_height;
        desc.size_requested_device_scale = ctx.device_scale;
        desc.device_scale = ctx.device_scale;
        desc.user_scale = ctx.user_scale;
        desc.render_scale = ctx.render_scale;
        view = dvz_view(app, ctx.figure, &desc);
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
        DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
        desc.size_policy = DVZ_VIEW_SIZE_FRAMEBUFFER_PX;
        desc.size_width = ctx.framebuffer_width;
        desc.size_height = ctx.framebuffer_height;
        desc.size_requested_device_scale = ctx.device_scale;
        desc.device_scale = ctx.device_scale;
        desc.user_scale = ctx.user_scale;
        desc.render_scale = ctx.render_scale;
        capture_view = dvz_view(app, ctx.figure, &desc);
        if (capture_view == NULL)
        {
            fprintf(stderr, "scenario_runner: offscreen capture view creation failed\n");
            goto cleanup;
        }
        fprintf(stdout, "scenario_runner: showing GLFW view and recording offscreen view\n");
    }

    if (!resolved.preview_sequence && resolved.frame_count > 0 && resolved.print_progress)
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

    if (resolved.capture_kind != DVZ_RUNNER_CAPTURE_NONE && !resolved.preview_sequence)
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
    else if (resolved.preview_sequence && resolved.capture_kind == DVZ_RUNNER_CAPTURE_PNG)
    {
        char path[RUNNER_PATH_SIZE] = {0};
        if (!_runner_png_sequence_path(&resolved.capture, 0, path, sizeof(path), true))
        {
            fprintf(stderr, "scenario_runner: PNG sequence path is too long\n");
            goto cleanup;
        }
        fprintf(stdout, "scenario_runner: PNG sequence output path: %s ...\n", path);
    }

    if (resolved.frame_count > 0 && resolved.print_progress)
        _print_progress(0, resolved.frame_count);
    if (resolved.benchmark)
    {
        uint32_t warmup_frames = resolved.frame_count / 10;
        if (warmup_frames == 0)
            warmup_frames = 1;
        if (warmup_frames > RUNNER_BENCHMARK_MAX_WARMUP)
            warmup_frames = RUNNER_BENCHMARK_MAX_WARMUP;
        if (warmup_frames > 0)
            dvz_app_run(app, warmup_frames);

        uint64_t start_ns = dvz_time_monotonic_ns();
        dvz_app_run(app, resolved.frame_count);
        uint64_t elapsed_ns = dvz_time_monotonic_ns() - start_ns;
        double elapsed_s = (double)elapsed_ns / (double)RUNNER_NS_PER_SEC;
        double fps = elapsed_s > 0.0 ? (double)resolved.frame_count / elapsed_s : 0.0;
        fprintf(
            stdout, "scenario_benchmark: scenario=%s frames=%u warmup=%u elapsed=%.6fs fps=%.2f\n",
            spec->id != NULL ? spec->id : "?", resolved.frame_count, warmup_frames, elapsed_s, fps);
    }
    else if (resolved.preview_sequence)
    {
        for (uint32_t frame = 0; frame < resolved.frame_count; frame++)
        {
            for (uint32_t sample = 0; sample + 1 < resolved.preview_sample_stride; sample++)
            {
                ctx.preview_frame_index =
                    (uint64_t)frame * (uint64_t)resolved.preview_sample_stride + sample;
                dvz_app_run(app, 1);
            }
            ctx.preview_frame_index =
                (uint64_t)frame * (uint64_t)resolved.preview_sample_stride +
                (uint64_t)resolved.preview_sample_stride - 1u;
            if (!_runner_capture_png_sequence_frame(app, capture_view, &resolved.capture, frame))
                goto cleanup;
            if (resolved.print_progress)
                _print_progress(frame + 1, resolved.frame_count);
        }
    }
    else if (resolved.pace_wall_time)
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
        else if (strcmp(arg, "--benchmark") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_GLFW;
            config.capture_kind = DVZ_RUNNER_CAPTURE_NONE;
            config.frame_count =
                _frames_after(argc, argv, i, RUNNER_BENCHMARK_DEFAULT_FRAMES);
            if (config.frame_count < 2)
            {
                fprintf(stderr, "scenario_runner: --benchmark requires at least two frames\n");
                return -1;
            }
            config.print_progress = false;
            config.pace_wall_time = false;
            config.benchmark = true;
        }
        else if (strcmp(arg, "--live-record") == 0)
        {
            config.presentation = DVZ_RUNNER_PRESENT_GLFW;
            config.capture_kind = DVZ_RUNNER_CAPTURE_VIDEO;
            config.frame_count = _frames_after(argc, argv, i, RUNNER_DEFAULT_FRAMES);
            config.pace_wall_time = true;
        }
        else if (strcmp(arg, "--offscreen-record") == 0 || strcmp(arg, "--video") == 0)
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
        else if (strcmp(arg, "--preview") == 0)
        {
            config.preview_mode = true;
        }
        else if (strcmp(arg, "--preview-sequence") == 0)
        {
            config.preview_mode = true;
            config.preview_sequence = true;
            config.presentation = DVZ_RUNNER_PRESENT_OFFSCREEN;
            config.capture_kind = DVZ_RUNNER_CAPTURE_PNG;
        }
        else if (strcmp(arg, "--preview-frame") == 0 && i + 1 < argc)
        {
            uint32_t frame = 0;
            if (!_parse_u32(argv[++i], &frame))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-frame value\n");
                return -1;
            }
            config.preview_frame_index = frame;
        }
        else if (strcmp(arg, "--preview-frames") == 0 && i + 1 < argc)
        {
            uint32_t frame_count = 0;
            if (!_parse_u32(argv[++i], &frame_count))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-frames value\n");
                return -1;
            }
            config.preview_frame_count = frame_count;
        }
        else if (strcmp(arg, "--preview-fps") == 0 && i + 1 < argc)
        {
            float fps = 0.0f;
            if (!_parse_float(argv[++i], &fps))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-fps value\n");
                return -1;
            }
            config.preview_fps = (double)fps;
        }
        else if (strcmp(arg, "--preview-sample-stride") == 0 && i + 1 < argc)
        {
            uint32_t stride = 0;
            if (!_parse_u32(argv[++i], &stride) || stride == 0)
            {
                fprintf(stderr, "scenario_runner: invalid --preview-sample-stride value\n");
                return -1;
            }
            config.preview_sample_stride = stride;
        }
        else if (strcmp(arg, "--preview-time-scale") == 0 && i + 1 < argc)
        {
            float scale = 0.0f;
            if (!_parse_float(argv[++i], &scale))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-time-scale value\n");
                return -1;
            }
            config.preview_time_scale = (double)scale;
        }
        else if (strcmp(arg, "--preview-timeline") == 0 && i + 1 < argc)
        {
            if (!_parse_preview_timeline(argv[++i], &config.preview_timeline))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-timeline value\n");
                return -1;
            }
            config.preview_mode = true;
        }
        else if (strcmp(arg, "--preview-motion") == 0 && i + 1 < argc)
        {
            const char* motion = argv[++i];
            if (strcmp(motion, "none") == 0)
                config.preview_motion = DVZ_SCENARIO_MOTION_NONE;
            else if (strcmp(motion, "visual-spin") == 0)
                config.preview_motion = DVZ_SCENARIO_MOTION_VISUAL_SPIN;
            else
            {
                fprintf(stderr, "scenario_runner: invalid --preview-motion value\n");
                return -1;
            }
        }
        else if (strcmp(arg, "--preview-motion-target") == 0 && i + 1 < argc)
        {
            config.preview_motion_target = argv[++i];
        }
        else if (strcmp(arg, "--preview-motion-axis") == 0 && i + 1 < argc)
        {
            if (!_parse_axis(argv[++i], config.preview_motion_axis))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-motion-axis value\n");
                return -1;
            }
        }
        else if (strcmp(arg, "--preview-motion-cycles") == 0 && i + 1 < argc)
        {
            float cycles = 0.0f;
            if (!_parse_float(argv[++i], &cycles))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-motion-cycles value\n");
                return -1;
            }
            config.preview_motion_cycles = (double)cycles;
        }
        else if (strcmp(arg, "--preview-motion-phase") == 0 && i + 1 < argc)
        {
            if (!_parse_preview_phase_policy(argv[++i], &config.preview_motion_phase_policy))
            {
                fprintf(stderr, "scenario_runner: invalid --preview-motion-phase value\n");
                return -1;
            }
        }
        else if (strcmp(arg, "--size") == 0 && i + 1 < argc)
        {
            uint32_t width = 0;
            uint32_t height = 0;
            if (!_parse_size(argv[++i], &width, &height))
            {
                fprintf(stderr, "scenario_runner: invalid --size value\n");
                return -1;
            }
            config.framebuffer_width = width;
            config.framebuffer_height = height;
            config.logical_width = width;
            config.logical_height = height;
            config.width = width;
            config.height = height;
            config.device_scale = 1.0f;
        }
        else if (strcmp(arg, "--logical-size") == 0 && i + 1 < argc)
        {
            uint32_t width = 0;
            uint32_t height = 0;
            if (!_parse_size(argv[++i], &width, &height))
            {
                fprintf(stderr, "scenario_runner: invalid --logical-size value\n");
                return -1;
            }
            config.logical_width = width;
            config.logical_height = height;
            config.width = width;
            config.height = height;
        }
        else if (strcmp(arg, "--device-scale") == 0 && i + 1 < argc)
        {
            if (!_parse_float(argv[++i], &config.device_scale))
            {
                fprintf(stderr, "scenario_runner: invalid --device-scale value\n");
                return -1;
            }
            config.framebuffer_width = 0;
            config.framebuffer_height = 0;
        }
        else if (strcmp(arg, "--user-scale") == 0 && i + 1 < argc)
        {
            if (!_parse_float(argv[++i], &config.user_scale))
            {
                fprintf(stderr, "scenario_runner: invalid --user-scale value\n");
                return -1;
            }
        }
        else if (strcmp(arg, "--render-scale") == 0 && i + 1 < argc)
        {
            if (!_parse_float(argv[++i], &config.render_scale))
            {
                fprintf(stderr, "scenario_runner: invalid --render-scale value\n");
                return -1;
            }
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

    if (
        config.presentation == DVZ_RUNNER_PRESENT_GLFW && config.frame_count > 0 &&
        !config.benchmark)
        config.pace_wall_time = true;

    return dvz_scenario_run_native(spec, &config);
}
