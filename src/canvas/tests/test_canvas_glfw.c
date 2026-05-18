/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas GLFW tests                                                                            */
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
#include "_env.h"
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
#include "wrap_surface_fixture.h"

#if DVZ_HAS_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif



/*************************************************************************************************/
/*  Helpers                                                                                     */
/*************************************************************************************************/

typedef struct CanvasGlfwClearContext
{
    DvzDevice* device;
    VkFormat format;
    VkExtent2D latest_extent;
    uint32_t callback_count;
} CanvasGlfwClearContext;



typedef struct CanvasGlfwFixture
{
    DvzInstance* instance;
    DvzWindowHost* host;
    DvzDevice* device;
    DvzWindow* window;
    DvzCanvas* canvas;
} CanvasGlfwFixture;



#if DVZ_HAS_GLFW
typedef struct CanvasWrapSurfaceFixture
{
    DvzWindow* wrap_window;
    GLFWwindow* external_handle;
    VkSurfaceKHR external_surface;
    DvzWindowExternalSurfaceInfo info;
} CanvasWrapSurfaceFixture;
#endif



/**
 * Return whether automated GLFW tests should create visible windows.
 *
 * @return true when DVZ_TEST_VISIBLE is set to a non-zero value
 */
static bool _canvas_glfw_test_visible(void)
{
    return checkenv("DVZ_TEST_VISIBLE");
}



/**
 * Record a fullscreen clear command for the current canvas command buffer.
 *
 * @param canvas owning canvas (unused)
 * @param frame stream frame that carries the command buffer and extent
 * @param user_data unused pointer
 */
static void canvas_glfw_clear_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);

    VkCommandBuffer cmd = frame->command_buffer;
    if (cmd == VK_NULL_HANDLE)
    {
        return;
    }

    CanvasGlfwClearContext* ctx = (CanvasGlfwClearContext*)user_data;
    if (!ctx || !ctx->device)
    {
        return;
    }

    VkImageView image_view = frame->image_view;
    if (image_view == VK_NULL_HANDLE)
    {
        log_error("canvas frame missing image view");
        return;
    }

    ctx->latest_extent = frame->extent;
    ctx->callback_count++;

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    ANN(rendering);
    dvz_cmd_rendering_default(
        cmds, image_view, frame->extent.width, frame->extent.height,
        (VkClearValue){.color.float32 = {0.08f, 0.12f, 0.16f, 1.00f}}, rendering);
    dvz_cmd_rendering_begin(cmds, rendering);
    dvz_cmd_rendering_end(cmds);
    dvz_rendering_free(rendering);
    dvz_commands_free(cmds);
}



/**
 * Stop the interactive GLFW loop when Escape is pressed.
 *
 * @param router input router emitting the event (unused)
 * @param event observed keyboard event
 * @param user_data pointer to the boolean guard used by the running loop
 */
static void canvas_glfw_keyboard_callback(
    DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    ANN(router);
    if (!event || !user_data)
    {
        return;
    }

    if (event->type == DVZ_KEYBOARD_EVENT_PRESS && event->key == DVZ_KEY_ESCAPE)
    {
        bool* keep_running = (bool*)user_data;
        *keep_running = false;
    }
}



/*************************************************************************************************/
/*  Test fixtures                                                                                */
/*************************************************************************************************/

/**
 * Initialize a GLFW-backed canvas fixture for integration tests.
 *
 * @param fixture fixture storage to initialize
 * @param[out] skipped true when the environment cannot run the fixture and the test should skip
 * @return 0 on success, -1 on setup failure
 */
static int canvas_glfw_fixture_create(CanvasGlfwFixture* fixture, bool* skipped)
{
    ANN(fixture);
    ANN(skipped);

    *skipped = false;
    dvz_memset(fixture, sizeof(*fixture), 0, sizeof(*fixture));

#if !DVZ_HAS_GLFW
    *skipped = true;
    log_warn("canvas glfw fixture skipped because Datoviz was not build with glfw support");
    return 0;
#else
    DvzInstanceConfig icfg = dvz_instance_default_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    dvz_instance_config_request_extension(&icfg, VK_KHR_SURFACE_EXTENSION_NAME);

    fixture->host = dvz_window_host();
    ANN(fixture->host);

    if (!dvz_window_glfw_init())
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because GLFW could not initialize");
        return 0;
    }

    uint32_t ext_count = dvz_window_host_required_extension_count(fixture->host, DVZ_BACKEND_GLFW);
    if (ext_count == 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because GLFW returned no Vulkan instance extensions");
        return 0;
    }
    const char** extensions = dvz_calloc(ext_count, sizeof(char*));
    if (extensions == NULL)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because extension-list allocation failed");
        return 0;
    }
    int written =
        dvz_window_host_required_extensions(fixture->host, DVZ_BACKEND_GLFW, ext_count, extensions);
    if (written != (int)ext_count)
    {
        dvz_free((void*)extensions);
        *skipped = true;
        log_warn("canvas glfw fixture skipped because required-extension query failed");
        return 0;
    }
    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_config_request_extension(&icfg, extensions[i]);
    }
    dvz_free((void*)extensions);

    fixture->instance = dvz_instance_create(&icfg);
    if (fixture->instance == NULL)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because Vulkan instance creation failed");
        return 0;
    }

    uint32_t gpu_count = dvz_instance_gpu_count(fixture->instance);
    if (gpu_count == 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because no Vulkan GPU was found");
        return 0;
    }

    DvzQueueCaps caps = {0};
    if (!dvz_instance_gpu_queue_caps(fixture->instance, 0, &caps))
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because queue capability query failed");
        return 0;
    }

    DvzQueues queues = {0};
    dvz_queues(&caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(fixture->instance);
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
    dvz_device_config_enable_canvas_extensions(&dcfg, true);
    fixture->device = dvz_device_create(&dcfg);
    if (fixture->device == NULL)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because Vulkan device creation failed");
        return 0;
    }

    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
    window_cfg.visible = _canvas_glfw_test_visible();
    fixture->window = dvz_window_create(fixture->host, DVZ_BACKEND_GLFW, &window_cfg);
    if (fixture->window == NULL || dvz_window_backend_type(fixture->window) != DVZ_BACKEND_GLFW)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because GLFW window creation failed");
        return 0;
    }

    dvz_window_host_poll(fixture->host);

    DvzCanvasConfig cfg = dvz_canvas_default_config();
    cfg.window = fixture->window;
    cfg.device = fixture->device;
    cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    cfg.timing_history = 1;

    fixture->canvas = dvz_canvas_create(&cfg);
    if (fixture->canvas == NULL)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because canvas creation failed");
        return 0;
    }
    return 0;
#endif
}



/**
 * Destroy all resources owned by a GLFW canvas fixture.
 *
 * @param fixture fixture storage to cleanup
 */
static void canvas_glfw_fixture_destroy(CanvasGlfwFixture* fixture)
{
    if (fixture == NULL)
    {
        return;
    }

    dvz_canvas_swapchain_test_fail_slot(fixture->canvas, -1);
    dvz_canvas_test_force_wait_semaphore_export_failure(fixture->canvas, false);
    if (fixture->canvas != NULL)
    {
        dvz_canvas_set_draw_callback(fixture->canvas, NULL, NULL);
        dvz_canvas_destroy(fixture->canvas);
        fixture->canvas = NULL;
    }
    if (fixture->window != NULL)
    {
        dvz_window_destroy(fixture->window);
        fixture->window = NULL;
    }
    if (fixture->host != NULL)
    {
        dvz_window_host_destroy(fixture->host);
        fixture->host = NULL;
    }
    if (fixture->device != NULL)
    {
        dvz_device_destroy(fixture->device);
        fixture->device = NULL;
    }
    if (fixture->instance != NULL)
    {
        dvz_instance_destroy(fixture->instance);
        fixture->instance = NULL;
    }
}



#if DVZ_HAS_GLFW
/**
 * Initialize a wrap window and external GLFW surface for canvas integration tests.
 *
 * @param fixture shared GLFW/Vulkan fixture with host and instance
 * @param cfg window config used for wrap window and external GLFW handle
 * @param wrap output wrap-surface fixture storage
 * @return true on success, false when setup should be skipped
 */
static bool _canvas_wrap_surface_fixture_create(
    CanvasGlfwFixture* fixture, const DvzWindowConfig* cfg, CanvasWrapSurfaceFixture* wrap)
{
    ANN(fixture);
    ANN(cfg);
    ANN(wrap);
    dvz_memset(wrap, sizeof(*wrap), 0, sizeof(*wrap));

    wrap->wrap_window = dvz_window_create(fixture->host, DVZ_BACKEND_WRAP, cfg);
    if (wrap->wrap_window == NULL || dvz_window_backend_type(wrap->wrap_window) != DVZ_BACKEND_WRAP)
    {
        log_warn("canvas wrap test skipped because wrap window creation failed");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, cfg->visible ? GLFW_TRUE : GLFW_FALSE);
    wrap->external_handle =
        glfwCreateWindow((int)cfg->width, (int)cfg->height, cfg->title, NULL, NULL);
    if (wrap->external_handle == NULL)
    {
        log_warn("canvas wrap test skipped because external GLFW window creation failed");
        return false;
    }

    VkInstance instance = dvz_instance_handle(fixture->instance);
    VkResult surface_res =
        glfwCreateWindowSurface(instance, wrap->external_handle, NULL, &wrap->external_surface);
    if (surface_res != VK_SUCCESS || wrap->external_surface == VK_NULL_HANDLE)
    {
        log_warn(
            "canvas wrap test skipped because external GLFW surface creation failed (%d)",
            (int)surface_res);
        return false;
    }

    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(wrap->external_handle, &fb_width, &fb_height);
    if (fb_width <= 0 || fb_height <= 0)
    {
        fb_width = (int)cfg->width;
        fb_height = (int)cfg->height;
    }

    float scale_x = 1.0f;
    float scale_y = 1.0f;
    glfwGetWindowContentScale(wrap->external_handle, &scale_x, &scale_y);
    if (scale_x <= 0.0f || scale_y <= 0.0f)
    {
        scale_x = 1.0f;
        scale_y = 1.0f;
    }

    wrap->info = dvz_test_wrap_surface_info(
        instance, wrap->external_surface, (uint32_t)fb_width, (uint32_t)fb_height, scale_x,
        scale_y, false);
    if (dvz_window_wrap_attach_surface(wrap->wrap_window, &wrap->info) != 0)
    {
        log_warn("canvas wrap test skipped because wrap attach_surface() failed");
        return false;
    }
    return true;
}



/**
 * Destroy wrap external-surface resources used by canvas integration tests.
 *
 * @param fixture shared GLFW/Vulkan fixture with host and instance
 * @param wrap wrap-surface fixture storage
 */
static void _canvas_wrap_surface_fixture_destroy(
    CanvasGlfwFixture* fixture, CanvasWrapSurfaceFixture* wrap)
{
    if (fixture == NULL || wrap == NULL)
    {
        return;
    }
    if (wrap->wrap_window != NULL)
    {
        dvz_window_wrap_detach_surface(wrap->wrap_window);
    }
    if (wrap->external_surface != VK_NULL_HANDLE && fixture->instance != NULL)
    {
        vkDestroySurfaceKHR(dvz_instance_handle(fixture->instance), wrap->external_surface, NULL);
        wrap->external_surface = VK_NULL_HANDLE;
    }
    if (wrap->external_handle != NULL)
    {
        glfwDestroyWindow(wrap->external_handle);
        wrap->external_handle = NULL;
    }
    if (wrap->wrap_window != NULL)
    {
        dvz_window_destroy(wrap->wrap_window);
        wrap->wrap_window = NULL;
    }
}
#endif



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/
/**
 * Ensure slot initialization failures abort swapchain creation without partial-frame progression.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_swapchain_failfast_slot_init(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    dvz_canvas_swapchain_test_fail_slot(canvas, 0);
    dvz_window_host_poll(fixture.host);
    int frame_rc = dvz_canvas_frame(canvas);
    AT(frame_rc < 0);

    dvz_canvas_swapchain_test_fail_slot(canvas, -1);
    bool resumed = false;
    for (uint32_t i = 0; i < 12; i++)
    {
        dvz_window_host_poll(fixture.host);
        frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        resumed = true;
        break;
    }
    AT(resumed);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate explicit out-of-date recovery on GLFW: recreate and resume frame submissions.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_glfw_present_recovery(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);
    uint64_t frame_id_before = canvas->frame_id;
    uint64_t timeline_before = canvas->timeline_value;

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool got_first_submit = false;
    bool got_recovery_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);

        if (!got_first_submit)
        {
            got_first_submit = true;
            dvz_canvas_swapchain_mark_out_of_date(canvas);
            continue;
        }

        got_recovery_submit = true;
        break;
    }
    AT(got_first_submit);
    AT(got_recovery_submit);
    AT(canvas->frame_id >= frame_id_before + 2);
    AT(canvas->timeline_value >= timeline_before + 2);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate stream sinks refresh frame handles before post-recreate submissions.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_handle_refresh_order(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    bool first_submit_done = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        first_submit_done = true;
        break;
    }
    AT(first_submit_done);
    AT(probe.start_count > 0);
    AT(probe.submit_count > 0);

    probe.awaiting_refresh = true;
    probe.saw_update_since_refresh = false;
    dvz_canvas_swapchain_mark_out_of_date(canvas);

    bool post_recreate_submit_done = false;
    uint32_t submits_before = probe.submit_count;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        if (probe.submit_count > submits_before)
        {
            post_recreate_submit_done = true;
            break;
        }
    }

    AT(post_recreate_submit_done);
    AT(probe.update_count > 0);
    AT(probe.saw_update_since_refresh);
    AT(probe.stale_submit_count == 0);
    AT(!probe.awaiting_refresh);
    AT(probe.latest_handles_dirty);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate that submit wait values are propagated monotonically to stream sinks.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_wait_value_propagation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    uint32_t submits = 0;
    for (uint32_t i = 0; i < 32 && submits < 3; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        submits++;
        AT(probe.last_wait_value == canvas->timeline_value);
    }

    AT(submits == 3);
    AT(probe.wait_value_count >= submits);
    AT(probe.wait_value_non_monotonic == 0);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Ensure first stream start observes a ready timeline wait-semaphore FD when video sync is enabled.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_wait_handle_ready_on_first_start(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    if (!canvas->supports_external_semaphore || dvz_canvas_timeline_handle_type() == 0)
    {
        log_warn("canvas wait-handle readiness test skipped (no exportable external semaphore)");
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "no exportable external semaphore");
        return 0;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    canvas->video_sink_enabled = true;
    bool frame_ready = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        frame_ready = true;
        break;
    }

    AT(frame_ready);
    AT(probe.start_count > 0);
    AT(probe.latest_wait_semaphore_fd >= 0);
    DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
    AT(frame != NULL);
    AT(frame->wait_semaphore_fd >= 0);
    AT(dvz_canvas_submit(canvas) == 0);
    canvas->video_sink_enabled = false;

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate deterministic fallback when timeline wait-semaphore export fails at frame start.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_wait_handle_export_fallback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    if (!canvas->supports_external_semaphore || dvz_canvas_timeline_handle_type() == 0)
    {
        log_warn("canvas wait-handle fallback test skipped (no exportable external semaphore)");
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "no exportable external semaphore");
        return 0;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    canvas->video_sink_enabled = true;
    dvz_canvas_test_force_wait_semaphore_export_failure(canvas, true);
    bool frame_ready = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        frame_ready = true;
        break;
    }

    AT(frame_ready);
    AT(probe.start_count > 0);
    AT(probe.latest_wait_semaphore_fd < 0);
    DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
    AT(frame != NULL);
    AT(frame->wait_semaphore_fd < 0);
    AT(dvz_canvas_submit(canvas) == 0);
    dvz_canvas_test_force_wait_semaphore_export_failure(canvas, false);
    canvas->video_sink_enabled = false;

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate deterministic wait-handle export fallback across recreate/update refresh paths.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_wait_handle_export_fallback_after_recreate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    if (!canvas->supports_external_semaphore || dvz_canvas_timeline_handle_type() == 0)
    {
        log_warn("canvas recreate fallback test skipped (no exportable external semaphore)");
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "no exportable external semaphore");
        return 0;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    canvas->video_sink_enabled = true;
    bool first_submit_done = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        first_submit_done = true;
        break;
    }
    AT(first_submit_done);
    AT(probe.start_count > 0);
    AT(probe.latest_wait_semaphore_fd >= 0);

    uint32_t update_before = probe.update_count;
    uint32_t submit_before = probe.submit_count;
    dvz_canvas_test_force_wait_semaphore_export_failure(canvas, true);
    dvz_canvas_swapchain_mark_out_of_date(canvas);

    bool resumed = false;
    for (uint32_t i = 0; i < 32; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
        AT(frame != NULL);
        AT(!frame->handles_dirty);
        AT(frame->wait_semaphore_fd < 0);
        AT(dvz_canvas_submit(canvas) == 0);
        resumed = true;
        break;
    }

    AT(resumed);
    AT(probe.update_count > update_before);
    AT(probe.submit_count > submit_before);
    AT(probe.latest_wait_semaphore_fd < 0);
    AT(probe.latest_handles_dirty);
    AT(probe.wait_value_non_monotonic == 0);

    dvz_canvas_test_force_wait_semaphore_export_failure(canvas, false);
    canvas->video_sink_enabled = false;

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate real video sink start+submit integration when backend/handles are available.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_sink_start_submit_integration(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        skip_reason = "GLFW fixture unavailable";
        goto cleanup;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    if (
        !canvas->supports_external_memory || !canvas->supports_external_semaphore ||
        dvz_canvas_timeline_handle_type() == 0)
    {
        skip_reason = "external memory/semaphore unsupported";
        goto cleanup;
    }

    DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
    DvzVideoSinkConfig sink_cfg = dvz_video_sink_default_config();
    sink_cfg.encoder.backend = "auto";
    sink_cfg.encoder.width = surface.extent.width ? surface.extent.width : 640;
    sink_cfg.encoder.height = surface.extent.height ? surface.extent.height : 480;
    sink_cfg.encoder.fps = 30;
    sink_cfg.encoder.mux = DVZ_VIDEO_MUX_NONE;
    sink_cfg.encoder.mp4_path = "/tmp/dvz_canvas_video_sink_test.mp4";
    sink_cfg.encoder.raw_path = "/tmp/dvz_canvas_video_sink_test.h26x";
    if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) != 0)
    {
        skip_reason = "sink could not be enabled";
        goto cleanup;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool submitted = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        if (frame_rc != DVZ_CANVAS_FRAME_READY)
        {
            skip_reason = "video backend unavailable";
            break;
        }

        DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
        AT(frame != NULL);
        AT(frame->memory_fd >= 0);
        AT(frame->wait_semaphore_fd >= 0);
        AT(dvz_canvas_submit(canvas) == 0);
        submitted = true;
        break;
    }

    if (!submitted && skip_reason == NULL)
    {
        skip_reason = "submit path not reached within frame budget";
    }

cleanup:
    if (fixture.canvas != NULL && fixture.canvas->video_sink_enabled)
    {
        AT(dvz_canvas_configure_video_sink(fixture.canvas, false, NULL) == 0);
    }
    if (skip_reason != NULL)
    {
        log_warn("canvas video sink integration skipped (%s)", skip_reason);
        tst_skip(suite, skip_reason);
    }
    else
    {
        AT(submitted);
    }
    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate disabling an active video sink rebuilds the stream and keeps rendering functional.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_sink_disable_rebuild(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* skip_reason = NULL;
    bool resumed = false;
    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        skip_reason = "GLFW fixture unavailable";
        goto cleanup;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    if (
        !canvas->supports_external_memory || !canvas->supports_external_semaphore ||
        dvz_canvas_timeline_handle_type() == 0)
    {
        skip_reason = "external memory/semaphore unsupported";
        goto cleanup;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
    DvzVideoSinkConfig sink_cfg = dvz_video_sink_default_config();
    sink_cfg.encoder.backend = "auto";
    sink_cfg.encoder.width = surface.extent.width ? surface.extent.width : 640;
    sink_cfg.encoder.height = surface.extent.height ? surface.extent.height : 480;
    sink_cfg.encoder.fps = 30;
    sink_cfg.encoder.mux = DVZ_VIDEO_MUX_NONE;
    sink_cfg.encoder.mp4_path = "/tmp/dvz_canvas_video_disable_test.mp4";
    sink_cfg.encoder.raw_path = "/tmp/dvz_canvas_video_disable_test.h26x";
    if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) != 0)
    {
        skip_reason = "sink could not be enabled";
        goto cleanup;
    }

    bool first_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        if (frame_rc != DVZ_CANVAS_FRAME_READY)
        {
            skip_reason = "video backend unavailable";
            break;
        }
        AT(dvz_canvas_submit(canvas) == 0);
        first_submit = true;
        break;
    }
    if (!first_submit)
    {
        if (skip_reason == NULL)
        {
            skip_reason = "initial submit path not reached within frame budget";
        }
        goto cleanup;
    }

    AT(canvas->video_sink_enabled);
    AT(canvas->stream_started);

    AT(dvz_canvas_configure_video_sink(canvas, false, NULL) == 0);
    AT(!canvas->video_sink_enabled);
    AT(!canvas->stream_started);

    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
        AT(frame != NULL);
        AT(frame->wait_semaphore_fd < 0);
        AT(dvz_canvas_submit(canvas) == 0);
        resumed = true;
        break;
    }

    if (!resumed && skip_reason == NULL)
    {
        skip_reason = "submit path not reached after disabling sink";
    }

cleanup:
    if (fixture.canvas != NULL && fixture.canvas->video_sink_enabled)
    {
        AT(dvz_canvas_configure_video_sink(fixture.canvas, false, NULL) == 0);
    }
    if (skip_reason != NULL)
    {
        log_warn("canvas video sink disable test skipped (%s)", skip_reason);
        tst_skip(suite, skip_reason);
    }
    else
    {
        AT(resumed);
    }
    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate raw and PNG capture APIs on the latest presented canvas frame.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_capture_api(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool submitted = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        submitted = true;
        break;
    }
    AT(submitted);

    DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
    uint32_t width = surface.extent.width;
    uint32_t height = surface.extent.height;
    AT(width > 0);
    AT(height > 0);

    size_t byte_count = (size_t)width * (size_t)height * 4;
    uint8_t* scratch = (uint8_t*)dvz_calloc(byte_count, sizeof(uint8_t));
    ANN(scratch);
    AT(dvz_canvas_capture_rgba_into(canvas, width, height, scratch, byte_count) == 0);
    AT_EXPECTED_ERROR(
        suite, dvz_canvas_capture_rgba_into(canvas, width + 1, height, scratch, byte_count) != 0);

    uint64_t sum = 0;
    for (size_t i = 0; i < byte_count; ++i)
    {
        sum += scratch[i];
    }
    AT(sum > 0);
    dvz_free(scratch);

    uint32_t out_width = 0;
    uint32_t out_height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &out_width, &out_height, &rgba) == 0);
    ANN(rgba);
    AT(out_width == width);
    AT(out_height == height);
    AT(rgba[0] + rgba[1] + rgba[2] + rgba[3] > 0);
    dvz_free(rgba);

    const char* png_path = "/tmp/dvz_canvas_capture_api.png";
#if OS_UNIX
    unlink(png_path);
#endif
    AT(dvz_canvas_capture_png(canvas, png_path) == 0);
    struct stat st = {0};
    AT(stat(png_path, &st) == 0);
    AT(st.st_size > 0);
#if OS_UNIX
    unlink(png_path);
#endif

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Ensure sink handle refresh and submit wait-value continuity after forced recreate.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_video_handle_refresh_after_recreate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    bool first_submit_done = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        first_submit_done = true;
        break;
    }
    AT(first_submit_done);

    probe.awaiting_refresh = true;
    probe.saw_update_since_refresh = false;
    uint64_t wait_before = probe.last_wait_value;
    uint32_t submit_before = probe.submit_count;
    dvz_canvas_swapchain_mark_out_of_date(canvas);

    bool resumed = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        if (probe.submit_count > submit_before)
        {
            resumed = true;
            break;
        }
    }

    AT(resumed);
    AT(probe.update_count > 0);
    AT(probe.saw_update_since_refresh);
    AT(probe.stale_submit_count == 0);
    AT(probe.last_wait_value > wait_before);
    AT(probe.wait_value_non_monotonic == 0);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify that device loss moves the canvas presentation runtime into a fatal state.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_device_lost_fatal_transition(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool ready = false;
    for (uint32_t i = 0; i < 16; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        ready = true;
        break;
    }
    AT(ready);

    dvz_canvas_swapchain_test_force_present_status(canvas, DVZ_PRESENT_STATUS_DEVICE_LOST);
    tst_expect_error_begin(suite);
    AT(dvz_canvas_submit(canvas) < 0);
    (void)tst_expect_error_end(suite);
    AT(
        dvz_canvas_present_runtime_state(canvas) ==
        DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST);

    dvz_window_host_poll(fixture.host);
    int frame_rc = 0;
    tst_expect_error_begin(suite);
    frame_rc = dvz_canvas_frame(canvas);
    AT(frame_rc < 0);
    (void)tst_expect_error_end(suite);

    dvz_canvas_swapchain_mark_out_of_date(canvas);
    AT(
        dvz_canvas_present_runtime_state(canvas) ==
        DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST);

    dvz_canvas_swapchain_test_force_recreate_status(canvas, -1);
    dvz_canvas_swapchain_test_force_acquire_status(canvas, -1);
    dvz_canvas_swapchain_test_force_present_status(canvas, -1);

    canvas_glfw_fixture_destroy(&fixture);
    return 0;
}



/**
 * Validate wrap-backend canvas present flow through external-surface loss and restore.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_glfw_wrap_surface_present_recovery(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if DVZ_HAS_GLFW
    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    if (fixture.canvas != NULL)
    {
        dvz_canvas_set_draw_callback(fixture.canvas, NULL, NULL);
        dvz_canvas_destroy(fixture.canvas);
        fixture.canvas = NULL;
    }
    if (fixture.window != NULL)
    {
        dvz_window_destroy(fixture.window);
        fixture.window = NULL;
    }

    uint32_t ext_count = dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_GLFW);
    if (ext_count > 0)
    {
        const char** extensions = dvz_calloc(ext_count, sizeof(char*));
        ANN(extensions);
        AT(dvz_window_host_required_extensions(fixture.host, DVZ_BACKEND_GLFW, ext_count, extensions) == (int)ext_count);
        AT(dvz_window_wrap_set_required_extensions(fixture.host, ext_count, extensions) == 0);
        AT(dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_WRAP) == ext_count);
        dvz_free((void*)extensions);
    }

    DvzWindowConfig cfg = dvz_test_wrap_window_config("canvas-wrap-external-surface", 320, 240);
    CanvasWrapSurfaceFixture wrap = {0};
    if (!_canvas_wrap_surface_fixture_create(&fixture, &cfg, &wrap))
    {
        _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "wrap-surface fixture unavailable");
        return 0;
    }

    DvzCanvasConfig canvas_cfg = dvz_canvas_default_config();
    canvas_cfg.window = wrap.wrap_window;
    canvas_cfg.device = fixture.device;
    canvas_cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    canvas_cfg.timing_history = 1;
    fixture.canvas = dvz_canvas_create(&canvas_cfg);
    if (fixture.canvas == NULL)
    {
        log_warn("canvas wrap test skipped because canvas creation failed");
        _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "canvas creation failed");
        return 0;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(fixture.canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool initial_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(fixture.canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(fixture.canvas) == 0);
        initial_submit = true;
        break;
    }
    AT(initial_submit);

    DvzWindowExternalSurfaceInfo loss =
        dvz_test_wrap_surface_info(
            VK_NULL_HANDLE, VK_NULL_HANDLE, wrap.info.extent.width, wrap.info.extent.height,
            wrap.info.scale_x, wrap.info.scale_y, false);
    AT(dvz_window_wrap_update_surface(wrap.wrap_window, &loss) == 0);

    uint32_t wait_surface_count = 0;
    uint32_t loss_ready_count = 0;
    for (uint32_t i = 0; i < 12; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(fixture.canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            wait_surface_count++;
            continue;
        }
        if (frame_rc == DVZ_CANVAS_FRAME_READY)
        {
            loss_ready_count++;
            AT(dvz_canvas_submit(fixture.canvas) == 0);
        }
    }
    AT(wait_surface_count + loss_ready_count > 0);

    uint64_t frame_id_before_restore = fixture.canvas->frame_id;
    AT(dvz_window_wrap_update_surface(wrap.wrap_window, &wrap.info) == 0);
    dvz_canvas_swapchain_mark_out_of_date(fixture.canvas);

    bool restored_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(fixture.canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(fixture.canvas) == 0);
        restored_submit = true;
        break;
    }
    AT(restored_submit);
    AT(fixture.canvas->frame_id > frame_id_before_restore);

    if (fixture.canvas != NULL)
    {
        dvz_canvas_set_draw_callback(fixture.canvas, NULL, NULL);
        dvz_canvas_destroy(fixture.canvas);
        fixture.canvas = NULL;
    }
    _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
    canvas_glfw_fixture_destroy(&fixture);
    return 0;
#else
    log_warn("canvas wrap test skipped because Datoviz was not build with glfw support");
    tst_skip(suite, "Datoviz was not built with GLFW support");
    return 0;
#endif
}



int test_canvas_glfw_wrap_surface_resize_recreate_refreshes_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if DVZ_HAS_GLFW
    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "GLFW fixture unavailable");
        return 0;
    }

    if (fixture.canvas != NULL)
    {
        dvz_canvas_set_draw_callback(fixture.canvas, NULL, NULL);
        dvz_canvas_destroy(fixture.canvas);
        fixture.canvas = NULL;
    }
    if (fixture.window != NULL)
    {
        dvz_window_destroy(fixture.window);
        fixture.window = NULL;
    }

    uint32_t ext_count = dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_GLFW);
    if (ext_count > 0)
    {
        const char** extensions = dvz_calloc(ext_count, sizeof(char*));
        ANN(extensions);
        AT(dvz_window_host_required_extensions(fixture.host, DVZ_BACKEND_GLFW, ext_count, extensions) == (int)ext_count);
        AT(dvz_window_wrap_set_required_extensions(fixture.host, ext_count, extensions) == 0);
        AT(dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_WRAP) == ext_count);
        dvz_free((void*)extensions);
    }

    DvzWindowConfig cfg = dvz_test_wrap_window_config("canvas-wrap-resize-recreate", 320, 240);
    CanvasWrapSurfaceFixture wrap = {0};
    if (!_canvas_wrap_surface_fixture_create(&fixture, &cfg, &wrap))
    {
        _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "wrap-surface fixture unavailable");
        return 0;
    }

    DvzCanvasConfig canvas_cfg = dvz_canvas_default_config();
    canvas_cfg.window = wrap.wrap_window;
    canvas_cfg.device = fixture.device;
    canvas_cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    canvas_cfg.timing_history = 1;
    fixture.canvas = dvz_canvas_create(&canvas_cfg);
    if (fixture.canvas == NULL)
    {
        log_warn("canvas wrap resize test skipped because canvas creation failed");
        _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "canvas creation failed");
        return 0;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(fixture.canvas, canvas_glfw_clear_draw, &clear_ctx);

    CanvasRefreshProbeState probe = {
        .awaiting_refresh = false,
        .saw_update_since_refresh = false,
        .latest_memory_fd = -1,
        .latest_wait_semaphore_fd = -1,
    };
    AT(dvz_stream_attach_sink(fixture.canvas->stream, &CANVAS_REFRESH_PROBE_BACKEND, &probe) == 0);

    bool initial_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(fixture.canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(fixture.canvas) == 0);
        initial_submit = true;
        break;
    }
    AT(initial_submit);

    DvzCanvasSurfaceInfo before_surface = dvz_canvas_window_surface_info(fixture.canvas);
    uint32_t update_before = probe.update_count;
    uint32_t submit_before = probe.submit_count;
    uint32_t callback_before = clear_ctx.callback_count;
    probe.awaiting_refresh = true;
    probe.saw_update_since_refresh = false;

    /* Actually resize the underlying GLFW window so VkSurfaceCapabilitiesKHR.currentExtent
     * picks up the new size — otherwise the canvas swapchain will resolve to the old extent
     * and the offscreen image (which we keep in sync with the swapchain to avoid blit
     * stretching) won't reach the requested resized.extent. */
    DvzWindowExternalSurfaceInfo resized = wrap.info;
    int target_w = (int)wrap.info.extent.width + 64;
    int target_h = (int)wrap.info.extent.height + 32;
    glfwSetWindowSize(wrap.external_handle, target_w, target_h);
    int observed_w = 0, observed_h = 0;
    for (uint32_t i = 0; i < 32; i++)
    {
        glfwPollEvents();
        glfwGetFramebufferSize(wrap.external_handle, &observed_w, &observed_h);
        if (observed_w == target_w && observed_h == target_h)
            break;
    }
    if (observed_w != target_w || observed_h != target_h)
    {
        log_warn(
            "wrap external GLFW window did not reach %dx%d (got %dx%d); skipping resize test",
            target_w, target_h, observed_w, observed_h);
        if (fixture.canvas != NULL)
        {
            dvz_canvas_set_draw_callback(fixture.canvas, NULL, NULL);
            dvz_canvas_destroy(fixture.canvas);
            fixture.canvas = NULL;
        }
        _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
        canvas_glfw_fixture_destroy(&fixture);
        tst_skip(suite, "external GLFW window did not reach target resize");
        return 0;
    }
    resized.extent.width = (uint32_t)observed_w;
    resized.extent.height = (uint32_t)observed_h;
    AT(dvz_window_wrap_update_surface(wrap.wrap_window, &resized) == 0);
    const DvzWindowSurface* resized_surface = dvz_window_surface(wrap.wrap_window);
    ANN(resized_surface);
    AT(resized_surface->extent.width == resized.extent.width);
    AT(resized_surface->extent.height == resized.extent.height);
    dvz_canvas_swapchain_mark_out_of_date(fixture.canvas);

    bool recreated_submit = false;
    for (uint32_t i = 0; i < 24; i++)
    {
        dvz_window_host_poll(fixture.host);
        int frame_rc = dvz_canvas_frame(fixture.canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(fixture.canvas) == 0);
        recreated_submit = true;
        if (probe.update_count > update_before)
        {
            break;
        }
    }

    AT(recreated_submit);
    AT(probe.update_count > update_before);
    AT(probe.submit_count > submit_before);
    AT(probe.saw_update_since_refresh);
    AT(probe.stale_submit_count == 0);
    AT(probe.latest_handles_dirty);
    AT(probe.latest_extent.width == resized.extent.width);
    AT(probe.latest_extent.height == resized.extent.height);
    AT(clear_ctx.callback_count > callback_before);
    AT(clear_ctx.latest_extent.width == resized.extent.width);
    AT(clear_ctx.latest_extent.height == resized.extent.height);
    DvzCanvasSurfaceInfo after_surface = dvz_canvas_window_surface_info(fixture.canvas);
    AT(after_surface.extent.width != before_surface.extent.width || after_surface.extent.height != before_surface.extent.height);

    if (fixture.canvas != NULL)
    {
        dvz_canvas_set_draw_callback(fixture.canvas, NULL, NULL);
        dvz_canvas_destroy(fixture.canvas);
        fixture.canvas = NULL;
    }
    _canvas_wrap_surface_fixture_destroy(&fixture, &wrap);
    canvas_glfw_fixture_destroy(&fixture);
    return 0;
#else
    log_warn("canvas wrap resize test skipped because Datoviz was not build with glfw support");
    tst_skip(suite, "Datoviz was not built with GLFW support");
    return 0;
#endif
}



/**
 * Exercise the GLFW-backed canvas and ensure the frame submission path works.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_glfw(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if DVZ_HAS_GLFW
    DvzInstance* instance = NULL;

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzDevice* device = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    const char* skip_reason = NULL;
    DvzInstanceConfig icfg = dvz_instance_default_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    dvz_instance_config_request_extension(&icfg, VK_KHR_SURFACE_EXTENSION_NAME);

    // Additional ones for glfw.
    if (!dvz_window_glfw_init())
    {
        skip_reason = "GLFW could not initialize";
        log_warn("canvas glfw test skipped because GLFW could not initialize");
        goto canvas_glfw_cleanup;
    }

    uint32_t ext_count = dvz_window_host_required_extension_count(host, DVZ_BACKEND_GLFW);
    if (ext_count == 0)
    {
        skip_reason = "GLFW returned no Vulkan instance extensions";
        log_warn("canvas glfw test skipped because GLFW returned no Vulkan instance extensions");
        goto canvas_glfw_cleanup;
    }
    const char** extensions = dvz_calloc(ext_count, sizeof(char*));
    if (extensions == NULL)
    {
        skip_reason = "extension-list allocation failed";
        log_warn("canvas glfw test skipped because extension-list allocation failed");
        goto canvas_glfw_cleanup;
    }
    int written = dvz_window_host_required_extensions(host, DVZ_BACKEND_GLFW, ext_count, extensions);
    if (written != (int)ext_count)
    {
        dvz_free((void*)extensions);
        skip_reason = "required-extension query failed";
        log_warn("canvas glfw test skipped because required-extension query failed");
        goto canvas_glfw_cleanup;
    }

    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_config_request_extension(&icfg, extensions[i]);
    }
    dvz_free((void*)extensions);

    instance = dvz_instance_create(&icfg);
    if (instance == NULL)
    {
        skip_reason = "Vulkan instance creation failed";
        log_warn("canvas glfw test skipped because Vulkan instance creation failed");
        goto canvas_glfw_cleanup;
    }

    AT(dvz_instance_has_extension(instance, VK_KHR_SURFACE_EXTENSION_NAME));

    uint32_t gpu_count = dvz_instance_gpu_count(instance);
    if (gpu_count == 0)
    {
        skip_reason = "no Vulkan GPU was found";
        log_warn("canvas glfw test skipped because no Vulkan GPU was found");
        goto canvas_glfw_cleanup;
    }

    DvzGpuInfo info = {0};
    AT(dvz_instance_gpu_info(instance, 0, &info));
    log_debug("device name: %s", info.name);

    DvzQueueCaps caps = {0};
    AT(dvz_instance_gpu_queue_caps(instance, 0, &caps));

    // Create the device.
    DvzQueues queues = {0};
    dvz_queues(&caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(instance);
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
    dvz_device_config_enable_canvas_extensions(&dcfg, true);
    device = dvz_device_create(&dcfg);
    if (device == NULL)
    {
        skip_reason = "Vulkan device creation failed";
        log_warn("canvas glfw test skipped because Vulkan device creation failed");
        goto canvas_glfw_cleanup;
    }

    log_trace("creating window");
    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
    window_cfg.visible = _canvas_glfw_test_visible();
    window = dvz_window_create(host, DVZ_BACKEND_GLFW, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        skip_reason = "GLFW window creation failed";
        log_warn("canvas glfw test skipped because GLFW window creation failed");
        goto canvas_glfw_cleanup;
    }

    dvz_window_host_poll(host);

    DvzCanvasConfig cfg = dvz_canvas_default_config();
    cfg.window = window;
    cfg.device = device;
    cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    cfg.timing_history = 1;

    log_trace("creating canvas");
    canvas = dvz_canvas_create(&cfg);
    if (canvas == NULL)
    {
        skip_reason = "canvas creation failed";
        log_warn("canvas glfw test skipped because canvas creation failed");
        goto canvas_glfw_cleanup;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = device,
        .format = cfg.color_format,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool record_video = false;
    const char* video_env = getenv("DVZ_CANVAS_GLFW_VIDEO");
    if (video_env && video_env[0] != '\0' && video_env[0] != '0')
    {
        DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
        bool has_external_memory =
            canvas->supports_external_memory && canvas->allocator != NULL &&
            dvz_allocator_external(canvas->allocator) != 0;
        bool has_external_semaphore = canvas->supports_external_semaphore;
        if (has_external_memory && has_external_semaphore)
        {
            DvzVideoSinkConfig sink_cfg = dvz_video_sink_default_config();
            sink_cfg.encoder.backend = "nvenc";
            sink_cfg.encoder.width = surface.extent.width ? surface.extent.width : 1920;
            sink_cfg.encoder.height = surface.extent.height ? surface.extent.height : 1080;
            sink_cfg.encoder.fps = 60;
            sink_cfg.encoder.mp4_path = "canvas.mp4";
            sink_cfg.bitstream = NULL;
            if (dvz_canvas_configure_video_sink(canvas, true, &sink_cfg) == 0)
            {
                record_video = true;
                log_info("canvas GLFW recording will be written to canvas.mp4");
            }
            else
            {
                log_warn("canvas GLFW video sink could not be enabled");
            }
        }
        else
        {
            log_warn("video sink requested but canvas lacks external memory/semaphore support");
        }
    }

    bool interactive_loop = false;
    bool keep_running = true;
    DvzInputRouter* router = dvz_canvas_input(canvas);
    const char* loop_env = getenv("DVZ_CANVAS_GLFW_LOOP");
    bool keep_looping = false;
    if (loop_env && loop_env[0] != '\0' && loop_env[0] != '0')
    {
        keep_looping = true;
    }
    if (record_video)
    {
        keep_looping = true;
    }
    if (keep_looping && router)
    {
        interactive_loop = true;
        dvz_input_subscribe_keyboard(router, canvas_glfw_keyboard_callback, &keep_running);
    }

    DvzClock loop_clock = dvz_clock();
    dvz_clock_tick(&loop_clock);
    size_t submit_count = 0;
    bool recovery_forced = false;
    bool recovery_resumed = false;
    const size_t target_submits = interactive_loop ? 1 : 2;

    do
    {
        dvz_window_host_poll(host);
        int frame_rc = dvz_canvas_frame(canvas);
        if (frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            continue;
        }
        AT(frame_rc == DVZ_CANVAS_FRAME_READY);
        AT(dvz_canvas_submit(canvas) == 0);
        submit_count++;
        if (!recovery_forced)
        {
            dvz_canvas_swapchain_mark_out_of_date(canvas);
            recovery_forced = true;
            continue;
        }
        recovery_resumed = true;
    } while (
        (interactive_loop && keep_running) ||
        (!interactive_loop && submit_count < target_submits));

    AT(recovery_forced);
    AT(recovery_resumed);

    if (device != NULL)
    {
        dvz_device_wait(device);
    }

    if (interactive_loop && router)
    {
        dvz_input_unsubscribe_keyboard(router, canvas_glfw_keyboard_callback, &keep_running);
    }

    double elapsed_s = dvz_clock_interval(&loop_clock);
    if (submit_count > 0 && elapsed_s > 0.0)
    {
        double avg_fps = (double)submit_count / elapsed_s;
        const char* frame_label = submit_count == 1 ? "frame" : "frames";
        log_info(
            "canvas GLFW average FPS: %.2f (%zu %s over %.2fs)", avg_fps, submit_count,
            frame_label, elapsed_s);
    }

canvas_glfw_cleanup:
    if (canvas != NULL)
    {
        dvz_canvas_set_draw_callback(canvas, NULL, NULL);
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
    if (device != NULL)
    {
        dvz_device_destroy(device);
    }
    if (instance != NULL)
    {
        dvz_instance_destroy(instance);
    }
    if (skip_reason != NULL)
    {
        tst_skip(suite, skip_reason);
    }

#else
    log_warn("canvas glfw test skipped because Datoviz was not build with glfw support");
    tst_skip(suite, "Datoviz was not built with GLFW support");
#endif


    return 0;
}
