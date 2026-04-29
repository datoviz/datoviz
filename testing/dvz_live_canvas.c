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

#if DVZ_HAS_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CANVAS_DEFAULT_BG_R 0.08f
#define DVZ_CANVAS_DEFAULT_BG_G 0.12f
#define DVZ_CANVAS_DEFAULT_BG_B 0.16f
#define DVZ_CANVAS_DEFAULT_BG_A 1.00f
#define DVZ_CANVAS_DEFAULT_FPS  60
#define DVZ_CANVAS_DEFAULT_WIDTH 1024
#define DVZ_CANVAS_DEFAULT_HEIGHT 640



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_CANVAS_DRAW_CLEAR,
    DVZ_CANVAS_DRAW_SCENE_DRP2,
} DvzCanvasDrawMode;


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
    bool start_recording;
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
    uint64_t scene_frame_index;
    bool scene_last_ok;
    bool scene_reported_ok;
    bool scene_reported_error;
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
    options->start_recording = false;
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
 * Print command-line usage information.
 */
static void _dvz_canvas_usage(void)
{
    dvz_fprintf(
        stderr,
        "usage: dvz_live_canvas [--backend glfw|offscreen] [--width N] [--height N]\n"
        "                  [--mode present|offscreen] [--frames N] [--bg r,g,b,a] [--fps N]\n"
        "                  [--draw clear|scene-drp2]\n"
        "                  [--present fifo|immediate] [--duration seconds]\n"
        "                  [--record path.mp4] [--record-mode auto|external|cpu]\n"
        "                  [--start-recording] [--screenshots base]\n"
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
    if (options->backend == DVZ_BACKEND_OFFSCREEN &&
        options->render_mode == DVZ_CANVAS_RENDER_MODE_PRESENT)
    {
        dvz_fprintf(
            stderr, "invalid combination: --backend offscreen requires --mode offscreen\\n");
        return false;
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

    DvzRendering* rendering = dvz_rendering_create();
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
 * Render a minimal scene through DRP2 into the current canvas frame.
 *
 * @param canvas owning canvas
 * @param frame stream frame with command buffer and target image view
 * @param user_data app state pointer
 */
static void _dvz_canvas_draw_scene_drp2(
    DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzCanvasApp* app = (DvzCanvasApp*)user_data;
    ANN(app);
    app->scene_last_ok = false;
    if (app->drp2_runtime == NULL || app->scene_emitter == NULL)
    {
        return;
    }

    DvzFramePlan* plan = dvz_frame_plan("live.canvas.scene", app->scene_frame_index++);
    if (plan == NULL)
    {
        return;
    }

    const char* stage = "attach frame target";
    DvzDrp2ValidationResult result = {0};

    bool ok = dvz_drp2_runtime_attach_frame_target(app->drp2_runtime, 1, frame);
    ok = ok && dvz_frame_plan_upload(plan, "buf.live.position", 0, 16, "live.position");
    ok = ok && dvz_frame_plan_render(plan, "panel.0", "target.canvas.color", false);
    ok = ok && dvz_frame_plan_render_visual(plan, "visual.live.fullscreen");

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = NULL;
    if (ok)
    {
        stage = "emit DRP2 stream";
        stream = dvz_frame_plan_emitter_emit_drp2(
            app->scene_emitter, plan, &app->scene_caps, &report, &app->scene_emit_cfg);
        ok = stream != NULL && dvz_diagnostic_report_count(&report) == 0;
    }
    if (ok)
    {
        stage = "execute DRP2 stream";
        result = dvz_drp2_runtime_execute(app->drp2_runtime, stream);
        ok = result.ok && result.code == DVZ_DRP2_VALIDATION_OK;
    }

    app->scene_last_ok = ok;
    if (ok && !app->scene_reported_ok)
    {
        dvz_fprintf(
            stderr, "scene-drp2: rendering into canvas target %ux%u\n", frame->extent.width,
            frame->extent.height);
        app->scene_reported_ok = true;
    }
    if (!ok && !app->scene_reported_error)
    {
        dvz_fprintf(
            stderr,
            "scene-drp2: failed to %s (diagnostics=%u, validation=%d at command %u)\n", stage,
            dvz_diagnostic_report_count(&report), (int)result.code, result.command_index);
        app->scene_reported_error = true;
    }
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
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
        app->video_cfg = dvz_video_sink_default_config();
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
static void _dvz_canvas_keyboard(
    DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
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
static void _dvz_canvas_glfw_key_callback(
    GLFWwindow* handle, int key, int scancode, int action, int mods)
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

    DvzInstanceConfig icfg = dvz_instance_default_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    uint32_t ext_count =
        dvz_window_host_required_extension_count(app->host, app->options.backend);
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

    uint32_t gpu_count = dvz_instance_gpu_count(app->instance);
    if (gpu_count == 0)
    {
        dvz_fprintf(stderr, "no Vulkan GPU available\\n");
        return false;
    }

    DvzQueueCaps caps = {0};
    if (!dvz_instance_gpu_queue_caps(app->instance, 0, &caps))
    {
        dvz_fprintf(stderr, "failed to query Vulkan queue capabilities\\n");
        return false;
    }

    DvzQueues queues = {0};
    dvz_queues(&caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(app->instance);
    dvz_device_config_set_gpu_index(&dcfg, 0);
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

    DvzWindowConfig wcfg = dvz_window_default_config();
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

    if (
        app->options.backend == DVZ_BACKEND_OFFSCREEN && app->options.duration_s <= 0.0 &&
        app->options.max_frames == 0)
    {
        app->options.duration_s = 5.0;
        dvz_fprintf(stderr, "offscreen backend selected, defaulting duration to %.1fs\\n", 5.0);
    }

    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window = app->window;
    ccfg.device = app->device;
    ccfg.render_mode = app->options.render_mode;
    ccfg.present_mode = app->options.present_mode;
    ccfg.enable_video_sink = false;
    app->canvas = dvz_canvas_create(&ccfg);
    if (app->canvas == NULL)
    {
        dvz_fprintf(stderr, "failed to create canvas\\n");
        return false;
    }

    if (app->options.draw_mode == DVZ_CANVAS_DRAW_SCENE_DRP2)
    {
        DvzDrp2RuntimeConfig runtime_cfg =
            dvz_drp2_runtime_vklite_config(app->device, app->allocator);
        app->drp2_runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        app->scene_emitter = dvz_frame_plan_emitter();
        if (app->drp2_runtime == NULL || app->scene_emitter == NULL)
        {
            dvz_fprintf(stderr, "failed to create scene/DRP2 runtime\\n");
            return false;
        }
        dvz_capability_snapshot_default(&app->scene_caps);
        app->scene_emit_cfg = dvz_frame_plan_emit_config();
        app->scene_emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
        app->scene_emit_cfg.external_color_target = true;
        app->scene_emit_cfg.color_target_id = 1;
        app->scene_emit_cfg.fullscreen_triangle = true;
        dvz_canvas_set_draw_callback(app->canvas, _dvz_canvas_draw_scene_drp2, app);
    }
    else
    {
        dvz_canvas_set_draw_callback(app->canvas, _dvz_canvas_draw, app->options.bg);
    }

    DvzInputRouter* router = dvz_canvas_input(app->canvas);
    if (router != NULL)
    {
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
    if (app->canvas != NULL)
    {
        DvzInputRouter* router = dvz_canvas_input(app->canvas);
        if (router != NULL)
        {
            dvz_input_unsubscribe_keyboard(router, _dvz_canvas_keyboard, app);
        }
        dvz_canvas_set_draw_callback(app->canvas, NULL, NULL);
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

    if (app->options.start_recording)
    {
        _dvz_canvas_toggle_recording(app);
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
            break;
        }
        if (dvz_canvas_submit(app->canvas) != 0)
        {
            dvz_fprintf(stderr, "canvas submit error\\n");
            break;
        }
        submitted_frames++;
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
    return 0;
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
