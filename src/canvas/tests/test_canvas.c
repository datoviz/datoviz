/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas tests                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"
#include <sys/stat.h>
#include <stdlib.h>
#if OS_UNIX
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/canvas.h"
#include "datoviz/input/keycodes.h"
#include "datoviz/video.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/swapchain.h"
#include "datoviz/window.h"
#include "_test_canvas_probe.h"
#include "test_canvas.h"
#include "testing.h"

/*************************************************************************************************/
/*  Refresh probe                                                                                */
/*************************************************************************************************/

typedef struct CanvasLiveProbeState
{
    uint32_t callback_count;
    uint64_t last_wait_value;
    uint32_t non_monotonic_wait_count;
    uint32_t zero_extent_count;
} CanvasLiveProbeState;



typedef struct CanvasSubmitFailState
{
    bool fail_submit;
    uint32_t submit_count;
} CanvasSubmitFailState;



typedef struct CanvasLiveGapProbeState
{
    uint32_t callback_count;
    uint64_t last_wait_value;
    uint32_t non_increasing_wait_count;
} CanvasLiveGapProbeState;



/**
 * Track stream-frame metadata observed by the refresh probe sink.
 *
 * @param state probe state to update
 * @param frame stream frame observed by start/update callbacks
 * @param is_update true for update callback, false for start callback
 * @return 0 on success
 */
static int
canvas_refresh_probe_apply_frame(
    CanvasRefreshProbeState* state, const DvzStreamFrame* frame, bool is_update)
{
    ANN(state);
    ANN(frame);

    state->latest_memory_fd = frame->memory_fd;
    state->latest_wait_semaphore_fd = frame->wait_semaphore_fd;
    state->latest_extent = frame->extent;
    state->latest_handles_dirty = frame->handles_dirty;
    if (is_update)
    {
        state->update_count++;
        if (frame->handles_dirty)
        {
            state->saw_update_since_refresh = true;
        }
    }
    else
    {
        state->start_count++;
    }
    return 0;
}



/**
 * Count and validate metadata events observed via the live-image sink callback.
 *
 * @param frame callback frame payload
 * @param user_data callback state
 * @returns 0 on success
 */
static int canvas_live_probe_callback(const DvzCanvasLiveImageFrame* frame, void* user_data)
{
    ANN(frame);
    CanvasLiveProbeState* state = (CanvasLiveProbeState*)user_data;
    ANN(state);
    state->callback_count++;
    if (state->last_wait_value > 0 && frame->wait_value != state->last_wait_value + 1)
    {
        state->non_monotonic_wait_count++;
    }
    state->last_wait_value = frame->wait_value;
    if (frame->extent.width == 0 || frame->extent.height == 0)
    {
        state->zero_extent_count++;
    }
    return 0;
}



/**
 * Record a deterministic clear pass for offscreen canvas tests.
 *
 * @param canvas owning canvas (unused)
 * @param frame stream frame carrying command buffer and attachment view
 * @param user_data callback state (unused)
 */
static void
canvas_offscreen_clear_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    (void)user_data;
    ANN(frame);

    VkCommandBuffer cmd = frame->command_buffer;
    if (cmd == VK_NULL_HANDLE || frame->image_view == VK_NULL_HANDLE)
    {
        return;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    ANN(rendering);
    dvz_cmd_rendering_default(
        cmds, frame->image_view, frame->extent.width, frame->extent.height,
        (VkClearValue){.color.float32 = {0.08f, 0.12f, 0.16f, 1.00f}}, rendering);
    dvz_cmd_rendering_begin(cmds, rendering);
    dvz_cmd_rendering_end(cmds);
    dvz_rendering_free(rendering);
    dvz_commands_free(cmds);
}



static int canvas_live_gap_probe_callback(const DvzCanvasLiveImageFrame* frame, void* user_data)
{
    ANN(frame);
    CanvasLiveGapProbeState* state = (CanvasLiveGapProbeState*)user_data;
    ANN(state);
    if (state->last_wait_value > 0 && frame->wait_value <= state->last_wait_value)
    {
        state->non_increasing_wait_count++;
    }
    state->last_wait_value = frame->wait_value;
    state->callback_count++;
    return 0;
}



static bool canvas_submit_fail_probe(const void* config)
{
    return config != NULL;
}



static int canvas_submit_fail_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    const CanvasSubmitFailState* cfg = (const CanvasSubmitFailState*)config;
    ANN(cfg);
    CanvasSubmitFailState* state =
        (CanvasSubmitFailState*)dvz_calloc(1, sizeof(CanvasSubmitFailState));
    ANN(state);
    *state = *cfg;
    sink->backend_data = state;
    return 0;
}



static int canvas_submit_fail_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    return 0;
}



static int canvas_submit_fail_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    (void)wait_value;
    CanvasSubmitFailState* state = (CanvasSubmitFailState*)sink->backend_data;
    ANN(state);
    state->submit_count++;
    if (state->fail_submit)
    {
        state->fail_submit = false;
        return -1;
    }
    return 0;
}



static int canvas_submit_fail_stop(DvzStreamSink* sink)
{
    ANN(sink);
    return 0;
}



static int canvas_submit_fail_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    return 0;
}



static void canvas_submit_fail_destroy(DvzStreamSink* sink)
{
    if (!sink || !sink->backend_data)
    {
        return;
    }
    dvz_free(sink->backend_data);
    sink->backend_data = NULL;
}



static const DvzStreamSinkBackend CANVAS_SUBMIT_FAIL_BACKEND = {
    .name = "canvas_submit_fail_probe",
    .probe = canvas_submit_fail_probe,
    .create = canvas_submit_fail_create,
    .start = canvas_submit_fail_start,
    .submit = canvas_submit_fail_submit,
    .stop = canvas_submit_fail_stop,
    .update = canvas_submit_fail_update,
    .destroy = canvas_submit_fail_destroy,
};



static bool canvas_refresh_probe_probe(const void* config)
{
    return config != NULL;
}



static int canvas_refresh_probe_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    ANN(config);
    void* state = NULL;
    dvz_memcpy(&state, sizeof(state), &config, sizeof(config));
    sink->backend_data = state;
    return 0;
}



static int canvas_refresh_probe_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    CanvasRefreshProbeState* state = (CanvasRefreshProbeState*)sink->backend_data;
    return canvas_refresh_probe_apply_frame(state, frame, false);
}



static int canvas_refresh_probe_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    CanvasRefreshProbeState* state = (CanvasRefreshProbeState*)sink->backend_data;
    ANN(state);
    state->submit_count++;
    state->wait_value_count++;
    if (state->last_wait_value > 0 && wait_value != state->last_wait_value + 1)
    {
        state->wait_value_non_monotonic++;
    }
    state->last_wait_value = wait_value;
    if (state->awaiting_refresh)
    {
        if (!state->saw_update_since_refresh)
        {
            state->stale_submit_count++;
        }
        else
        {
            state->awaiting_refresh = false;
        }
    }
    return 0;
}



/**
 * Create a default Vulkan instance/device pair for offscreen canvas tests.
 *
 * @param[out] out_instance destination instance pointer
 * @param[out] out_device destination device pointer
 * @param[out] skip_reason optional skip reason when initialization fails
 * @return true on success, false when setup is unavailable
 */
static bool canvas_test_create_instance_device(
    DvzInstance** out_instance, DvzDevice** out_device, const char** skip_reason)
{
    ANN(out_instance);
    ANN(out_device);
    if (skip_reason != NULL)
    {
        *skip_reason = NULL;
    }

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    if (instance == NULL)
    {
        if (skip_reason != NULL)
        {
            *skip_reason = "Vulkan instance creation failed";
        }
        return false;
    }

    uint32_t gpu_count = dvz_instance_gpu_count(instance);
    if (gpu_count == 0)
    {
        if (skip_reason != NULL)
        {
            *skip_reason = "no Vulkan GPU found";
        }
        dvz_instance_destroy(instance);
        return false;
    }

    DvzQueueCaps caps = {0};
    if (!dvz_instance_gpu_queue_caps(instance, 0, &caps))
    {
        if (skip_reason != NULL)
        {
            *skip_reason = "failed to query Vulkan GPU queue capabilities";
        }
        dvz_instance_destroy(instance);
        return false;
    }

    DvzQueues queues = {0};
    dvz_queues(&caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, 0);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }

    VkPhysicalDeviceVulkan12Features fet12 = {0};
    fet12.timelineSemaphore = true;
    dvz_device_config_set_features12(&dcfg, &fet12);

    VkPhysicalDeviceVulkan13Features features = {0};
    features.synchronization2 = true;
    features.dynamicRendering = true;
    dvz_device_config_set_features13(&dcfg, &features);

    DvzDevice* device = dvz_device_create(&dcfg);
    if (device == NULL)
    {
        if (skip_reason != NULL)
        {
            *skip_reason = "Vulkan device creation failed";
        }
        dvz_instance_destroy(instance);
        return false;
    }

    *out_instance = instance;
    *out_device = device;
    return true;
}



/**
 * Destroy an instance/device pair created for canvas tests.
 *
 * @param instance instance pointer
 * @param device device pointer
 */
static void canvas_test_destroy_instance_device(DvzInstance* instance, DvzDevice* device)
{
    if (device != NULL)
    {
        dvz_device_destroy(device);
    }
    if (instance != NULL)
    {
        dvz_instance_destroy(instance);
    }
}



static int canvas_refresh_probe_stop(DvzStreamSink* sink)
{
    ANN(sink);
    return 0;
}



static int canvas_refresh_probe_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    CanvasRefreshProbeState* state = (CanvasRefreshProbeState*)sink->backend_data;
    return canvas_refresh_probe_apply_frame(state, frame, true);
}



static void canvas_refresh_probe_destroy(DvzStreamSink* sink)
{
    ANN(sink);
    sink->backend_data = NULL;
}



const DvzStreamSinkBackend CANVAS_REFRESH_PROBE_BACKEND = {
    .name = "canvas_refresh_probe",
    .probe = canvas_refresh_probe_probe,
    .create = canvas_refresh_probe_create,
    .start = canvas_refresh_probe_start,
    .submit = canvas_refresh_probe_submit,
    .stop = canvas_refresh_probe_stop,
    .update = canvas_refresh_probe_update,
    .destroy = canvas_refresh_probe_destroy,
};



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Validate the default canvas configuration.
 */
int test_canvas_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasConfig cfg = dvz_canvas_config();
    AT(cfg.window == NULL);
    AT(cfg.device == NULL);
    AT(cfg.render_mode == DVZ_CANVAS_RENDER_MODE_PRESENT);
    AT(cfg.color_format == VK_FORMAT_UNDEFINED);
    AT(cfg.present_mode == VK_PRESENT_MODE_FIFO_KHR);
    AT(!cfg.enable_video_sink);
    AT(cfg.timing_history == DVZ_CANVAS_DEFAULT_TIMING_HISTORY);
    return 0;
}



/**
 * Ensure the frame pool rotates across entries.
 */
int test_canvas_frame_pool(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasFramePool pool = {0};
    dvz_canvas_frame_pool_init(&pool, 3);
    DvzStreamFrame* a = dvz_canvas_frame_pool_current(&pool);
    DvzStreamFrame* b = dvz_canvas_frame_pool_rotate(&pool);
    DvzStreamFrame* c = dvz_canvas_frame_pool_rotate(&pool);
    AT(a != NULL);
    AT(b != NULL);
    AT(c != NULL);
    AT(a != b);
    AT(b != c);
    dvz_canvas_frame_pool_release(&pool);
    return 0;
}



/**
 * Check that timing samples wrap around the configured capacity.
 */
int test_canvas_timings(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasTimingState timings = {0};
    dvz_canvas_timings_init(&timings, 4);
    for (uint64_t i = 0; i < 6; ++i)
    {
        dvz_canvas_timings_record(&timings, i, 100.0 + (double)i);
    }
    size_t count = 0;
    const DvzFrameTiming* samples = dvz_canvas_timings_view(&timings, &count);
    AT(samples != NULL);
    AT(count == 4);
    dvz_canvas_timings_release(&timings);
    return 0;
}



/**
 * Validate offscreen canvas destroy/recreate on the same device and window setup.
 */
int test_canvas_offscreen_destroy_recreate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_recreate_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-destroy-recreate";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_recreate_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;

    for (uint32_t i = 0; i < 2; i++)
    {
        canvas = dvz_canvas_create(&cfg);
        AT(canvas != NULL);
        dvz_canvas_set_draw_callback(canvas, canvas_offscreen_clear_draw, NULL);
        AT(dvz_canvas_render_mode(canvas) == DVZ_CANVAS_RENDER_MODE_OFFSCREEN);
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
        AT(dvz_canvas_submit(canvas) == 0);
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
        dvz_canvas_destroy(canvas);
        canvas = NULL;
    }

offscreen_recreate_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate GLFW present canvas destroy/recreate on the same device and window setup.
 */
int test_canvas_glfw_destroy_recreate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto glfw_recreate_cleanup;
    }

    host = dvz_window_host();
    if (host == NULL)
    {
        skip_reason = "window host creation failed";
        goto glfw_recreate_cleanup;
    }
    if (!dvz_window_glfw_init())
    {
        skip_reason = "GLFW initialization failed";
        goto glfw_recreate_cleanup;
    }

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-glfw-destroy-recreate";
    window_cfg.width = 320;
    window_cfg.height = 240;
    tst_expect_log_begin(suite, LOG_ERROR);
    window = dvz_window_create(host, DVZ_BACKEND_GLFW, &window_cfg);
    (void)tst_expect_error_end(suite);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        skip_reason = "GLFW window creation failed";
        goto glfw_recreate_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT;
    cfg.timing_history = 4;

    for (uint32_t i = 0; i < 2; i++)
    {
        canvas = dvz_canvas_create(&cfg);
        if (canvas == NULL)
        {
            skip_reason = "canvas creation failed";
            goto glfw_recreate_cleanup;
        }
        AT(dvz_canvas_render_mode(canvas) == DVZ_CANVAS_RENDER_MODE_PRESENT);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            skip_reason = "surface unavailable";
            goto glfw_recreate_cleanup;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        dvz_canvas_destroy(canvas);
        canvas = NULL;
    }

glfw_recreate_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}

/**
 * Validate first-class offscreen mode frame/submit flow on a headless window backend.
 */
static int test_canvas_offscreen_mode_headless(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);
    // Ensure deterministic non-zero capture content across drivers by explicitly recording a clear pass.
    dvz_canvas_set_draw_callback(canvas, canvas_offscreen_clear_draw, NULL);
    AT(dvz_canvas_render_mode(canvas) == DVZ_CANVAS_RENDER_MODE_OFFSCREEN);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
    DvzVideoSinkConfig external_cfg = dvz_video_sink_config();
    external_cfg.capture_mode = DVZ_VIDEO_CAPTURE_EXTERNAL;
    int external_video_rc = -1;
    bool external_supported =
#if OS_UNIX
        canvas->allocator != NULL && dvz_allocator_external(canvas->allocator) != 0 &&
        canvas->supports_external_semaphore;
#else
        false;
#endif
    if (external_supported)
    {
        external_video_rc = dvz_canvas_configure_video_sink(canvas, true, &external_cfg);
        AT(external_video_rc == 0);
        AT(dvz_canvas_configure_video_sink(canvas, false, NULL) == 0);
    }
    else
    {
        AT_EXPECTED_ERROR(
            suite,
            (external_video_rc = dvz_canvas_configure_video_sink(canvas, true, &external_cfg)) < 0);
    }
    CanvasLiveProbeState live_probe = {0};
    DvzCanvasLiveImageSinkConfig live_cfg = {
        .callback = canvas_live_probe_callback,
        .user_data = &live_probe,
    };
    AT(dvz_canvas_configure_live_image_sink(canvas, true, &live_cfg) == 0);

    for (uint32_t i = 0; i < 3; ++i)
    {
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
        int frame_rc = dvz_canvas_frame(canvas);
        AT(frame_rc != DVZ_CANVAS_FRAME_WAIT_SURFACE);
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
        if (i == 0)
        {
            uint32_t pending_width = 0;
            uint32_t pending_height = 0;
            uint8_t* pending_rgba = NULL;
            AT_EXPECTED_ERROR(
                suite,
                dvz_canvas_capture_rgba(
                    canvas, &pending_width, &pending_height, &pending_rgba) != 0);
            AT(pending_rgba == NULL);
        }
        AT(dvz_canvas_submit(canvas) == 0);
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
    }
    AT(live_probe.callback_count == 3);
    AT(live_probe.non_monotonic_wait_count == 0);
    AT(live_probe.zero_extent_count == 0);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 320);
    AT(height == 240);
    uint64_t sum = 0;
    for (size_t i = 0; i < (size_t)width * (size_t)height * 4; ++i)
    {
        sum += rgba[i];
    }
    AT(sum > 0);
    dvz_free(rgba);

offscreen_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen video sink CPU-readback contract with capability-gated skip behavior.
 */
static int test_canvas_offscreen_video_sink_cpu_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    bool enabled = false;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_video_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-video-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_video_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    DvzVideoSinkConfig sink_cfg = dvz_video_sink_config();
    sink_cfg.capture_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;
    sink_cfg.encoder.backend = "auto";
    sink_cfg.encoder.width = 320;
    sink_cfg.encoder.height = 240;
    sink_cfg.encoder.fps = 30;
    sink_cfg.encoder.mux = DVZ_VIDEO_MUX_NONE;
    sink_cfg.encoder.mp4_path = "/tmp/dvz_canvas_offscreen_video_test.mp4";
    sink_cfg.encoder.raw_path = "/tmp/dvz_canvas_offscreen_video_test.h26x";
    if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) != 0)
    {
        skip_reason = "video backend unavailable";
        goto offscreen_video_cleanup;
    }
    enabled = true;
    AT(canvas->video_sink_enabled);
    AT(canvas->video_capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);

    bool submitted = false;
    for (uint32_t i = 0; i < 8; ++i)
    {
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc != DVZ_CANVAS_FRAME_READY)
        {
            skip_reason = "offscreen frame path unavailable";
            break;
        }
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
        if (dvz_canvas_submit(canvas) != 0)
        {
            skip_reason = "offscreen video submit failed";
            break;
        }
        AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
        submitted = true;
        break;
    }

    if (!submitted && skip_reason == NULL)
    {
        skip_reason = "offscreen submit path not reached";
    }

offscreen_video_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    else
    {
        AT(enabled);
        AT(submitted);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen state rollback to DRAW_PENDING when a stream sink submit fails.
 */
static int test_canvas_offscreen_state_on_stream_submit_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_submit_fail_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-submit-fail-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_submit_fail_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    CanvasSubmitFailState fail_state = {
        .fail_submit = true,
        .submit_count = 0,
    };
    CanvasRefreshProbeState refresh_probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &refresh_probe) == 0);
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_SUBMIT_FAIL_BACKEND, &fail_state) == 0);

    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
    AT(dvz_canvas_submit(canvas) < 0);
    AT(refresh_probe.wait_value_count >= 1);
    AT(refresh_probe.last_wait_value == 1);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(refresh_probe.wait_value_count >= 2);
    AT(refresh_probe.last_wait_value == 2);
    AT(refresh_probe.wait_value_non_monotonic == 0);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);

offscreen_submit_fail_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen runtime enters FATAL_DEVICE_LOST after a forced submit device-loss status.
 */
static int test_canvas_offscreen_state_device_lost(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    bool capture_active = false;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_device_lost_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-device-lost-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_device_lost_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING);

    tst_log_capture_begin(suite);
    capture_active = true;
    dvz_canvas_test_force_offscreen_submit_status(canvas, VK_ERROR_DEVICE_LOST);
    tst_expect_error_begin(suite);
    int submit_rc = dvz_canvas_submit(canvas);
    AT(tst_expect_error_end(suite) == 0);
    if (
        submit_rc >= 0 ||
        dvz_canvas_offscreen_runtime_state(canvas) != DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST)
    {
        skip_reason = "forced offscreen device-loss submit path unavailable";
        goto offscreen_device_lost_cleanup;
    }

    tst_expect_error_begin(suite);
    AT(dvz_canvas_frame(canvas) < 0);
    AT(tst_expect_error_end(suite) == 0);
    tst_expect_error_begin(suite);
    AT(dvz_canvas_submit(canvas) < 0);
    AT(tst_expect_error_end(suite) == 0);
    AT(tst_log_capture_count(suite) >= 3);

offscreen_device_lost_cleanup:
    if (capture_active)
    {
        tst_log_capture_end(suite);
    }
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen frames keep clean handle metadata across stream rebuild toggles.
 */
static int test_canvas_offscreen_handles_clean_after_rebuild(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_clean_handles_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-clean-handles-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_clean_handles_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    DvzCanvasLiveImageSinkConfig live_cfg = {
        .callback = canvas_live_probe_callback,
        .user_data = &(CanvasLiveProbeState){0},
    };
    AT(dvz_canvas_configure_live_image_sink(canvas, true, &live_cfg) == 0);

    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
        AT(frame != NULL);
        AT(!frame->handles_dirty);
        AT(dvz_canvas_submit(canvas) == 0);
    }

    AT(dvz_canvas_configure_live_image_sink(canvas, false, NULL) == 0);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
    AT(frame != NULL);
    AT(!frame->handles_dirty);
    AT(dvz_canvas_submit(canvas) == 0);

offscreen_clean_handles_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen live-sink wait values stay strictly increasing across sink rebuild toggles.
 */
static int test_canvas_offscreen_live_wait_monotonic_across_rebuild(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_live_wait_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-live-wait-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_live_wait_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    CanvasLiveGapProbeState probe = {
        .callback_count = 0,
        .last_wait_value = 0,
        .non_increasing_wait_count = 0,
    };
    DvzCanvasLiveImageSinkConfig live_cfg = {
        .callback = canvas_live_gap_probe_callback,
        .user_data = &probe,
    };
    AT(dvz_canvas_configure_live_image_sink(canvas, true, &live_cfg) == 0);

    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }
    uint64_t wait_before_disable = probe.last_wait_value;
    AT(probe.callback_count == 2);
    AT(probe.non_increasing_wait_count == 0);

    AT(dvz_canvas_configure_live_image_sink(canvas, false, NULL) == 0);
    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }

    AT(dvz_canvas_configure_live_image_sink(canvas, true, &live_cfg) == 0);
    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }

    AT(probe.callback_count == 4);
    AT(probe.last_wait_value > wait_before_disable);
    AT(probe.non_increasing_wait_count == 0);

offscreen_live_wait_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen stream ordering: start() once per stream lifecycle and no update() calls.
 */
static int test_canvas_offscreen_start_update_order_across_rebuild(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_order_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-order-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_order_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    CanvasRefreshProbeState probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }
    AT(probe.start_count >= 1);
    AT(probe.update_count == 0);
    AT(probe.submit_count >= 2);
    AT(probe.wait_value_non_monotonic == 0);

    CanvasLiveProbeState live_probe = {0};
    DvzCanvasLiveImageSinkConfig live_cfg = {
        .callback = canvas_live_probe_callback,
        .user_data = &live_probe,
    };
    AT(dvz_canvas_configure_live_image_sink(canvas, true, &live_cfg) == 0);

    probe = (CanvasRefreshProbeState){
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(probe.start_count >= 1);
    AT(probe.update_count == 0);
    AT(probe.submit_count >= 1);
    AT(probe.wait_value_non_monotonic == 0);

    AT(dvz_canvas_configure_live_image_sink(canvas, false, NULL) == 0);

    probe = (CanvasRefreshProbeState){
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(probe.start_count >= 1);
    AT(probe.update_count == 0);
    AT(probe.submit_count >= 1);
    AT(probe.wait_value_non_monotonic == 0);

offscreen_order_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



/**
 * Validate offscreen video-sink rebuild keeps wait values strictly increasing.
 */
static int test_canvas_offscreen_video_wait_monotonic_across_rebuild(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto offscreen_video_wait_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-video-wait-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto offscreen_video_wait_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    cfg.timing_history = 4;
    canvas = dvz_canvas_create(&cfg);
    AT(canvas != NULL);

    CanvasRefreshProbeState probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    DvzVideoSinkConfig sink_cfg = dvz_video_sink_config();
    sink_cfg.capture_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;
    sink_cfg.encoder.backend = "auto";
    sink_cfg.encoder.width = 320;
    sink_cfg.encoder.height = 240;
    sink_cfg.encoder.fps = 30;
    sink_cfg.encoder.mux = DVZ_VIDEO_MUX_NONE;
    sink_cfg.encoder.mp4_path = "/tmp/dvz_canvas_offscreen_video_wait_test.mp4";
    sink_cfg.encoder.raw_path = "/tmp/dvz_canvas_offscreen_video_wait_test.h26x";
    if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) != 0)
    {
        skip_reason = "video backend unavailable";
        goto offscreen_video_wait_cleanup;
    }
    probe = (CanvasRefreshProbeState){
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }
    uint64_t wait_before_disable = probe.last_wait_value;
    AT(probe.wait_value_count >= 2);
    AT(probe.wait_value_non_monotonic == 0);

    AT(dvz_canvas_configure_video_sink(canvas, false, NULL) == 0);

    probe = (CanvasRefreshProbeState){
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);
    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }
    AT(probe.wait_value_count >= 2);
    AT(probe.wait_value_non_monotonic == 0);

    if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) != 0)
    {
        skip_reason = "video backend unavailable after rebuild";
        goto offscreen_video_wait_cleanup;
    }
    probe = (CanvasRefreshProbeState){
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);
    for (uint32_t i = 0; i < 2; ++i)
    {
        AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
    }
    AT(probe.wait_value_count >= 2);
    AT(probe.wait_value_non_monotonic == 0);
    AT(probe.last_wait_value > wait_before_disable);

offscreen_video_wait_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}
/**
 * Validate that present mode cannot be created on an offscreen window backend.
 */
static int test_canvas_present_mode_rejects_offscreen_window(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzWindowHost* host = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;

    if (!canvas_test_create_instance_device(&instance, &device, &skip_reason))
    {
        goto guard_cleanup;
    }

    host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_config();
    window_cfg.title = "canvas-offscreen-guard-test";
    window_cfg.width = 320;
    window_cfg.height = 240;
    window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        skip_reason = "headless window creation failed";
        goto guard_cleanup;
    }

    DvzCanvasConfig cfg = dvz_canvas_config();
    cfg.window = window;
    cfg.device = device;
    cfg.render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT;
    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    canvas = dvz_canvas_create(&cfg);
    AT(tst_expect_error_end(suite) == 0);
    tst_log_capture_end(suite);
    AT(canvas == NULL);

guard_cleanup:
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }
    if (canvas != NULL)
    {
        dvz_canvas_destroy(canvas);
    }
    if (window != NULL)
    {
        dvz_window_destroy(window);
    }
    if (host != NULL)
    {
        dvz_window_host_destroy(host);
    }
    canvas_test_destroy_instance_device(instance, device);
    return 0;
}



#define TST_CANVAS_VK_RES (TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN)
#define TST_CANVAS_GLFW_RES (TST_CANVAS_VK_RES | TST_RES_GLFW)
#define TST_CANVAS_VIDEO_RES (TST_RES_VIDEO | TST_RES_FILESYSTEM)

#define TST_CANVAS_CASE(test, resource_flags, isolation_mode)                                    \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = (resource_flags);                                                   \
        _tst_desc.isolation = (isolation_mode);                                                   \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)



/**
 * Register the canvas tests.
 */
int test_canvas(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "canvas";
    TST_MODULE(suite, tags);
    TST_CANVAS_CASE(test_canvas_defaults, TST_RES_CPU, TST_ISOLATION_THREAD_SAFE);
    TST_CANVAS_CASE(test_canvas_frame_pool, TST_RES_CPU, TST_ISOLATION_THREAD_SAFE);
    TST_CANVAS_CASE(test_canvas_timings, TST_RES_CPU, TST_ISOLATION_THREAD_SAFE);
    TST_CANVAS_CASE(
        test_canvas_offscreen_destroy_recreate, TST_CANVAS_VK_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(test_canvas_glfw_destroy_recreate, TST_CANVAS_GLFW_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(test_canvas_offscreen_mode_headless, TST_CANVAS_VK_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_video_sink_cpu_readback,
        TST_CANVAS_VK_RES | TST_CANVAS_VIDEO_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_state_on_stream_submit_failure, TST_CANVAS_VK_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_state_device_lost,
        TST_CANVAS_VK_RES | TST_RES_LOG_CAPTURE | TST_RES_GLOBAL_STATE,
        TST_ISOLATION_EXCLUSIVE);
    TST_CANVAS_CASE(
        test_canvas_offscreen_handles_clean_after_rebuild, TST_CANVAS_VK_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_live_wait_monotonic_across_rebuild, TST_CANVAS_VK_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_start_update_order_across_rebuild, TST_CANVAS_VK_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_offscreen_video_wait_monotonic_across_rebuild,
        TST_CANVAS_VK_RES | TST_CANVAS_VIDEO_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_present_mode_rejects_offscreen_window,
        TST_CANVAS_VK_RES | TST_RES_LOG_CAPTURE, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_swapchain_failfast_slot_init,
        TST_CANVAS_GLFW_RES | TST_RES_GLOBAL_STATE, TST_ISOLATION_EXCLUSIVE);
    TST_CANVAS_CASE(test_canvas_glfw_present_recovery, TST_CANVAS_GLFW_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(test_canvas_handle_refresh_order, TST_CANVAS_GLFW_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_video_wait_value_propagation,
        TST_CANVAS_GLFW_RES | TST_RES_VIDEO, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_video_wait_handle_ready_on_first_start,
        TST_CANVAS_GLFW_RES | TST_RES_VIDEO, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_video_wait_handle_export_fallback,
        TST_CANVAS_GLFW_RES | TST_RES_VIDEO | TST_RES_GLOBAL_STATE, TST_ISOLATION_EXCLUSIVE);
    TST_CANVAS_CASE(
        test_canvas_video_wait_handle_export_fallback_after_recreate,
        TST_CANVAS_GLFW_RES | TST_RES_VIDEO | TST_RES_GLOBAL_STATE, TST_ISOLATION_EXCLUSIVE);
    TST_CANVAS_CASE(
        test_canvas_video_sink_start_submit_integration,
        TST_CANVAS_GLFW_RES | TST_CANVAS_VIDEO_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_video_sink_disable_rebuild,
        TST_CANVAS_GLFW_RES | TST_CANVAS_VIDEO_RES, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_capture_api,
        TST_CANVAS_GLFW_RES | TST_RES_FILESYSTEM | TST_RES_LOG_CAPTURE,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_video_handle_refresh_after_recreate,
        TST_CANVAS_GLFW_RES | TST_RES_VIDEO, TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_device_lost_fatal_transition,
        TST_CANVAS_GLFW_RES | TST_RES_LOG_CAPTURE | TST_RES_GLOBAL_STATE,
        TST_ISOLATION_EXCLUSIVE);
    TST_CANVAS_CASE(
        test_canvas_glfw_wrap_surface_present_recovery, TST_CANVAS_GLFW_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_glfw_wrap_surface_resize_recreate_refreshes_state, TST_CANVAS_GLFW_RES,
        TST_ISOLATION_PROCESS);
    TST_CANVAS_CASE(
        test_canvas_glfw,
        TST_CANVAS_GLFW_RES | TST_CANVAS_VIDEO_RES | TST_RES_ENV, TST_ISOLATION_PROCESS);
    return 0;
}
