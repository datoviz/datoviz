/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Interactive canvas smoke app                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_runtime.h"
#include "_scene.h"
#include "canvas_internal.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/keycodes.h"
#include "datoviz/scene.h"
#include "datoviz/stream.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#include "datoviz_gpu_selection.h"
#include "frame_plan/frame_plan.h"

#if DVZ_HAS_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CANVAS_DEFAULT_BG_R             0.08f
#define DVZ_CANVAS_DEFAULT_BG_G             0.12f
#define DVZ_CANVAS_DEFAULT_BG_B             0.16f
#define DVZ_CANVAS_DEFAULT_BG_A             1.00f
#define DVZ_CANVAS_DEFAULT_FPS              60
#define DVZ_CANVAS_DEFAULT_WIDTH            1024
#define DVZ_CANVAS_DEFAULT_HEIGHT           640
#define DVZ_CANVAS_BENCHMARK_DEFAULT_FRAMES 10000
#define DVZ_CANVAS_BENCHMARK_WARMUP_FRAMES  200
#define DVZ_CANVAS_SCENE_MAX_POINTS         10000000u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_CANVAS_DRAW_CLEAR,
    DVZ_CANVAS_DRAW_SCENE_DRP2,
} DvzCanvasDrawMode;


typedef enum
{
    DVZ_CANVAS_SCENE_PATH_FULL,
    DVZ_CANVAS_SCENE_PATH_CACHED_PLAN,
    DVZ_CANVAS_SCENE_PATH_CACHED_STREAM,
} DvzCanvasScenePath;


typedef struct DvzCanvasSceneTiming
{
    double attach_ms;
    double plan_ms;
    double emit_ms;
    double execute_ms;
    double cleanup_ms;
    double semantic_validation_ms;
    double backend_ms;
    double semantic_commit_ms;
} DvzCanvasSceneTiming;


typedef struct DvzCanvasAppOptions
{
    DvzBackend backend;
    DvzCanvasRenderMode render_mode;
    uint32_t width;
    uint32_t height;
    uint32_t max_frames;
    float bg[4];
    int fps;
    VkPresentModeKHR present_mode;
    double duration_s;
    const char* title;
    const char* record_path;
    const char* screenshot_base;
    DvzVideoCaptureMode record_mode;
    DvzCanvasDrawMode draw_mode;
    DvzCanvasScenePath scene_path;
    uint32_t scene_point_count;
    bool start_recording;
    bool benchmark;
    DvzTestingGpuSelection gpu_selection;
} DvzCanvasAppOptions;



typedef struct DvzCanvasApp
{
    DvzCanvasAppOptions options;
    DvzInstance* instance;
    DvzWindowHost* host;
    DvzDevice* device;
    DvzVma* allocator;
    DvzWindow* window;
    DvzCanvas* canvas;
    DvzDrp2Runtime* drp2_runtime;
    DvzFramePlanEmitter* scene_emitter;
    float* scene_point_positions;
    DvzColor* scene_point_colors;
    float* scene_point_sizes;
    DvzFramePlan* scene_cached_plan;
    DvzDrp2CommandStream* scene_cached_stream;
    DvzCapabilitySnapshot scene_caps;
    DvzFramePlanEmitConfig scene_emit_cfg;
    DvzVideoSinkConfig video_cfg;
    bool running;
    bool recording;
    bool toggle_record_requested;
    bool screenshot_requested;
    uint32_t screenshot_index;
    int key_prev_escape;
    int key_prev_s;
    int key_prev_r;
    DvzCallbackId keyboard_subscription_id;
    uint64_t scene_frame_index;
    bool scene_last_ok;
    bool scene_failed;
    bool scene_reported_ok;
    bool scene_reported_error;
    bool scene_stream_warm;
    DvzFormat scene_cached_stream_format;
    uint32_t scene_cached_stream_width;
    uint32_t scene_cached_stream_height;
    bool present_mode_reported;
    double* benchmark_frame_ms;
    uint32_t benchmark_warmup_frames;
    uint32_t benchmark_sample_count;
    double benchmark_start_s;
    double benchmark_last_s;
    uint64_t benchmark_recreate_start;
    DvzCanvasSceneTiming* benchmark_scene_timings;
    uint32_t benchmark_scene_timing_count;
} DvzCanvasApp;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill application options with sensible defaults.
 *
 * @param options destination options structure
 */
static void _dvz_canvas_options_default(DvzCanvasAppOptions* options)
{
    ANN(options);
    options->backend = DVZ_BACKEND_GLFW;
    options->render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT;
    options->width = DVZ_CANVAS_DEFAULT_WIDTH;
    options->height = DVZ_CANVAS_DEFAULT_HEIGHT;
    options->max_frames = 0;
    options->bg[0] = DVZ_CANVAS_DEFAULT_BG_R;
    options->bg[1] = DVZ_CANVAS_DEFAULT_BG_G;
    options->bg[2] = DVZ_CANVAS_DEFAULT_BG_B;
    options->bg[3] = DVZ_CANVAS_DEFAULT_BG_A;
    options->fps = DVZ_CANVAS_DEFAULT_FPS;
    options->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    options->duration_s = 0.0;
    options->title = "Datoviz Canvas";
    options->record_path = "canvas.mp4";
    options->screenshot_base = "canvas_capture";
    options->record_mode = DVZ_VIDEO_CAPTURE_AUTO;
    options->draw_mode = DVZ_CANVAS_DRAW_CLEAR;
    options->scene_path = DVZ_CANVAS_SCENE_PATH_FULL;
    options->scene_point_count = 1;
    options->start_recording = false;
    options->benchmark = false;
    dvz_testing_gpu_selection_init(&options->gpu_selection);
}



/**
 * Parse an unsigned integer command-line value.
 *
 * @param arg input text
 * @param out destination parsed value
 * @returns true on success, false on parse failure
 */
static bool _dvz_canvas_parse_u32(const char* arg, uint32_t* out)
{
    ANN(arg);
    ANN(out);
    char* end = NULL;
    unsigned long value = strtoul(arg, &end, 10);
    if (end == arg || *end != '\0' || value == 0 || value > UINT32_MAX)
    {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse a double command-line value.
 *
 * @param arg input text
 * @param out destination parsed value
 * @returns true on success, false on parse failure
 */
static bool _dvz_canvas_parse_double(const char* arg, double* out)
{
    ANN(arg);
    ANN(out);
    char* end = NULL;
    double value = strtod(arg, &end);
    if (end == arg || *end != '\0' || value < 0.0)
    {
        return false;
    }
    *out = value;
    return true;
}



/**
 * Parse an RGBA color list encoded as r,g,b,a floats.
 *
 * @param arg input text
 * @param rgba destination color values
 * @returns true on success, false otherwise
 */
static bool _dvz_canvas_parse_bg(const char* arg, float rgba[4])
{
    ANN(arg);
    ANN(rgba);
    float r = 0, g = 0, b = 0, a = 0;
    if (sscanf(arg, "%f,%f,%f,%f", &r, &g, &b, &a) != 4)
    {
        return false;
    }
    rgba[0] = r;
    rgba[1] = g;
    rgba[2] = b;
    rgba[3] = a;
    return true;
}



/**
 * Return a display name for a Vulkan present mode.
 *
 * @param mode Vulkan present mode
 * @returns static present mode name
 */
static const char* _dvz_canvas_present_mode_name(VkPresentModeKHR mode)
{
    switch (mode)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return "immediate";
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return "mailbox";
    case VK_PRESENT_MODE_FIFO_KHR:
        return "fifo";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return "fifo-relaxed";
    default:
        return "unknown";
    }
}



/**
 * Return a display name for the scene benchmark path.
 *
 * @param path scene benchmark path
 * @returns static path name
 */
static const char* _dvz_canvas_scene_path_name(DvzCanvasScenePath path)
{
    switch (path)
    {
    case DVZ_CANVAS_SCENE_PATH_FULL:
        return "full";
    case DVZ_CANVAS_SCENE_PATH_CACHED_PLAN:
        return "cached-plan";
    case DVZ_CANVAS_SCENE_PATH_CACHED_STREAM:
        return "cached-stream";
    default:
        return "unknown";
    }
}



/**
 * Return a high-resolution monotonic timestamp in seconds.
 *
 * @returns monotonic timestamp in seconds
 */
static double _dvz_canvas_benchmark_now(void)
{
    struct timespec ts = {0};
#if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    timespec_get(&ts, TIME_UTC);
#endif
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}



/**
 * Compare two double values for qsort().
 *
 * @param a first value pointer
 * @param b second value pointer
 * @returns negative, zero, or positive comparison result
 */
static int _dvz_canvas_compare_double(const void* a, const void* b)
{
    const double da = *(const double*)a;
    const double db = *(const double*)b;
    if (da < db)
    {
        return -1;
    }
    if (da > db)
    {
        return 1;
    }
    return 0;
}



/**
 * Return an integer percentile from a sorted frame-time array.
 *
 * @param sorted sorted frame-time samples in milliseconds
 * @param count number of samples
 * @param percentile percentile in the [0, 100] range
 * @returns selected percentile value, or zero without samples
 */
static double
_dvz_canvas_benchmark_percentile(const double* sorted, uint32_t count, uint32_t percentile)
{
    if (sorted == NULL || count == 0)
    {
        return 0.0;
    }
    if (percentile >= 100)
    {
        return sorted[count - 1];
    }
    uint64_t numerator = (uint64_t)percentile * (uint64_t)(count - 1) + 50;
    uint32_t idx = (uint32_t)(numerator / 100);
    if (idx >= count)
    {
        idx = count - 1;
    }
    return sorted[idx];
}



/**
 * Initialize benchmark state before entering the frame loop.
 *
 * @param app canvas app state
 * @returns true on success, false on allocation failure
 */
static bool _dvz_canvas_benchmark_begin(DvzCanvasApp* app)
{
    ANN(app);
    if (!app->options.benchmark)
    {
        return true;
    }

    uint32_t max_frames = app->options.max_frames;
    if (max_frames == 0)
    {
        max_frames = DVZ_CANVAS_BENCHMARK_DEFAULT_FRAMES;
        app->options.max_frames = max_frames;
    }

    uint32_t max_warmup = max_frames / 10;
    if (max_warmup == 0)
        max_warmup = 1;
    app->benchmark_warmup_frames = DVZ_CANVAS_BENCHMARK_WARMUP_FRAMES;
    if (app->benchmark_warmup_frames > max_warmup)
    {
        app->benchmark_warmup_frames = max_warmup;
    }

    uint32_t sample_capacity = max_frames - app->benchmark_warmup_frames;
    app->benchmark_frame_ms = (double*)dvz_calloc(sample_capacity, sizeof(double));
    if (app->benchmark_frame_ms == NULL)
    {
        dvz_fprintf(stderr, "benchmark: failed to allocate frame-time buffer\n");
        return false;
    }
    if (app->options.draw_mode == DVZ_CANVAS_DRAW_SCENE_DRP2)
    {
        app->benchmark_scene_timings =
            (DvzCanvasSceneTiming*)dvz_calloc(max_frames, sizeof(DvzCanvasSceneTiming));
        if (app->benchmark_scene_timings == NULL)
        {
            dvz_fprintf(stderr, "benchmark: failed to allocate scene timing buffer\n");
            return false;
        }
    }
    app->benchmark_sample_count = 0;
    app->benchmark_scene_timing_count = 0;
    app->benchmark_recreate_start = 0;

    double now = _dvz_canvas_benchmark_now();
    app->benchmark_start_s = now;
    app->benchmark_last_s = now;
    return true;
}



/**
 * Record one submitted frame in the active benchmark.
 *
 * @param app canvas app state
 * @param submitted_frames submitted frame count after the current submit
 */
static void _dvz_canvas_benchmark_record(DvzCanvasApp* app, uint64_t submitted_frames)
{
    ANN(app);
    if (!app->options.benchmark)
    {
        return;
    }

    double now = _dvz_canvas_benchmark_now();
    if (submitted_frames == app->benchmark_warmup_frames)
    {
        app->benchmark_start_s = now;
        app->benchmark_last_s = now;
        app->benchmark_recreate_start = dvz_canvas_swapchain_recreate_count(app->canvas);
        return;
    }
    if (submitted_frames < app->benchmark_warmup_frames)
    {
        return;
    }

    uint32_t capacity = app->options.max_frames - app->benchmark_warmup_frames;
    if (app->benchmark_sample_count >= capacity)
    {
        return;
    }

    app->benchmark_frame_ms[app->benchmark_sample_count++] =
        (now - app->benchmark_last_s) * 1000.0;
    app->benchmark_last_s = now;
}



/**
 * Print benchmark throughput, frame-time distribution, stutters, and recreation counts.
 *
 * @param app canvas app state
 * @returns true when no steady-state swapchain recreation occurred
 */
static bool _dvz_canvas_benchmark_end(DvzCanvasApp* app)
{
    ANN(app);
    if (!app->options.benchmark || app->benchmark_sample_count == 0)
    {
        return true;
    }

    uint32_t count = app->benchmark_sample_count;
    double* sorted = (double*)dvz_calloc(count, sizeof(double));
    if (sorted == NULL)
    {
        dvz_fprintf(stderr, "benchmark: failed to allocate percentile buffer\n");
        return false;
    }
    dvz_memcpy(sorted, count * sizeof(double), app->benchmark_frame_ms, count * sizeof(double));
    qsort(sorted, count, sizeof(double), _dvz_canvas_compare_double);

    double sum_ms = 0.0;
    double max_ms = sorted[count - 1];
    uint32_t stutter_2ms = 0;
    uint32_t stutter_5ms = 0;
    uint32_t stutter_10ms = 0;
    uint32_t stutter_16ms = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        double dt = app->benchmark_frame_ms[i];
        sum_ms += dt;
        if (dt > 2.0)
        {
            stutter_2ms++;
        }
        if (dt > 5.0)
        {
            stutter_5ms++;
        }
        if (dt > 10.0)
        {
            stutter_10ms++;
        }
        if (dt > 16.6667)
        {
            stutter_16ms++;
        }
    }

    double elapsed_s = app->benchmark_last_s - app->benchmark_start_s;
    double fps = elapsed_s > 0.0 ? (double)count / elapsed_s : 0.0;
    double avg_ms = count > 0 ? sum_ms / (double)count : 0.0;
    VkPresentModeKHR resolved = VK_PRESENT_MODE_FIFO_KHR;
    bool has_resolved = dvz_canvas_swapchain_present_mode(app->canvas, &resolved);
    uint64_t recreate_count = dvz_canvas_swapchain_recreate_count(app->canvas);
    uint64_t recreate_delta = recreate_count - app->benchmark_recreate_start;

    dvz_fprintf(
        stderr, "benchmark: frames=%u warmup=%u samples=%u elapsed=%.6fs fps=%.2f avg_ms=%.4f\n",
        app->options.max_frames, app->benchmark_warmup_frames, count, elapsed_s, fps, avg_ms);
    if (has_resolved)
    {
        dvz_fprintf(
            stderr, "benchmark: present requested=%s (%d), resolved=%s (%d)\n",
            _dvz_canvas_present_mode_name(app->options.present_mode),
            (int)app->options.present_mode, _dvz_canvas_present_mode_name(resolved),
            (int)resolved);
    }
    dvz_fprintf(
        stderr, "benchmark: frame_ms min=%.4f p50=%.4f p90=%.4f p95=%.4f p99=%.4f max=%.4f\n",
        sorted[0], _dvz_canvas_benchmark_percentile(sorted, count, 50),
        _dvz_canvas_benchmark_percentile(sorted, count, 90),
        _dvz_canvas_benchmark_percentile(sorted, count, 95),
        _dvz_canvas_benchmark_percentile(sorted, count, 99), max_ms);
    dvz_fprintf(
        stderr, "benchmark: stutters >2ms=%u >5ms=%u >10ms=%u >16.67ms=%u\n", stutter_2ms,
        stutter_5ms, stutter_10ms, stutter_16ms);
    dvz_fprintf(
        stderr, "benchmark: swapchain recreates total=%llu steady=%llu\n",
        (unsigned long long)recreate_count, (unsigned long long)recreate_delta);

    if (app->options.draw_mode == DVZ_CANVAS_DRAW_SCENE_DRP2 &&
        app->benchmark_scene_timings != NULL &&
        app->benchmark_scene_timing_count > app->benchmark_warmup_frames)
    {
        dvz_fprintf(stderr, "benchmark: scene_points=%u\n", app->options.scene_point_count);
        DvzCanvasSceneTiming total = {0};
        uint32_t timing_end = app->benchmark_scene_timing_count;
        if (timing_end > app->options.max_frames)
            timing_end = app->options.max_frames;
        uint32_t timing_count = timing_end - app->benchmark_warmup_frames;
        for (uint32_t i = app->benchmark_warmup_frames; i < timing_end; i++)
        {
            const DvzCanvasSceneTiming* timing = &app->benchmark_scene_timings[i];
            total.attach_ms += timing->attach_ms;
            total.plan_ms += timing->plan_ms;
            total.emit_ms += timing->emit_ms;
            total.execute_ms += timing->execute_ms;
            total.cleanup_ms += timing->cleanup_ms;
            total.semantic_validation_ms += timing->semantic_validation_ms;
            total.backend_ms += timing->backend_ms;
            total.semantic_commit_ms += timing->semantic_commit_ms;
        }
        double divisor = timing_count > 0 ? (double)timing_count : 1.0;
        double callback_ms = (total.attach_ms + total.plan_ms + total.emit_ms + total.execute_ms +
                              total.cleanup_ms) /
                             divisor;
        dvz_fprintf(
            stderr,
            "benchmark: scene_path=%s phase_ms attach=%.4f plan=%.4f emit=%.4f "
            "execute=%.4f cleanup=%.4f callback=%.4f residual=%.4f\n",
            _dvz_canvas_scene_path_name(app->options.scene_path), total.attach_ms / divisor,
            total.plan_ms / divisor, total.emit_ms / divisor, total.execute_ms / divisor,
            total.cleanup_ms / divisor, callback_ms,
            avg_ms > callback_ms ? avg_ms - callback_ms : 0.0);
        dvz_fprintf(
            stderr,
            "benchmark: runtime_phase_ms semantic_validation=%.4f backend=%.4f "
            "semantic_commit=%.4f\n",
            total.semantic_validation_ms / divisor, total.backend_ms / divisor,
            total.semantic_commit_ms / divisor);
    }

    dvz_free(sorted);
    return recreate_delta == 0;
}



/**
 * Print the requested and resolved swapchain present modes once they are available.
 *
 * @param app canvas app state
 */
static void _dvz_canvas_report_present_mode(DvzCanvasApp* app)
{
    ANN(app);
    if (app->present_mode_reported || app->options.render_mode != DVZ_CANVAS_RENDER_MODE_PRESENT ||
        app->canvas == NULL)
    {
        return;
    }

    VkPresentModeKHR resolved = VK_PRESENT_MODE_FIFO_KHR;
    if (!dvz_canvas_swapchain_present_mode(app->canvas, &resolved))
    {
        return;
    }

    VkPresentModeKHR requested = app->options.present_mode;
    dvz_fprintf(
        stderr, "present mode: requested=%s (%d), resolved=%s (%d)\n",
        _dvz_canvas_present_mode_name(requested), (int)requested,
        _dvz_canvas_present_mode_name(resolved), (int)resolved);
    app->present_mode_reported = true;
}



/**
 * Print command-line usage information.
 */
static void _dvz_canvas_usage(void)
{
    dvz_fprintf(
        stderr,
        "usage: dvz_live_canvas [--backend glfw|offscreen] [--width N] [--height N]\n"
        "                  [--mode present|offscreen] [--frames N] [--bg r,g,b,a] [--fps N]\n"
        "                  [--draw clear|scene-drp2]\n"
        "                  [--scene-path full|cached-plan|cached-stream]\n"
        "                  [--scene-points N]\n"
        "                  [--present fifo|immediate] [--duration seconds]\n"
        "                  [--record path.mp4] [--record-mode auto|external|cpu]\n"
        "                  [--start-recording] [--screenshots base] [--benchmark] [--gpu index]\n"
        "Environment: DVZ_TEST_GPU selects an index when --gpu is absent.\n"
        "\n"
        "hotkeys: Esc quit, S screenshot, R toggle recording\n");
}



/**
 * Parse command-line arguments into options.
 *
 * @param argc argument count
 * @param argv argument vector
 * @param options destination options
 * @returns true on success, false on invalid arguments
 */
static bool _dvz_canvas_parse_args(int argc, char** argv, DvzCanvasAppOptions* options)
{
    ANN(options);
    bool mode_explicit = false;
    bool backend_explicit = false;
    bool present_explicit = false;
    bool gpu_seen = false;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (strcmp(arg, "--") == 0)
        {
            continue;
        }
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            _dvz_canvas_usage();
            return false;
        }
        if (strcmp(arg, "--start-recording") == 0)
        {
            options->start_recording = true;
            continue;
        }
        if (strcmp(arg, "--benchmark") == 0)
        {
            options->benchmark = true;
            continue;
        }
        if (strncmp(arg, "--gpu=", 6) == 0)
        {
            dvz_fprintf(stderr, "use --gpu <index>; --gpu=<index> is not supported\n");
            return false;
        }
        if (i + 1 >= argc)
        {
            dvz_fprintf(stderr, "missing value after %s\\n", arg);
            return false;
        }
        const char* value = argv[++i];
        if (strcmp(arg, "--width") == 0)
        {
            if (!_dvz_canvas_parse_u32(value, &options->width))
                return false;
        }
        else if (strcmp(arg, "--backend") == 0)
        {
            backend_explicit = true;
            if (strcmp(value, "glfw") == 0)
            {
                options->backend = DVZ_BACKEND_GLFW;
            }
            else if (strcmp(value, "offscreen") == 0)
            {
                options->backend = DVZ_BACKEND_OFFSCREEN;
            }
            else
            {
                dvz_fprintf(stderr, "invalid backend: %s\\n", value);
                return false;
            }
        }
        else if (strcmp(arg, "--mode") == 0)
        {
            mode_explicit = true;
            if (strcmp(value, "present") == 0)
            {
                options->render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT;
            }
            else if (strcmp(value, "offscreen") == 0)
            {
                options->render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
            }
            else
            {
                dvz_fprintf(stderr, "invalid mode: %s\\n", value);
                return false;
            }
        }
        else if (strcmp(arg, "--height") == 0)
        {
            if (!_dvz_canvas_parse_u32(value, &options->height))
                return false;
        }
        else if (strcmp(arg, "--gpu") == 0)
        {
            if (gpu_seen)
            {
                dvz_fprintf(stderr, "--gpu may be specified only once\n");
                return false;
            }
            if (!dvz_testing_gpu_selection_set_cli(&options->gpu_selection, value))
            {
                dvz_fprintf(stderr, "invalid --gpu index: %s\n", value);
                return false;
            }
            gpu_seen = true;
        }
        else if (strcmp(arg, "--frames") == 0)
        {
            if (!_dvz_canvas_parse_u32(value, &options->max_frames))
                return false;
        }
        else if (strcmp(arg, "--bg") == 0)
        {
            if (!_dvz_canvas_parse_bg(value, options->bg))
                return false;
        }
        else if (strcmp(arg, "--fps") == 0)
        {
            uint32_t fps = 0;
            if (!_dvz_canvas_parse_u32(value, &fps))
                return false;
            options->fps = (int)fps;
        }
        else if (strcmp(arg, "--draw") == 0)
        {
            if (strcmp(value, "clear") == 0)
            {
                options->draw_mode = DVZ_CANVAS_DRAW_CLEAR;
            }
            else if (strcmp(value, "scene-drp2") == 0)
            {
                options->draw_mode = DVZ_CANVAS_DRAW_SCENE_DRP2;
            }
            else
            {
                dvz_fprintf(stderr, "invalid draw mode: %s\\n", value);
                return false;
            }
        }
        else if (strcmp(arg, "--scene-path") == 0)
        {
            if (strcmp(value, "full") == 0)
            {
                options->scene_path = DVZ_CANVAS_SCENE_PATH_FULL;
            }
            else if (strcmp(value, "cached-plan") == 0)
            {
                options->scene_path = DVZ_CANVAS_SCENE_PATH_CACHED_PLAN;
            }
            else if (strcmp(value, "cached-stream") == 0)
            {
                options->scene_path = DVZ_CANVAS_SCENE_PATH_CACHED_STREAM;
            }
            else
            {
                dvz_fprintf(stderr, "invalid scene path: %s\n", value);
                return false;
            }
        }
        else if (strcmp(arg, "--scene-points") == 0)
        {
            if (!_dvz_canvas_parse_u32(value, &options->scene_point_count))
                return false;
        }
        else if (strcmp(arg, "--duration") == 0)
        {
            if (!_dvz_canvas_parse_double(value, &options->duration_s))
                return false;
        }
        else if (strcmp(arg, "--record") == 0)
        {
            options->record_path = value;
        }
        else if (strcmp(arg, "--screenshots") == 0)
        {
            options->screenshot_base = value;
        }
        else if (strcmp(arg, "--record-mode") == 0)
        {
            if (strcmp(value, "auto") == 0)
            {
                options->record_mode = DVZ_VIDEO_CAPTURE_AUTO;
            }
            else if (strcmp(value, "external") == 0)
            {
                options->record_mode = DVZ_VIDEO_CAPTURE_EXTERNAL;
            }
            else if (strcmp(value, "cpu") == 0)
            {
                options->record_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;
            }
            else
            {
                dvz_fprintf(stderr, "invalid record mode: %s\\n", value);
                return false;
            }
        }
        else if (strcmp(arg, "--present") == 0)
        {
            present_explicit = true;
            if (strcmp(value, "immediate") == 0)
            {
                options->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
            else if (strcmp(value, "fifo") == 0)
            {
                options->present_mode = VK_PRESENT_MODE_FIFO_KHR;
            }
            else
            {
                dvz_fprintf(stderr, "invalid present mode: %s\\n", value);
                return false;
            }
        }
        else
        {
            dvz_fprintf(stderr, "unknown argument: %s\\n", arg);
            return false;
        }
    }

    if (!mode_explicit && backend_explicit)
    {
        options->render_mode = options->backend == DVZ_BACKEND_OFFSCREEN
                                   ? DVZ_CANVAS_RENDER_MODE_OFFSCREEN
                                   : DVZ_CANVAS_RENDER_MODE_PRESENT;
    }
    if (options->draw_mode != DVZ_CANVAS_DRAW_SCENE_DRP2 &&
        options->scene_path != DVZ_CANVAS_SCENE_PATH_FULL)
    {
        dvz_fprintf(stderr, "--scene-path requires --draw scene-drp2\n");
        return false;
    }
    if (options->draw_mode != DVZ_CANVAS_DRAW_SCENE_DRP2 && options->scene_point_count != 1)
    {
        dvz_fprintf(stderr, "--scene-points requires --draw scene-drp2\n");
        return false;
    }
    if (options->backend == DVZ_BACKEND_OFFSCREEN &&
        options->render_mode == DVZ_CANVAS_RENDER_MODE_PRESENT)
    {
        dvz_fprintf(
            stderr, "invalid combination: --backend offscreen requires --mode offscreen\\n");
        return false;
    }
    if (!dvz_testing_gpu_selection_set_environment(&options->gpu_selection))
    {
        dvz_fprintf(stderr, "invalid DVZ_TEST_GPU index\n");
        return false;
    }
    if (options->benchmark)
    {
        if (options->max_frames == 0)
        {
            options->max_frames = DVZ_CANVAS_BENCHMARK_DEFAULT_FRAMES;
        }
        if (options->max_frames < 2)
        {
            dvz_fprintf(stderr, "benchmark requires at least two frames\n");
            return false;
        }
        if (!present_explicit && options->render_mode == DVZ_CANVAS_RENDER_MODE_PRESENT)
        {
            options->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }
    return true;
}



/**
 * Record a fullscreen clear pass for the current canvas command buffer.
 *
 * @param canvas owning canvas
 * @param frame stream frame with command buffer and target image view
 * @param user_data pointer to the RGBA clear color array
 */
static void _dvz_canvas_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    ANN(canvas);
    ANN(frame);
    if (frame->command_buffer == VK_NULL_HANDLE || frame->image_view == VK_NULL_HANDLE)
    {
        return;
    }

    float* bg = (float*)user_data;
    ANN(bg);
    DvzDevice* device = dvz_stream_device(dvz_canvas_stream(canvas));
    ANN(device);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(device, frame->command_buffer, cmds);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    ANN(rendering);
    dvz_cmd_rendering_default(
        cmds, frame->image_view, frame->extent.width, frame->extent.height,
        (VkClearValue){.color.float32 = {bg[0], bg[1], bg[2], bg[3]}}, rendering);
    dvz_cmd_rendering_begin(cmds, rendering);
    dvz_cmd_rendering_end(cmds);
    dvz_rendering_free(rendering);
    dvz_commands_free(cmds);
}



/**
 * Build the fixed minimal scene plan used by the live canvas benchmark.
 *
 * @param app canvas app state
 * @return owned frame plan, or NULL on failure
 */
static DvzFramePlan* _dvz_canvas_scene_plan(DvzCanvasApp* app)
{
    ANN(app);
    DvzFramePlan* plan = dvz_frame_plan("live.canvas.scene", app->scene_frame_index++);
    if (plan == NULL)
        return NULL;

    const uint32_t point_count = app->options.scene_point_count;
    const size_t position_size = (size_t)point_count * 3 * sizeof(float);
    const size_t color_size = (size_t)point_count * sizeof(DvzColor);
    const size_t point_size = (size_t)point_count * sizeof(float);
    bool ok = dvz_frame_plan_upload_bytes(
        plan, "visual.point.0_position", 0, position_size, "position", app->scene_point_positions);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "visual.point.0_color", 0, color_size, "color", app->scene_point_colors);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "visual.point.0_size", 0, point_size, "size", app->scene_point_sizes);
    ok = ok && dvz_frame_plan_render(plan, "panel.0", "target.canvas.color", false);
    ok = ok && dvz_frame_plan_render_visual(plan, "visual.point.0");
    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    metadata.vertex_count = point_count;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = true;
    dvz_strlcpy(metadata.position_id, "visual.point.0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "visual.point.0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "visual.point.0_size", sizeof(metadata.size_id));
    ok = ok && dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (!ok)
    {
        dvz_frame_plan_destroy(plan);
        return NULL;
    }
    return plan;
}



/**
 * Allocate deterministic point data for the minimal scene benchmark.
 *
 * @param app canvas app state
 * @return true on success
 */
static bool _dvz_canvas_scene_data_init(DvzCanvasApp* app)
{
    ANN(app);
    const uint32_t point_count = app->options.scene_point_count;
    if (point_count == 0 || point_count > DVZ_CANVAS_SCENE_MAX_POINTS)
        return false;

    app->scene_point_positions = (float*)dvz_calloc((size_t)point_count * 3, sizeof(float));
    app->scene_point_colors = (DvzColor*)dvz_calloc(point_count, sizeof(DvzColor));
    app->scene_point_sizes = (float*)dvz_calloc(point_count, sizeof(float));
    if (app->scene_point_positions == NULL || app->scene_point_colors == NULL ||
        app->scene_point_sizes == NULL)
    {
        return false;
    }

    const uint32_t side = (uint32_t)ceil(sqrt((double)point_count));
    for (uint32_t i = 0; i < point_count; i++)
    {
        const uint32_t row = i / side;
        const uint32_t col = i % side;
        app->scene_point_positions[3 * i + 0] =
            side > 1 ? -0.9f + 1.8f * (float)col / (float)(side - 1) : 0.0f;
        app->scene_point_positions[3 * i + 1] =
            side > 1 ? -0.9f + 1.8f * (float)row / (float)(side - 1) : 0.0f;
        app->scene_point_colors[i] = (DvzColor){255, 96, 32, 200};
        app->scene_point_sizes[i] = 8.0f;
    }
    return true;
}



/**
 * Record benchmark-only scene phase timings for one callback.
 *
 * @param app canvas app state
 * @param timing phase timings
 */
static void _dvz_canvas_scene_timing_record(DvzCanvasApp* app, const DvzCanvasSceneTiming* timing)
{
    ANN(app);
    ANN(timing);
    if (!app->options.benchmark || app->benchmark_scene_timings == NULL ||
        app->benchmark_scene_timing_count >= app->options.max_frames)
    {
        return;
    }
    app->benchmark_scene_timings[app->benchmark_scene_timing_count++] = *timing;
}


/**
 * Render a minimal scene through DRP2 into the current canvas frame.
 *
 * @param canvas owning canvas
 * @param frame stream frame with command buffer and target image view
 * @param user_data app state pointer
 */
static void
_dvz_canvas_draw_scene_drp2(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzCanvasApp* app = (DvzCanvasApp*)user_data;
    ANN(app);
    app->scene_last_ok = false;
    DvzCanvasSceneTiming timing = {0};
    double phase_start = 0.0;
    const char* stage = "initialize scene runtime";
    DvzDrp2ValidationResult result = {0};
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlan* plan = NULL;
    DvzDrp2CommandStream* stream = NULL;
    bool destroy_plan = false;
    bool destroy_stream = false;
    bool emitted_stream = false;
    bool ok = true;

    if (app->drp2_runtime == NULL || app->scene_emitter == NULL)
    {
        ok = false;
        goto finish;
    }

    if (app->scene_cached_stream != NULL &&
        (app->scene_cached_stream_format != (DvzFormat)frame->color_format ||
         app->scene_cached_stream_width != frame->extent.width ||
         app->scene_cached_stream_height != frame->extent.height))
    {
        dvz_drp2_stream_destroy(app->scene_cached_stream);
        app->scene_cached_stream = NULL;
        app->scene_stream_warm = false;
    }

    stage = "attach frame target";
    phase_start = _dvz_canvas_benchmark_now();
    ok = dvz_drp2_runtime_attach_frame_target(app->drp2_runtime, 1, frame);
    timing.attach_ms = (_dvz_canvas_benchmark_now() - phase_start) * 1000.0;
    if (!ok)
        goto finish;

    if (app->options.scene_path == DVZ_CANVAS_SCENE_PATH_CACHED_STREAM &&
        app->scene_cached_stream != NULL)
    {
        stream = app->scene_cached_stream;
    }
    else
    {
        stage = "build frame plan";
        phase_start = _dvz_canvas_benchmark_now();
        if (app->options.scene_path == DVZ_CANVAS_SCENE_PATH_FULL)
        {
            plan = _dvz_canvas_scene_plan(app);
            destroy_plan = true;
        }
        else
        {
            if (app->scene_cached_plan == NULL)
                app->scene_cached_plan = _dvz_canvas_scene_plan(app);
            plan = app->scene_cached_plan;
        }
        timing.plan_ms = (_dvz_canvas_benchmark_now() - phase_start) * 1000.0;
        if (plan == NULL)
        {
            ok = false;
            goto finish;
        }

        app->scene_emit_cfg.color_target_format = (DvzFormat)frame->color_format;
        app->scene_emit_cfg.target_width = frame->extent.width;
        app->scene_emit_cfg.target_height = frame->extent.height;
        stage = "emit DRP2 stream";
        phase_start = _dvz_canvas_benchmark_now();
        stream = dvz_frame_plan_emitter_emit_drp2(
            app->scene_emitter, plan, &app->scene_caps, &report, &app->scene_emit_cfg);
        timing.emit_ms = (_dvz_canvas_benchmark_now() - phase_start) * 1000.0;
        ok = stream != NULL && dvz_diagnostic_report_count(&report) == 0;
        emitted_stream = stream != NULL;
        destroy_stream = stream != NULL;
    }
    if (ok)
    {
        stage = "execute DRP2 stream";
        phase_start = _dvz_canvas_benchmark_now();
        result = dvz_drp2_runtime_execute(app->drp2_runtime, stream);
        timing.execute_ms = (_dvz_canvas_benchmark_now() - phase_start) * 1000.0;
        DvzDrp2RuntimeTiming runtime_timing = {0};
        if (_dvz_drp2_runtime_timing_get(app->drp2_runtime, &runtime_timing))
        {
            timing.semantic_validation_ms = (double)runtime_timing.semantic_validation_ns * 1e-6;
            timing.backend_ms = (double)runtime_timing.backend_ns * 1e-6;
            timing.semantic_commit_ms = (double)runtime_timing.semantic_commit_ns * 1e-6;
        }
        ok = result.ok && result.code == DVZ_DRP2_VALIDATION_OK;
    }

    if (ok && emitted_stream && app->options.scene_path == DVZ_CANVAS_SCENE_PATH_CACHED_STREAM)
    {
        if (app->scene_stream_warm)
        {
            app->scene_cached_stream = stream;
            app->scene_cached_stream_format = (DvzFormat)frame->color_format;
            app->scene_cached_stream_width = frame->extent.width;
            app->scene_cached_stream_height = frame->extent.height;
            destroy_stream = false;
        }
        else
        {
            app->scene_stream_warm = true;
        }
    }

finish:
    app->scene_last_ok = ok;
    app->scene_failed = app->scene_failed || !ok;
    if (ok && !app->scene_reported_ok)
    {
        dvz_fprintf(
            stderr, "scene-drp2: rendering into canvas target %ux%u\n", frame->extent.width,
            frame->extent.height);
        app->scene_reported_ok = true;
    }
    if (!ok && !app->scene_reported_error)
    {
        DvzDrp2CommandType command_type = DVZ_DRP2_COMMAND_NONE;
        if (stream != NULL && result.command_index < dvz_drp2_stream_count(stream))
        {
            const DvzDrp2Command* command = dvz_drp2_stream_get(stream, result.command_index);
            command_type = dvz_drp2_command_type(command);
        }
        dvz_fprintf(
            stderr,
            "scene-drp2: failed to %s (diagnostics=%u, validation=%d at command %u, type=%d)\n",
            stage, dvz_diagnostic_report_count(&report), (int)result.code, result.command_index,
            (int)command_type);
        app->scene_reported_error = true;
    }
    phase_start = _dvz_canvas_benchmark_now();
    if (destroy_stream)
        dvz_drp2_stream_destroy(stream);
    if (destroy_plan)
        dvz_frame_plan_destroy(plan);
    timing.cleanup_ms = (_dvz_canvas_benchmark_now() - phase_start) * 1000.0;
    _dvz_canvas_scene_timing_record(app, &timing);
}



/**
 * Build a numbered PNG path for screenshot capture.
 *
 * @param app canvas app state
 * @param out_path destination buffer
 * @param out_len destination buffer length
 */
static void _dvz_canvas_screenshot_path(DvzCanvasApp* app, char* out_path, size_t out_len)
{
    ANN(app);
    ANN(out_path);
    ASSERT(out_len > 0);
    const char* base =
        app->options.screenshot_base ? app->options.screenshot_base : "canvas_capture";
    int n = dvz_snprintf(out_path, out_len, "%s_%04u.png", base, app->screenshot_index++);
    if (n < 0 || (size_t)n >= out_len)
    {
        dvz_snprintf(out_path, out_len, "canvas_capture_%04u.png", app->screenshot_index++);
    }
}



/**
 * Toggle video recording by enabling or disabling the canvas video sink.
 *
 * @param app canvas app state
 */
static void _dvz_canvas_toggle_recording(DvzCanvasApp* app)
{
    ANN(app);
    ANN(app->canvas);

    if (!app->recording)
    {
        const DvzWindowSurface* surface = dvz_window_surface(app->window);
        ANN(surface);
        app->video_cfg = dvz_video_sink_config();
        app->video_cfg.encoder.backend = "auto";
        app->video_cfg.encoder.width = surface->extent.width;
        app->video_cfg.encoder.height = surface->extent.height;
        app->video_cfg.encoder.fps = (uint32_t)app->options.fps;
        app->video_cfg.encoder.mp4_path = app->options.record_path;
        app->video_cfg.bitstream = NULL;
        app->video_cfg.capture_mode = app->options.record_mode;
        int rc = dvz_canvas_configure_video_sink(app->canvas, true, &app->video_cfg);
        if (rc == 0)
        {
            app->recording = true;
            dvz_fprintf(stderr, "recording enabled: %s\\n", app->options.record_path);
        }
        else
        {
            dvz_fprintf(stderr, "failed to enable recording\\n");
        }
    }
    else
    {
        int rc = dvz_canvas_configure_video_sink(app->canvas, false, NULL);
        if (rc == 0)
        {
            app->recording = false;
            dvz_fprintf(stderr, "recording disabled\\n");
        }
        else
        {
            dvz_fprintf(stderr, "failed to disable recording\\n");
        }
    }
}



/**
 * Handle keyboard events for quit/screenshot/record controls.
 *
 * @param router input router emitting the event
 * @param event keyboard event
 * @param user_data app state pointer
 */
static void
_dvz_canvas_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    DvzCanvasApp* app = (DvzCanvasApp*)user_data;
    ANN(app);

    if (event->type != DVZ_KEYBOARD_EVENT_PRESS)
    {
        return;
    }
    if (event->key == DVZ_KEY_ESCAPE)
    {
        app->running = false;
    }
    else if (event->key == DVZ_KEY_S)
    {
        app->screenshot_requested = true;
    }
    else if (event->key == DVZ_KEY_R)
    {
        app->toggle_record_requested = true;
    }
}



/**
 * Handle GLFW key presses directly for interactive shortcuts.
 *
 * @param handle native GLFW window handle
 * @param key GLFW key code
 * @param scancode platform scancode
 * @param action GLFW key action
 * @param mods GLFW modifiers bitmask
 */
static void
_dvz_canvas_glfw_key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS)
    {
        return;
    }
    DvzWindow* window = (DvzWindow*)glfwGetWindowUserPointer(handle);
    if (window == NULL)
    {
        return;
    }
    DvzCanvasApp* app = (DvzCanvasApp*)dvz_window_user_data(window);
    if (app == NULL)
    {
        return;
    }
    if (key == GLFW_KEY_ESCAPE)
    {
        app->key_prev_escape = GLFW_PRESS;
        app->running = false;
    }
    else if (key == GLFW_KEY_S)
    {
        app->key_prev_s = GLFW_PRESS;
        app->screenshot_requested = true;
    }
    else if (key == GLFW_KEY_R)
    {
        app->key_prev_r = GLFW_PRESS;
        app->toggle_record_requested = true;
    }
}



/**
 * Check whether the native GLFW window was asked to close.
 *
 * @param app app state pointer
 * @returns true when the close button was pressed, false otherwise
 */
static bool _dvz_canvas_window_should_close(const DvzCanvasApp* app)
{
    ANN(app);
    if (app->window == NULL)
    {
        return false;
    }
#if DVZ_HAS_GLFW
    GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(app->window);
    return (handle != NULL) ? glfwWindowShouldClose(handle) != 0 : false;
#else
    return false;
#endif
}



/**
 * Return true only on a GLFW press edge for a single key.
 *
 * @param current current GLFW key state
 * @param previous previous GLFW key state storage
 * @returns true when the key transitioned to pressed
 */
static bool _dvz_canvas_key_pressed_edge(int current, int* previous)
{
    ANN(previous);
    bool pressed = (current == GLFW_PRESS) && (*previous != GLFW_PRESS);
    *previous = current;
    return pressed;
}



/**
 * Poll GLFW keyboard state and trigger app shortcuts.
 *
 * @param app app state pointer
 */
static void _dvz_canvas_poll_keyboard_shortcuts(DvzCanvasApp* app)
{
    ANN(app);
    if (app->window == NULL)
    {
        return;
    }
#if DVZ_HAS_GLFW
    GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(app->window);
    if (handle == NULL)
    {
        return;
    }
    if (glfwGetWindowAttrib(handle, GLFW_FOCUSED) == GLFW_FALSE)
    {
        return;
    }

    if (_dvz_canvas_key_pressed_edge(glfwGetKey(handle, GLFW_KEY_ESCAPE), &app->key_prev_escape))
    {
        app->running = false;
    }
    if (_dvz_canvas_key_pressed_edge(glfwGetKey(handle, GLFW_KEY_S), &app->key_prev_s))
    {
        app->screenshot_requested = true;
    }
    if (_dvz_canvas_key_pressed_edge(glfwGetKey(handle, GLFW_KEY_R), &app->key_prev_r))
    {
        app->toggle_record_requested = true;
    }
#endif
}



/**
 * Create the Vulkan/window/canvas runtime objects needed by the app.
 *
 * @param app app state to initialize
 * @returns true on success, false on failure
 */
static bool _dvz_canvas_init(DvzCanvasApp* app)
{
    ANN(app);
    DvzCanvasAppOptions options = app->options;
    dvz_memset(app, sizeof(*app), 0, sizeof(*app));
    app->options = options;
    app->running = true;

    app->host = dvz_window_host();
    if (app->host == NULL)
    {
        dvz_fprintf(stderr, "failed to create window host\\n");
        return false;
    }

    if (app->options.backend == DVZ_BACKEND_GLFW && !dvz_window_glfw_init())
    {
        dvz_fprintf(stderr, "unable to initialize GLFW\\n");
        return false;
    }

    DvzInstanceConfig icfg = dvz_instance_config();
#if ENABLE_VALIDATION_LAYERS
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
#endif
    uint32_t ext_count = dvz_window_host_required_extension_count(app->host, app->options.backend);
    if (ext_count > 0)
    {
        const char** extensions = dvz_calloc(ext_count, sizeof(char*));
        if (extensions == NULL)
        {
            dvz_fprintf(stderr, "failed to allocate backend extension list\\n");
            return false;
        }
        int written = dvz_window_host_required_extensions(
            app->host, app->options.backend, ext_count, extensions);
        if (written != (int)ext_count)
        {
            dvz_fprintf(stderr, "failed to query required backend extensions\\n");
            dvz_free((void*)extensions);
            return false;
        }
        for (uint32_t i = 0; i < ext_count; i++)
        {
            dvz_instance_config_request_extension(&icfg, extensions[i]);
        }
        dvz_free((void*)extensions);
    }
    else if (app->options.backend != DVZ_BACKEND_OFFSCREEN)
    {
        dvz_fprintf(stderr, "requested backend exposes no Vulkan instance extensions\\n");
        return false;
    }

    app->instance = dvz_instance_create(&icfg);
    if (app->instance == NULL)
    {
        dvz_fprintf(stderr, "failed to create Vulkan instance\\n");
        return false;
    }

    DvzGpuInfo gpu_info = {0};
    uint32_t gpu_count = 0;
    if (!dvz_testing_gpu_selection_resolve(
            app->instance, &app->options.gpu_selection, &gpu_info, &gpu_count))
    {
        if (gpu_count == 0)
            dvz_fprintf(stderr, "no Vulkan GPU available\n");
        else
            dvz_fprintf(
                stderr, "GPU index %u is unavailable (available count=%u)\n",
                app->options.gpu_selection.requested_index, gpu_count);
        return false;
    }
    const uint32_t gpu_index = gpu_info.index;
    dvz_fprintf(
        stderr, "GPU %u: %s (source=%s)\n", gpu_index, gpu_info.name,
        dvz_testing_gpu_source_name(app->options.gpu_selection.source));

    DvzQueueCaps caps = {0};
    if (!dvz_instance_gpu_queue_caps(app->instance, gpu_index, &caps))
    {
        dvz_fprintf(stderr, "failed to query Vulkan queue capabilities\\n");
        return false;
    }

    DvzQueues queues = {0};
    dvz_queues(&caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_config(app->instance);
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    VkPhysicalDeviceVulkan12Features fet12 = {0};
    fet12.timelineSemaphore = true;
    dvz_device_config_set_features12(&dcfg, &fet12);
    VkPhysicalDeviceVulkan13Features fet13 = {0};
    fet13.synchronization2 = true;
    fet13.dynamicRendering = true;
    dvz_device_config_set_features13(&dcfg, &fet13);

    if (app->options.backend == DVZ_BACKEND_GLFW)
    {
        dvz_device_config_enable_canvas_extensions(&dcfg, true);
    }
    app->device = dvz_device_create(&dcfg);
    if (app->device == NULL)
    {
        dvz_fprintf(stderr, "failed to create Vulkan device\\n");
        return false;
    }

    app->allocator = dvz_allocator_create();
    if (app->allocator == NULL || dvz_device_allocator(app->device, 0, app->allocator) != 0)
    {
        dvz_fprintf(stderr, "failed to create Vulkan allocator\\n");
        return false;
    }

    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width = app->options.width;
    wcfg.height = app->options.height;
    wcfg.title = app->options.title;
    app->window = dvz_window_create(app->host, app->options.backend, &wcfg);
    if (app->window == NULL || dvz_window_backend_type(app->window) != app->options.backend)
    {
        dvz_fprintf(stderr, "failed to create requested window backend\\n");
        return false;
    }
#if DVZ_HAS_GLFW
    if (app->options.backend == DVZ_BACKEND_GLFW)
    {
        GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(app->window);
        dvz_window_set_user_data(app->window, app);
        if (handle != NULL)
        {
            glfwShowWindow(handle);
#ifdef GLFW_FOCUS_ON_SHOW
            glfwSetWindowAttrib(handle, GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
#endif
            glfwSetKeyCallback(handle, _dvz_canvas_glfw_key_callback);
            glfwFocusWindow(handle);
            if (glfwGetWindowAttrib(handle, GLFW_FOCUSED) == GLFW_FALSE)
            {
                glfwRequestWindowAttention(handle);
            }
        }
    }
#endif

    if (app->options.backend == DVZ_BACKEND_OFFSCREEN && app->options.duration_s <= 0.0 &&
        app->options.max_frames == 0)
    {
        app->options.duration_s = 5.0;
        dvz_fprintf(stderr, "offscreen backend selected, defaulting duration to %.1fs\\n", 5.0);
    }

    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.window = app->window;
    ccfg.device = app->device;
    ccfg.render_mode = app->options.render_mode;
    ccfg.present_mode = app->options.present_mode;
    ccfg.enable_video_sink = false;
    if (app->options.draw_mode == DVZ_CANVAS_DRAW_SCENE_DRP2)
        ccfg.depth_format = VK_FORMAT_D32_SFLOAT;
    app->canvas = dvz_canvas_create(&ccfg);
    if (app->canvas == NULL)
    {
        dvz_fprintf(stderr, "failed to create canvas\\n");
        return false;
    }

    if (app->options.draw_mode == DVZ_CANVAS_DRAW_SCENE_DRP2)
    {
        if (!_dvz_canvas_scene_data_init(app))
        {
            dvz_fprintf(
                stderr, "failed to allocate %u scene benchmark points (maximum %u)\n",
                app->options.scene_point_count, DVZ_CANVAS_SCENE_MAX_POINTS);
            return false;
        }
        DvzDrp2RuntimeConfig runtime_cfg =
            dvz_drp2_runtime_vklite_config(app->device, app->allocator);
        app->drp2_runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        app->scene_emitter = dvz_frame_plan_emitter();
        if (app->drp2_runtime == NULL || app->scene_emitter == NULL)
        {
            dvz_fprintf(stderr, "failed to create scene/DRP2 runtime\\n");
            return false;
        }
        app->scene_caps = dvz_capability_snapshot();
        app->scene_emit_cfg = dvz_frame_plan_emit_config();
        app->scene_emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
        app->scene_emit_cfg.external_color_target = true;
        app->scene_emit_cfg.color_target_id = 1;
        _dvz_drp2_runtime_timing_enable(app->drp2_runtime, app->options.benchmark);
        dvz_canvas_set_draw_callback(app->canvas, _dvz_canvas_draw_scene_drp2, app);
    }
    else
    {
        dvz_canvas_set_draw_callback(app->canvas, _dvz_canvas_draw, app->options.bg);
    }

    DvzInputRouter* router = dvz_canvas_input(app->canvas);
    if (router != NULL)
    {
        app->keyboard_subscription_id =
            dvz_input_subscribe_keyboard(router, _dvz_canvas_keyboard, app);
    }
    return true;
}



/**
 * Destroy all runtime objects owned by the app.
 *
 * @param app app state to cleanup
 */
static void _dvz_canvas_destroy(DvzCanvasApp* app)
{
    if (app == NULL)
    {
        return;
    }
    if (app->benchmark_frame_ms != NULL)
    {
        dvz_free(app->benchmark_frame_ms);
        app->benchmark_frame_ms = NULL;
    }
    if (app->benchmark_scene_timings != NULL)
    {
        dvz_free(app->benchmark_scene_timings);
        app->benchmark_scene_timings = NULL;
    }
    dvz_free(app->scene_point_sizes);
    app->scene_point_sizes = NULL;
    dvz_free(app->scene_point_colors);
    app->scene_point_colors = NULL;
    dvz_free(app->scene_point_positions);
    app->scene_point_positions = NULL;
    if (app->canvas != NULL)
    {
        DvzInputRouter* router = dvz_canvas_input(app->canvas);
        if (router != NULL)
        {
            dvz_input_unsubscribe(router, app->keyboard_subscription_id);
            app->keyboard_subscription_id = DVZ_CALLBACK_ID_NONE;
        }
        dvz_canvas_set_draw_callback(app->canvas, NULL, NULL);
    }
    if (app->scene_cached_stream != NULL)
    {
        dvz_drp2_stream_destroy(app->scene_cached_stream);
        app->scene_cached_stream = NULL;
    }
    if (app->scene_cached_plan != NULL)
    {
        dvz_frame_plan_destroy(app->scene_cached_plan);
        app->scene_cached_plan = NULL;
    }
    if (app->scene_emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(app->scene_emitter);
        app->scene_emitter = NULL;
    }
    if (app->drp2_runtime != NULL)
    {
        dvz_drp2_runtime_destroy(app->drp2_runtime);
        app->drp2_runtime = NULL;
    }
    if (app->canvas != NULL)
    {
        dvz_canvas_destroy(app->canvas);
        app->canvas = NULL;
    }
    if (app->window != NULL)
    {
        dvz_window_destroy(app->window);
        app->window = NULL;
    }
    if (app->host != NULL)
    {
        dvz_window_host_destroy(app->host);
        app->host = NULL;
    }
    if (app->device != NULL)
    {
        if (app->allocator != NULL)
        {
            dvz_allocator_destroy(app->allocator);
            dvz_allocator_free(app->allocator);
            app->allocator = NULL;
        }
        dvz_device_destroy(app->device);
        app->device = NULL;
    }
    if (app->instance != NULL)
    {
        dvz_instance_destroy(app->instance);
        app->instance = NULL;
    }
}



/**
 * Run the interactive render loop.
 *
 * @param app initialized app state
 * @returns process exit code (0 on success)
 */
static int _dvz_canvas_run(DvzCanvasApp* app)
{
    ANN(app);
    ANN(app->host);
    ANN(app->canvas);

    time_t start_time = time(NULL);
    uint64_t submitted_frames = 0;
    bool run_ok = true;

    if (app->options.start_recording)
    {
        _dvz_canvas_toggle_recording(app);
    }
    if (!_dvz_canvas_benchmark_begin(app))
    {
        return 1;
    }

    while (app->running)
    {
        dvz_window_host_poll(app->host);
        _dvz_canvas_poll_keyboard_shortcuts(app);
        if (_dvz_canvas_window_should_close(app))
        {
            break;
        }

        if (app->toggle_record_requested)
        {
            app->toggle_record_requested = false;
            _dvz_canvas_toggle_recording(app);
        }

        if (app->screenshot_requested)
        {
            app->screenshot_requested = false;
            char path[1024] = {0};
            _dvz_canvas_screenshot_path(app, path, sizeof(path));
            if (dvz_canvas_capture_png(app->canvas, path) == 0)
            {
                dvz_fprintf(stderr, "screenshot: %s\n", path);
            }
            else
            {
                dvz_fprintf(stderr, "failed to capture screenshot\n");
            }
        }

        int frame_rc = dvz_canvas_frame(app->canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        if (frame_rc != DVZ_CANVAS_FRAME_READY)
        {
            dvz_fprintf(stderr, "canvas frame error: %d\\n", frame_rc);
            run_ok = false;
            break;
        }
        if (dvz_canvas_submit(app->canvas) != 0)
        {
            dvz_fprintf(stderr, "canvas submit error\\n");
            run_ok = false;
            break;
        }
        if (app->scene_failed)
        {
            run_ok = false;
            break;
        }
        _dvz_canvas_report_present_mode(app);
        submitted_frames++;
        _dvz_canvas_benchmark_record(app, submitted_frames);
        if (app->options.max_frames > 0 && submitted_frames >= app->options.max_frames)
        {
            break;
        }

        if (app->options.duration_s > 0.0)
        {
            time_t now = time(NULL);
            double elapsed = difftime(now, start_time);
            if (elapsed >= app->options.duration_s)
            {
                break;
            }
        }
    }

    if (app->device != NULL)
    {
        dvz_device_wait(app->device);
    }
    bool benchmark_ok = _dvz_canvas_benchmark_end(app);
    return run_ok && benchmark_ok ? 0 : 1;
}



/*************************************************************************************************/
/*  Entry point                                                                                  */
/*************************************************************************************************/

/**
 * Entry point for the interactive canvas smoke executable.
 *
 * @param argc argument count
 * @param argv argument vector
 * @returns process exit code
 */
int main(int argc, char** argv)
{
    DvzCanvasApp app = {0};
    log_set_level_env();
    _dvz_canvas_options_default(&app.options);
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            _dvz_canvas_usage();
            return 0;
        }
    }
    if (!_dvz_canvas_parse_args(argc, argv, &app.options))
    {
        return 1;
    }

    if (!_dvz_canvas_init(&app))
    {
        _dvz_canvas_destroy(&app);
        return 1;
    }

    int rc = _dvz_canvas_run(&app);
    _dvz_canvas_destroy(&app);
    return rc;
}
