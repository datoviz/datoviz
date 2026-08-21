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
#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../presentation_policy.h"
#include "../_status.h"
#include "../_trace.h"
#include "../../drp2/_stream.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#include "datoviz_testing.h"
#include "test_app.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AppTestWindowBackendState
{
    uint32_t create_count;
    uint32_t destroy_count;
} AppTestWindowBackendState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _test_restore_env(const char* name, const char* value)
{
    ANN(name);
    if (value != NULL)
        (void)tst_setenv(name, value);
    else
        (void)tst_unsetenv(name);
}



/**
 * Return an app config that does not request instance extensions for borrowed GPU contexts.
 *
 * @return app config with GPU-extension requests disabled
 */
static DvzAppConfig _test_app_resource_config(void)
{
    DvzAppConfig config = dvz_app_config();
    config.instance_extension_count = 0;
    config.instance_extensions = NULL;
    config.enable_canvas_extensions = false;
    config.enable_glfw_extensions = false;
    return config;
}



/**
 * Create a GPU context with the same baseline features as the default app path.
 *
 * @return GPU context, or NULL when Vulkan setup is unavailable
 */
static DvzGpuCtx* _test_app_gpu_ctx(const TstContext* suite)
{
    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return dvz_gpu_ctx(&gpu_cfg);
}



/**
 * Create a test backend window and record the creation.
 *
 * @param backend backend descriptor
 * @param window window being created
 * @param config window configuration
 * @return true
 */
static bool _test_app_backend_create(
    DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)window;
    (void)config;
    ANN(backend);
    AppTestWindowBackendState* state = (AppTestWindowBackendState*)backend->user_data;
    ANN(state);
    state->create_count++;
    return true;
}



/**
 * Destroy a test backend window and record the destruction.
 *
 * @param backend backend descriptor
 * @param window window being destroyed
 */
static void _test_app_backend_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)window;
    ANN(backend);
    AppTestWindowBackendState* state = (AppTestWindowBackendState*)backend->user_data;
    ANN(state);
    state->destroy_count++;
}



/**
 * Register a CPU-only test backend on a window host.
 *
 * @param host window host receiving the backend
 * @param state backend state storing callback counts
 */
static void _test_app_register_backend(DvzWindowHost* host, AppTestWindowBackendState* state)
{
    ANN(host);
    ANN(state);
    DvzWindowBackend backend = {
        .name = "app-test",
        .type = DVZ_BACKEND_QT,
        .user_data = state,
        .procs = {
            .create = _test_app_backend_create,
            .destroy = _test_app_backend_destroy,
        },
    };
    dvz_window_host_register_backend(host, &backend);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
    (void)tst_unsetenv("DVZ_APP_SCHEDULE");
    (void)tst_unsetenv("DVZ_FPS_CAP");

    DvzAppConfig config = dvz_app_config();
    AT(config.instance_extension_count == 0);
    AT(config.instance_extensions == NULL);
    AT(!config.enable_canvas_extensions);
    AT(config.enable_glfw_extensions);
    AT(config.schedule_mode == DVZ_APP_SCHEDULE_ON_DEMAND);
    AT(config.exit_policy == DVZ_APP_EXIT_WHEN_ALL_WINDOWS_CLOSED);
    AT(config.present_mode == DVZ_APP_PRESENT_MODE_AUTOMATIC);
    AT(config.fps_cap == 0);
    DvzFontDefaults fonts = dvz_font_defaults();
    AT(strcmp(config.font_sans_family, fonts.sans_family) == 0);
    AT(strcmp(config.font_sans_style, fonts.sans_style) == 0);
    AT(config.font_ui_size_px == fonts.ui_size_px);
    AT(config.font_mono_size_px == fonts.mono_size_px);
    AT(config.font_text_size_px == fonts.text_size_px);

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

    AT(tst_setenv("DVZ_APP_SCHEDULE", "continuous") == 0);
    DvzAppConfig config = dvz_app_config();
    AT(config.schedule_mode == DVZ_APP_SCHEDULE_CONTINUOUS);

    AT(tst_setenv("DVZ_APP_SCHEDULE", "on_demand") == 0);
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

    AT(tst_setenv("DVZ_FPS_CAP", "144.5") == 0);
    DvzAppConfig config = dvz_app_config();
    AT(config.fps_cap == 144.5);

    _test_restore_env("DVZ_FPS_CAP", old_fps_cap != NULL ? saved_fps_cap : NULL);
    return 0;
}



static int test_app_presentation_policy_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_present_mode = getenv("DVZ_PRESENT_MODE");
    const char* old_frame_slots = getenv("DVZ_MAX_FRAMES_IN_FLIGHT");
    char saved_present_mode[64] = {0};
    char saved_frame_slots[64] = {0};
    if (old_present_mode != NULL)
        dvz_snprintf(saved_present_mode, sizeof(saved_present_mode), "%s", old_present_mode);
    if (old_frame_slots != NULL)
        dvz_snprintf(saved_frame_slots, sizeof(saved_frame_slots), "%s", old_frame_slots);
    (void)tst_unsetenv("DVZ_PRESENT_MODE");
    (void)tst_unsetenv("DVZ_MAX_FRAMES_IN_FLIGHT");

#if defined(VK_KHR_present_mode_fifo_latest_ready)
    AT(_dvz_app_present_mode_default() == VK_PRESENT_MODE_FIFO_LATEST_READY_KHR);
#else
    AT(_dvz_app_present_mode_default() == VK_PRESENT_MODE_FIFO_KHR);
#endif
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t frame_slot_count = 0;
    AT(!_dvz_app_present_mode_parse(NULL, &present_mode));
    AT(!_dvz_app_frame_slot_count_env(&frame_slot_count));

    _test_restore_env(
        "DVZ_PRESENT_MODE", old_present_mode != NULL ? saved_present_mode : NULL);
    _test_restore_env(
        "DVZ_MAX_FRAMES_IN_FLIGHT", old_frame_slots != NULL ? saved_frame_slots : NULL);
    return 0;
}



static int test_app_presentation_policy_env_overrides(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_present_mode = getenv("DVZ_PRESENT_MODE");
    const char* old_frame_slots = getenv("DVZ_MAX_FRAMES_IN_FLIGHT");
    char saved_present_mode[64] = {0};
    char saved_frame_slots[64] = {0};
    if (old_present_mode != NULL)
        dvz_snprintf(saved_present_mode, sizeof(saved_present_mode), "%s", old_present_mode);
    if (old_frame_slots != NULL)
        dvz_snprintf(saved_frame_slots, sizeof(saved_frame_slots), "%s", old_frame_slots);

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    AT(tst_setenv("DVZ_PRESENT_MODE", "mailbox") == 0);
    AT(_dvz_app_present_mode_parse(getenv("DVZ_PRESENT_MODE"), &present_mode));
    AT(present_mode == VK_PRESENT_MODE_MAILBOX_KHR);
    AT(tst_setenv("DVZ_PRESENT_MODE", "fifo-latest") == 0);
    AT(_dvz_app_present_mode_parse(getenv("DVZ_PRESENT_MODE"), &present_mode));
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    AT(present_mode == VK_PRESENT_MODE_FIFO_LATEST_READY_KHR);
#else
    AT(present_mode == VK_PRESENT_MODE_FIFO_KHR);
#endif
    AT(tst_setenv("DVZ_PRESENT_MODE", "invalid") == 0);
    AT(!_dvz_app_present_mode_parse(getenv("DVZ_PRESENT_MODE"), &present_mode));

    uint32_t frame_slot_count = 0;
    AT(tst_setenv("DVZ_MAX_FRAMES_IN_FLIGHT", "auto") == 0);
    AT(_dvz_app_frame_slot_count_env(&frame_slot_count));
    AT(frame_slot_count == DVZ_CANVAS_FRAME_SLOT_COUNT_AUTOMATIC);
    AT(tst_setenv("DVZ_MAX_FRAMES_IN_FLIGHT", "2") == 0);
    AT(_dvz_app_frame_slot_count_env(&frame_slot_count));
    AT(frame_slot_count == 2);
    AT(tst_setenv("DVZ_MAX_FRAMES_IN_FLIGHT", "0") == 0);
    AT(!_dvz_app_frame_slot_count_env(&frame_slot_count));

    _test_restore_env(
        "DVZ_PRESENT_MODE", old_present_mode != NULL ? saved_present_mode : NULL);
    _test_restore_env(
        "DVZ_MAX_FRAMES_IN_FLIGHT", old_frame_slots != NULL ? saved_frame_slots : NULL);
    return 0;
}



static int test_app_presentation_policy_config(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_present_mode = getenv("DVZ_PRESENT_MODE");
    char saved_present_mode[64] = {0};
    if (old_present_mode != NULL)
        dvz_snprintf(saved_present_mode, sizeof(saved_present_mode), "%s", old_present_mode);
    (void)tst_unsetenv("DVZ_PRESENT_MODE");

    bool explicit_mode = true;
    AT(_dvz_app_present_mode_config(
           DVZ_APP_PRESENT_MODE_AUTOMATIC, true, &explicit_mode, NULL) ==
       _dvz_app_present_mode_default());
    AT(!explicit_mode);
    AT(_dvz_app_present_mode_config(
           DVZ_APP_PRESENT_MODE_AUTOMATIC, false, &explicit_mode, NULL) ==
       VK_PRESENT_MODE_FIFO_KHR);
    AT(!explicit_mode);

    AT(_dvz_app_present_mode_resolve(DVZ_APP_PRESENT_MODE_FIFO) == VK_PRESENT_MODE_FIFO_KHR);
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    AT(_dvz_app_present_mode_resolve(DVZ_APP_PRESENT_MODE_FIFO_LATEST) ==
       VK_PRESENT_MODE_FIFO_LATEST_READY_KHR);
#else
    AT(_dvz_app_present_mode_resolve(DVZ_APP_PRESENT_MODE_FIFO_LATEST) ==
       VK_PRESENT_MODE_FIFO_KHR);
#endif
    AT(_dvz_app_present_mode_resolve(DVZ_APP_PRESENT_MODE_MAILBOX) ==
       VK_PRESENT_MODE_MAILBOX_KHR);
    AT(_dvz_app_present_mode_resolve(DVZ_APP_PRESENT_MODE_IMMEDIATE) ==
       VK_PRESENT_MODE_IMMEDIATE_KHR);

    AT(_dvz_app_present_mode_config(DVZ_APP_PRESENT_MODE_FIFO, true, &explicit_mode, NULL) ==
       VK_PRESENT_MODE_FIFO_KHR);
    AT(explicit_mode);
    AT(tst_setenv("DVZ_PRESENT_MODE", "immediate") == 0);
    const char* invalid_env_value = NULL;
    AT(_dvz_app_present_mode_config(
           DVZ_APP_PRESENT_MODE_FIFO, true, &explicit_mode, &invalid_env_value) ==
       VK_PRESENT_MODE_IMMEDIATE_KHR);
    AT(explicit_mode);
    AT(invalid_env_value == NULL);
    AT(tst_setenv("DVZ_PRESENT_MODE", "invalid") == 0);
    AT(_dvz_app_present_mode_config(
           DVZ_APP_PRESENT_MODE_FIFO, false, &explicit_mode, &invalid_env_value) ==
       VK_PRESENT_MODE_FIFO_KHR);
    AT(explicit_mode);
    AT(invalid_env_value != NULL && strcmp(invalid_env_value, "invalid") == 0);

    _test_restore_env(
        "DVZ_PRESENT_MODE", old_present_mode != NULL ? saved_present_mode : NULL);
    return 0;
}



static int test_app_presentation_policy_pacing(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzAppPacingPolicy policy = {0};
    const uint32_t refresh_rates[] = {60, 75, 120, 144};
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(refresh_rates); i++)
    {
        policy = _dvz_app_pacing_policy_resolve(true, false, 0, refresh_rates[i]);
        AT(policy.mode == DVZ_APP_PACING_REFRESH);
        AT(policy.fps_cap == (double)refresh_rates[i]);
    }
    policy = _dvz_app_pacing_policy_resolve(true, false, 0, 0);
    AT(policy.mode == DVZ_APP_PACING_REFRESH);
    AT(policy.fps_cap == 60.0);
    DvzAppPacingPolicy fifo_latest_fallback =
        _dvz_app_pacing_policy_resolve(true, false, 0, 144);
    DvzAppPacingPolicy ordinary_fifo_fallback =
        _dvz_app_pacing_policy_resolve(true, false, 0, 144);
    AT(fifo_latest_fallback.mode == ordinary_fifo_fallback.mode);
    AT(fifo_latest_fallback.fps_cap == ordinary_fifo_fallback.fps_cap);
    policy = _dvz_app_pacing_policy_resolve(true, true, 144.0, 60);
    AT(policy.mode == DVZ_APP_PACING_FIXED);
    AT(policy.fps_cap == 144.0);
    policy = _dvz_app_pacing_policy_resolve(true, true, 0, 60);
    AT(policy.mode == DVZ_APP_PACING_UNBOUNDED);
    AT(policy.fps_cap == 0);
    policy = _dvz_app_pacing_policy_resolve(false, false, 144.0, 60);
    AT(policy.mode == DVZ_APP_PACING_HOST_DRIVEN);
    AT(policy.fps_cap == 0);

    policy = _dvz_app_pacing_policy_resolve(true, false, 60.0, 0);
    uint64_t deadline = _dvz_app_pacing_policy_advance(&policy, 0, 1000);
    AT(deadline == 16667666);
    AT(!_dvz_app_pacing_policy_admits(&policy, deadline, deadline - 1));
    AT(_dvz_app_pacing_policy_admits(&policy, deadline, deadline));
    AT(_dvz_app_pacing_policy_advance(&policy, deadline, deadline) == 33334332);
    AT(_dvz_app_pacing_policy_advance(&policy, deadline, deadline + 3 * 16666666) ==
       deadline + 3 * 16666666 + 16666666);
    AT(_dvz_app_pacing_policy_advance(&policy, UINT64_MAX - 1, UINT64_MAX - 1) == UINT64_MAX);

    AT(_dvz_app_view_effective_fps_cap(144.0, VK_PRESENT_MODE_FIFO_KHR, 60) == 144.0);
    AT(_dvz_app_view_effective_fps_cap(0, VK_PRESENT_MODE_FIFO_KHR, 60) == 0);
    AT(!_dvz_app_view_requires_scheduler_poll(true, 60.0, 123));
    AT(_dvz_app_view_requires_scheduler_poll(true, 0, 123));
    AT(_dvz_app_view_requires_scheduler_poll(true, 60.0, 0));
    AT(!_dvz_app_view_requires_scheduler_poll(false, 0, 0));
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    double unknown_refresh_fps_cap = 0;
    AT(_dvz_app_view_effective_fps_cap(
           144.0, VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 60) == 144.0);
    AT(_dvz_app_view_effective_fps_cap(
           0, VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 75) == 75.0);
    AT(_dvz_app_view_effective_fps_cap(
           0, VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 120) == 120.0);
    AT(_dvz_app_view_effective_fps_cap(
           0, VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 144) == 144.0);
    unknown_refresh_fps_cap =
        _dvz_app_view_effective_fps_cap(0, VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 0);
    AT(unknown_refresh_fps_cap == 60.0);
    AT(!_dvz_app_view_requires_scheduler_poll(true, unknown_refresh_fps_cap, 123));
#endif
    return 0;
}



static int test_app_presentation_policy_scheduler_admission(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzAppPacingPolicy paced = _dvz_app_pacing_policy_resolve(true, false, 60.0, 0);
    DvzAppPacingPolicy unbounded = _dvz_app_pacing_policy_resolve(true, true, 0, 60);
    DvzAppPacingPolicy host_driven = _dvz_app_pacing_policy_resolve(false, false, 0, 60);
    uint64_t deadline = 0;

    DvzAppPacingRequest burst = {.needs_frame = true, .policy = paced};
    AT(_dvz_app_pacing_requests_deadline(1, &burst, 1000, &deadline));
    AT(deadline == 0);
    burst.next_frame_ns = _dvz_app_pacing_policy_advance(&paced, 0, 1000);
    AT(_dvz_app_pacing_requests_deadline(1, &burst, 2000, &deadline));
    AT(deadline == burst.next_frame_ns);
    AT(_dvz_app_pacing_requests_deadline(
        1, &burst, burst.next_frame_ns, &deadline));
    AT(deadline == 0);

    burst.needs_frame = false;
    AT(!_dvz_app_pacing_requests_deadline(1, &burst, 2000, &deadline));
    AT(deadline == 0);
    burst.needs_frame = true;

    DvzAppPacingRequest requests[] = {
        burst,
        {.needs_frame = true, .policy = paced, .next_frame_ns = burst.next_frame_ns + 1000},
        {.needs_frame = true, .policy = host_driven},
    };
    AT(_dvz_app_pacing_requests_deadline(
        DVZ_ARRAY_COUNT(requests), requests, 2000, &deadline));
    AT(deadline == burst.next_frame_ns);

    requests[1].policy = unbounded;
    AT(_dvz_app_pacing_requests_deadline(
        DVZ_ARRAY_COUNT(requests), requests, 2000, &deadline));
    AT(deadline == 0);

    requests[0].needs_frame = false;
    requests[1].needs_frame = false;
    AT(!_dvz_app_pacing_requests_deadline(
        DVZ_ARRAY_COUNT(requests), requests, 2000, &deadline));
    AT(deadline == 0);
    return 0;
}



static int test_app_view_size_policy_resolve(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzViewSizeDesc framebuffer = dvz_view_size_desc_framebuffer_px(640, 480);
    framebuffer.requested_device_scale = 2.0;
    DvzResolvedViewSize resolved = dvz_view_size_resolve(&framebuffer, DVZ_VIEW_OFFSCREEN);
    AT(resolved.requested_policy == DVZ_VIEW_SIZE_FRAMEBUFFER_PX);
    AT(resolved.host_logical_width == 320);
    AT(resolved.host_logical_height == 240);
    AT(resolved.framebuffer_width == 640);
    AT(resolved.framebuffer_height == 480);
    AT(fabs(resolved.framebuffer_per_canvas_px_x - 1.0) < 1e-9);
    AT(fabs(resolved.target_width_mm - 640.0 / 96.0 * 25.4) < 1e-9);
    AT(fabs(resolved.estimated_width_mm - resolved.target_width_mm) < 1e-9);

    DvzViewSizeDesc host = dvz_view_size_desc_host_logical_px(320, 240);
    resolved = dvz_view_size_resolve(&host, DVZ_VIEW_WINDOW);
    AT(resolved.requested_policy == DVZ_VIEW_SIZE_HOST_LOGICAL_PX);
    AT(resolved.host_logical_width == 320);
    AT(resolved.framebuffer_width == 320);
    AT(fabs(resolved.target_width_mm - 320.0 / 96.0 * 25.4) < 1e-9);
    AT(fabs(resolved.estimated_width_mm - resolved.target_width_mm) < 1e-9);

    DvzViewSizeDesc reference = dvz_view_size_desc_reference_px(960.0, 540.0, 96.0);
    reference.requested_device_scale = 2.0;
    resolved = dvz_view_size_resolve(&reference, DVZ_VIEW_WINDOW);
    AT(resolved.requested_policy == DVZ_VIEW_SIZE_REFERENCE_PX);
    AT(resolved.host_logical_width == 960);
    AT(resolved.host_logical_height == 540);
    AT(resolved.framebuffer_width == 1920);
    AT(resolved.framebuffer_height == 1080);
    AT(fabs(resolved.target_width_mm - 254.0) < 1e-9);
    AT(fabs(resolved.framebuffer_per_canvas_px_x - 2.0) < 1e-9);

    reference.monitor_dpi_x_override = 139.2;
    reference.monitor_dpi_y_override = 139.2;
    reference.requested_device_scale = 1.0;
    resolved = dvz_view_size_resolve(&reference, DVZ_VIEW_WINDOW);
    AT(resolved.physical_metrics_source == DVZ_PHYSICAL_METRICS_USER_OVERRIDE);
    AT(resolved.framebuffer_width == 1392);
    AT(resolved.framebuffer_height == 783);
    AT(resolved.host_logical_width == 1392);
    AT(resolved.host_logical_height == 783);
    AT(fabs(resolved.framebuffer_per_canvas_px_x - 1.45) < 1e-9);

    DvzViewSizeDesc physical = dvz_view_size_desc_physical_mm(254.0, 127.0, 96.0);
    physical.monitor_dpi_x_override = 192.0;
    physical.monitor_dpi_y_override = 192.0;
    physical.requested_device_scale = 2.0;
    resolved = dvz_view_size_resolve(&physical, DVZ_VIEW_WINDOW);
    AT(resolved.requested_policy == DVZ_VIEW_SIZE_PHYSICAL_MM);
    AT(resolved.host_logical_width == 960);
    AT(resolved.host_logical_height == 480);
    AT(resolved.framebuffer_width == 1920);
    AT(resolved.framebuffer_height == 960);
    AT(fabs(resolved.canvas_width_px - 960.0) < 1e-9);
    AT(fabs(resolved.canvas_height_px - 480.0) < 1e-9);

    return 0;
}



static int test_app_capture_config_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzAppCaptureConfig config = dvz_app_capture_config();
    AT(config.flags == DVZ_APP_CAPTURE_NONE);
    AT(strcmp(config.directory, ".") == 0);
    AT(strcmp(config.basename, "capture") == 0);
    AT(config.fps == 60.0);
    AT(strcmp(config.video_backend, "auto") == 0);
    AT(config.video_capture_mode == DVZ_VIDEO_CAPTURE_AUTO);
    return 0;
}



static int test_app_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzAppConfig config = dvz_app_config();
    config.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_app_with_config(scene, &config) == NULL);

    config = dvz_app_config();
    config.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_app_with_config(scene, &config) == NULL);

    config = dvz_app_config();
    config.present_mode = (DvzAppPresentMode)999;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_app_with_config(scene, &config) == NULL);

    DvzAppResources resources = dvz_app_resources();
    resources.struct_size = DVZ_STRUCT_SIZE(DvzAppResources) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_app_with_resources(scene, NULL, &resources) == NULL);

    resources = dvz_app_resources();
    resources.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_app_with_resources(scene, NULL, &resources) == NULL);

    dvz_scene_destroy(scene);
    return 0;
}



static int test_app_capture_config_env(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const char* old_capture = getenv("DVZ_CAPTURE");
    const char* old_dir = getenv("DVZ_CAPTURE_DIR");
    const char* old_basename = getenv("DVZ_CAPTURE_BASENAME");
    const char* old_fps = getenv("DVZ_CAPTURE_FPS");
    const char* old_backend = getenv("DVZ_CAPTURE_VIDEO_BACKEND");
    const char* old_mode = getenv("DVZ_CAPTURE_VIDEO_MODE");
    char saved_capture[128] = {0};
    char saved_dir[128] = {0};
    char saved_basename[128] = {0};
    char saved_fps[128] = {0};
    char saved_backend[128] = {0};
    char saved_mode[128] = {0};
    if (old_capture != NULL)
        dvz_snprintf(saved_capture, sizeof(saved_capture), "%s", old_capture);
    if (old_dir != NULL)
        dvz_snprintf(saved_dir, sizeof(saved_dir), "%s", old_dir);
    if (old_basename != NULL)
        dvz_snprintf(saved_basename, sizeof(saved_basename), "%s", old_basename);
    if (old_fps != NULL)
        dvz_snprintf(saved_fps, sizeof(saved_fps), "%s", old_fps);
    if (old_backend != NULL)
        dvz_snprintf(saved_backend, sizeof(saved_backend), "%s", old_backend);
    if (old_mode != NULL)
        dvz_snprintf(saved_mode, sizeof(saved_mode), "%s", old_mode);

    AT(tst_setenv("DVZ_CAPTURE", "dvzr,mp4,png") == 0);
    AT(tst_setenv("DVZ_CAPTURE_DIR", "/tmp/datoviz-capture") == 0);
    AT(tst_setenv("DVZ_CAPTURE_BASENAME", "env-name") == 0);
    AT(tst_setenv("DVZ_CAPTURE_FPS", "24") == 0);
    AT(tst_setenv("DVZ_CAPTURE_VIDEO_BACKEND", "stub") == 0);
    AT(tst_setenv("DVZ_CAPTURE_VIDEO_MODE", "cpu") == 0);

    DvzAppCaptureConfig config = dvz_app_capture_config_from_env("fallback-name");
    AT((config.flags & DVZ_APP_CAPTURE_DVZR) != 0);
    AT((config.flags & DVZ_APP_CAPTURE_VIDEO) != 0);
    AT((config.flags & DVZ_APP_CAPTURE_PNG) != 0);
    AT(strcmp(config.directory, "/tmp/datoviz-capture") == 0);
    AT(strcmp(config.basename, "env-name") == 0);
    AT(config.fps == 24.0);
    AT(strcmp(config.video_backend, "stub") == 0);
    AT(config.video_capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK);

    AT(tst_setenv("DVZ_CAPTURE", "off") == 0);
    config = dvz_app_capture_config_from_env("fallback-name");
    AT(config.flags == DVZ_APP_CAPTURE_NONE);

    _test_restore_env("DVZ_CAPTURE", old_capture != NULL ? saved_capture : NULL);
    _test_restore_env("DVZ_CAPTURE_DIR", old_dir != NULL ? saved_dir : NULL);
    _test_restore_env("DVZ_CAPTURE_BASENAME", old_basename != NULL ? saved_basename : NULL);
    _test_restore_env("DVZ_CAPTURE_FPS", old_fps != NULL ? saved_fps : NULL);
    _test_restore_env("DVZ_CAPTURE_VIDEO_BACKEND", old_backend != NULL ? saved_backend : NULL);
    _test_restore_env("DVZ_CAPTURE_VIDEO_MODE", old_mode != NULL ? saved_mode : NULL);
    return 0;
}



/**
 * Create and destroy an app that owns every top-level resource.
 */
static int test_app_resources_owned_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppConfig config = _test_app_resource_config();
    config.font_sans_family = "App Sans";
    config.font_sans_style = "Book";
    config.font_text_size_px = 17.0f;
    DvzApp* app = dvz_app_with_resources(scene, &config, NULL);
    if (app == NULL)
    {
        dvz_scene_destroy(scene);
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzFontDefaults scene_fonts = dvz_scene_font_defaults(scene);
    AT(strcmp(scene_fonts.sans_family, "App Sans") == 0);
    AT(strcmp(scene_fonts.sans_style, "Book") == 0);
    AT(scene_fonts.text_size_px == 17.0f);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Report hosted-loop exit state from the public app helper.
 */
static int test_app_should_exit_reflects_stop_request(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppConfig config = _test_app_resource_config();
    config.exit_policy = DVZ_APP_EXIT_NEVER;
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    if (app == NULL)
    {
        dvz_scene_destroy(scene);
        dvz_gpu_ctx_destroy(gpu_ctx);
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    AT(!dvz_app_should_exit(app));
    AT(dvz_app_stop(app) == DVZ_OK);
    AT(dvz_app_should_exit(app));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(gpu_ctx);
    return 0;
}


/**
 * Public hosted-loop close cleanup is a safe no-op when no views are closed.
 */
static int test_app_reap_closed_views_noops_without_closed_views(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppConfig config = _test_app_resource_config();
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    if (app == NULL)
    {
        dvz_scene_destroy(scene);
        dvz_gpu_ctx_destroy(gpu_ctx);
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    AT(!dvz_app_reap_closed_views(app));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(gpu_ctx);
    return 0;
}



/**
 * Reject a borrowed runtime when no matching GPU context is provided.
 */
static int test_app_resources_reject_runtime_without_gpu(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzAppResources resources = dvz_app_resources();
    resources.runtime = runtime;
    DvzAppConfig config = _test_app_resource_config();

    tst_expect_error_begin(suite);
    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    AT(app == NULL);
    AT(tst_expect_error_end(suite) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Reject a runtime whose device/allocator do not match the borrowed GPU context.
 */
static int test_app_resources_reject_incompatible_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    resources.runtime = runtime;
    DvzAppConfig config = _test_app_resource_config();

    tst_expect_error_begin(suite);
    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    AT(app == NULL);
    AT(tst_expect_error_end(suite) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(gpu_ctx);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Borrow a GPU context while the app owns its runtime and window host.
 */
static int test_app_resources_borrow_gpu_ctx(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    DvzAppConfig config = _test_app_resource_config();

    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    AT(app != NULL);
    dvz_app_destroy(app);

    AT(dvz_gpu_ctx_device(gpu_ctx) != NULL);
    dvz_gpu_ctx_destroy(gpu_ctx);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Borrow a GPU context and a compatible runtime.
 */
static int test_app_resources_borrow_gpu_ctx_and_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(gpu_ctx), dvz_gpu_ctx_alloc(gpu_ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    resources.runtime = runtime;
    DvzAppConfig config = _test_app_resource_config();

    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    AT(app != NULL);
    dvz_app_destroy(app);

    DvzDrp2RuntimeConfig after_cfg = dvz_drp2_runtime_get_config(runtime);
    AT(after_cfg.device == dvz_gpu_ctx_device(gpu_ctx));
    AT(after_cfg.allocator == dvz_gpu_ctx_alloc(gpu_ctx));
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(gpu_ctx);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Borrow a GPU context and window host without destroying either in app teardown.
 */
static int test_app_resources_borrow_gpu_ctx_and_window_host(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzGpuCtx* gpu_ctx = _test_app_gpu_ctx(suite);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    AppTestWindowBackendState state = {0};
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    _test_app_register_backend(host, &state);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_QT, NULL);
    ANN(window);
    AT(state.create_count == 1);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = gpu_ctx;
    resources.window_host = host;
    DvzAppConfig config = _test_app_resource_config();

    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    AT(app != NULL);
    dvz_app_destroy(app);
    AT(state.destroy_count == 0);

    dvz_window_host_destroy(host);
    AT(state.destroy_count == 1);
    dvz_gpu_ctx_destroy(gpu_ctx);
    dvz_scene_destroy(scene);
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

    AT(dvz_drp2_stream_write_buffer_base64(a, 42, 8, 4, "AAAA"));
    AT(dvz_drp2_stream_finish_command_encoder(a, 7, 100));
    AT(dvz_drp2_stream_queue_submit(a, 100, 200));

    AT(dvz_drp2_stream_write_buffer_base64(b, 42, 8, 4, "BBBB"));
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

    AT(dvz_drp2_stream_write_buffer_base64(a, 42, 8, 4, "AAAA"));
    AT(dvz_drp2_stream_write_buffer_base64(b, 42, 12, 4, "AAAA"));

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

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        a, 42, 64, 64, DVZ_FORMAT_R8G8B8A8_UNORM, 0x12));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        b, 42, 64, 64, DVZ_FORMAT_R8G8B8A8_SNORM, 0x12));

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

    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = 10;
    pipeline.vertex_shader_module_id = 1;
    pipeline.fragment_shader_module_id = 2;
    AT(dvz_drp2_stream_create_render_pipeline(a, &pipeline));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        a, DVZ_CULL_MODE_FRONT, DVZ_FRONT_FACE_CLOCKWISE));
    AT(dvz_drp2_stream_pipeline_set_color_target(a, 0, DVZ_FORMAT_R8G8B8A8_UNORM));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        a, 0, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_SRC_COLOR,
        DVZ_BLEND_OP_REVERSE_SUBTRACT, DVZ_BLEND_FACTOR_DST_COLOR,
        DVZ_BLEND_FACTOR_ONE_MINUS_DST_COLOR, DVZ_BLEND_OP_MIN, DVZ_MASK_COLOR_ALL));

    AT(dvz_drp2_stream_create_render_pipeline(b, &pipeline));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        b, DVZ_CULL_MODE_FRONT, DVZ_FRONT_FACE_CLOCKWISE));
    AT(dvz_drp2_stream_pipeline_set_color_target(b, 0, DVZ_FORMAT_R8G8B8A8_SNORM));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        b, 0, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_SRC_COLOR,
        DVZ_BLEND_OP_REVERSE_SUBTRACT, DVZ_BLEND_FACTOR_DST_COLOR,
        DVZ_BLEND_FACTOR_ONE_MINUS_DST_COLOR, DVZ_BLEND_OP_MIN, DVZ_MASK_COLOR_ALL));

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


static int test_app_trace_snapshot_normalizes_scoped_product_resources(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzDrp2CommandStream* a = dvz_drp2_stream();
    DvzDrp2CommandStream* b = dvz_drp2_stream();
    ANN(a);
    ANN(b);

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        a, 27, 800, 600, DVZ_FORMAT_D32_SFLOAT, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 27, "fig0_p0.surface.depth_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        a, 28, 800, 600, DVZ_FORMAT_B8G8R8A8_UNORM, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 28, "fig0_p0.surface.color_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5017, 5016, 28, 26));
    AT(dvz_drp2_stream_set_label(a, 5017, "_bg_product_resolve_28_27_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 28, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(a, 27, 1));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5017));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        b, 29, 800, 600, DVZ_FORMAT_D32_SFLOAT, 0x14));
    AT(dvz_drp2_stream_set_label(
        b, 29, "fig0_p0.surface.depth_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        b, 30, 800, 600, DVZ_FORMAT_B8G8R8A8_UNORM, 0x14));
    AT(dvz_drp2_stream_set_label(
        b, 30, "fig0_p0.surface.color_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5021, 5016, 30, 26));
    AT(dvz_drp2_stream_set_label(b, 5021, "_bg_product_resolve_30_29_26"));
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
           &sa, "+ texture id=fig0_p0.surface.color size=800x600x1 usage=0x14") == 0);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa,
           "render#0 target=fig0_p0.surface.color clear=yes depth=yes "
           "depth_target=fig0_p0.surface.depth area=(0,0 1x1)") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "pass#0 bind[0]=_bg_product_resolve_#_#_#") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_normalizes_generic_product_bind_groups(
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
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5034, 5019, 34, 26));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5034));
    AT(dvz_drp2_stream_set_label(a, 5034, "_bg_product_sample_34_33_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 100));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 101, 7, 36, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        a, 36, "fig0_p0.ambient.visibility_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5035, 5019, 36, 26));
    AT(dvz_drp2_stream_set_bind_group(a, 101, 0, 5035));
    AT(dvz_drp2_stream_set_label(a, 5035, "_bg_product_filter_35_34_33_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 101));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 102, 7, 5000, 0, 0, 0, 1, 0, 0, 1, 1, false));
    AT(dvz_drp2_stream_set_label(a, 5000, "rt"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5036, 5019, 36, 26));
    AT(dvz_drp2_stream_set_bind_group(a, 102, 0, 5036));
    AT(dvz_drp2_stream_set_label(a, 5036, "_bg_product_composite_36_26"));
    AT(dvz_drp2_stream_end_render_pass(a, 102));

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 200, 8, 39, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 39, "fig0_p0.gbuffer.normal_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(b, 38, 1));
    AT(dvz_drp2_stream_set_label(
        b, 38, "fig0_p0.gbuffer.depth_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5037, 5019, 39, 26));
    AT(dvz_drp2_stream_set_bind_group(b, 200, 0, 5037));
    AT(dvz_drp2_stream_set_label(b, 5037, "_bg_product_sample_39_38_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 200));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 201, 8, 41, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 41, "fig0_p0.ambient.visibility_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5038, 5019, 41, 26));
    AT(dvz_drp2_stream_set_bind_group(b, 201, 0, 5038));
    AT(dvz_drp2_stream_set_label(b, 5038, "_bg_product_filter_40_39_38_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 201));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 202, 8, 5000, 0, 0, 0, 1, 0, 0, 1, 1, false));
    AT(dvz_drp2_stream_set_label(b, 5000, "rt"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5039, 5019, 41, 26));
    AT(dvz_drp2_stream_set_bind_group(b, 202, 0, 5039));
    AT(dvz_drp2_stream_set_label(b, 5039, "_bg_product_composite_41_26"));
    AT(dvz_drp2_stream_end_render_pass(b, 202));

    DvzAppTraceSnapshot sa;
    DvzAppTraceSnapshot sb;
    _dvz_app_trace_snapshot_init(&sa);
    _dvz_app_trace_snapshot_init(&sb);
    AT(_dvz_app_trace_snapshot_build(&sa, a));
    AT(_dvz_app_trace_snapshot_build(&sb, b));

    AT(_dvz_app_trace_snapshot_equal(&sa, &sb));
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "pass#0 bind[0]=_bg_product_sample_#_#_#") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "pass#1 bind[0]=_bg_product_filter_#_#_#_#") == 1);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "pass#2 bind[0]=_bg_product_composite_#_#") == 1);

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

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        a, 34, 100, 100, DVZ_FORMAT_D32_SFLOAT, 0x14));
    AT(dvz_drp2_stream_set_label(
        a, 34, "fig0_p0.gbuffer.normal_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(a, 5034, 5019, 34, 26));
    AT(dvz_drp2_stream_set_label(a, 5034, "_bg_product_sample_34_33_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        a, 100, 7, 34, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 0, 5034));
    AT(dvz_drp2_stream_set_label(a, 6001, "_bg_material_v2"));
    AT(dvz_drp2_stream_set_bind_group(a, 100, 1, 6001));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_set_label(
        b, 35, "fig0_p0.gbuffer.normal_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(b, 5035, 5019, 35, 26));
    AT(dvz_drp2_stream_set_label(b, 5035, "_bg_product_sample_35_33_26"));
    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 101, 8, 35, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_bind_group(b, 101, 0, 5035));
    AT(dvz_drp2_stream_set_label(b, 6002, "_bg_material_v2"));
    AT(dvz_drp2_stream_set_bind_group(b, 101, 1, 6002));
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
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "+ bind-group id=_bg_product_sample_#_#_#") == 0);
    AT(_dvz_app_trace_snapshot_line_count(
           &sa, "pass#0 bind[0]=_bg_product_sample_#_#_#") == 1);
    AT(_dvz_app_trace_snapshot_line_count(&sa, "pass#0 bind[1]=_bg_material_v2") == 1);

    _dvz_app_trace_snapshot_destroy(&sa);
    _dvz_app_trace_snapshot_destroy(&sb);
    dvz_drp2_stream_destroy(a);
    dvz_drp2_stream_destroy(b);
    return 0;
}



static int test_app_trace_snapshot_keeps_scoped_product_draw_payload(
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
        a, 28, "fig0_p0.surface.color_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(a, 27, 1));
    AT(dvz_drp2_stream_set_label(
        a, 27, "fig0_p0.surface.depth_scope_aaaaaaaaaaaaaaaa"));
    AT(dvz_drp2_stream_draw(a, 100, 3, 1, 0, 0));

    AT(dvz_drp2_stream_begin_render_pass_region_clear(
        b, 104, 8, 30, 0, 0, 0, 1, 0, 0, 1, 1, true));
    AT(dvz_drp2_stream_set_label(
        b, 30, "fig0_p0.surface.color_scope_bbbbbbbbbbbbbbbb"));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(b, 29, 1));
    AT(dvz_drp2_stream_set_label(
        b, 29, "fig0_p0.surface.depth_scope_bbbbbbbbbbbbbbbb"));
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

#define TST_APP_GPU_CASE(test)                                                                   \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;                                     \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_APP_GPU_DEFAULT_EXEMPT_CASE(test)                                                    \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        /* This case deliberately verifies the production-owned default GPU path. */             \
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_EXEMPT;                                        \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

    TST_CASE(test_app_config_defaults);
    TST_CASE(test_app_config_env_schedule);
    TST_CASE(test_app_config_env_fps_cap);
    TST_CASE(test_app_presentation_policy_defaults);
    TST_CASE(test_app_presentation_policy_env_overrides);
    TST_CASE(test_app_presentation_policy_config);
    TST_CASE(test_app_presentation_policy_pacing);
    TST_CASE(test_app_presentation_policy_scheduler_admission);
    TST_CASE(test_app_view_size_policy_resolve);
    TST_CASE(test_app_capture_config_defaults);
    TST_CASE(test_app_abi_rejects_invalid_structs);
    TST_CASE(test_app_capture_config_env);
    TST_APP_GPU_DEFAULT_EXEMPT_CASE(test_app_resources_owned_defaults);
    TST_APP_GPU_CASE(test_app_should_exit_reflects_stop_request);
    TST_APP_GPU_CASE(test_app_reap_closed_views_noops_without_closed_views);
    TST_CASE(test_app_resources_reject_runtime_without_gpu);
    TST_APP_GPU_CASE(test_app_resources_reject_incompatible_runtime);
    TST_APP_GPU_CASE(test_app_resources_borrow_gpu_ctx);
    TST_APP_GPU_CASE(test_app_resources_borrow_gpu_ctx_and_runtime);
    TST_APP_GPU_CASE(test_app_resources_borrow_gpu_ctx_and_window_host);
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
    TST_CASE(test_app_trace_snapshot_normalizes_scoped_product_resources);
    TST_CASE(test_app_trace_snapshot_normalizes_generic_product_bind_groups);
    TST_CASE(test_app_trace_snapshot_ignores_transient_scoped_creates);
    TST_CASE(test_app_trace_snapshot_keeps_scoped_product_draw_payload);
    TST_CASE(test_app_trace_snapshot_rejects_truncated_suffix);
    TST_CASE(test_app_trace_snapshot_recovers_after_failed_build);

#undef TST_APP_GPU_CASE
#undef TST_APP_GPU_DEFAULT_EXEMPT_CASE
    return 0;
}
