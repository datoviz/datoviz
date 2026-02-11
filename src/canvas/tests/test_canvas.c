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
#include <stdlib.h>

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
#include "datoviz/window.h"
#include "test_canvas.h"
#include "testing.h"

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
} CanvasGlfwClearContext;



typedef struct CanvasGlfwFixture
{
    DvzInstance instance;
    DvzWindowHost* host;
    DvzDevice device;
    DvzWindow* window;
    DvzCanvas* canvas;
    bool device_initialized;
} CanvasGlfwFixture;



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

    DvzCommands cmds = {0};
    dvz_commands_wrap(canvas->device, cmd, &cmds);

    DvzRendering rendering = {0};
    dvz_cmd_rendering_default(
        &cmds, image_view, frame->extent.width, frame->extent.height,
        (VkClearValue){.color.float32 = {1, 0, 0, 1}}, &rendering);
    dvz_cmd_rendering_begin(&cmds, &rendering);
    dvz_cmd_rendering_end(&cmds);
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
    dvz_instance(&fixture->instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_request_extension(&fixture->instance, VK_KHR_SURFACE_EXTENSION_NAME);

    fixture->host = dvz_window_host();
    ANN(fixture->host);

    if (!dvz_window_glfw_init())
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because GLFW could not initialize");
        return 0;
    }

    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (extensions == NULL || ext_count == 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because GLFW returned no Vulkan instance extensions");
        return 0;
    }
    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_request_extension(&fixture->instance, extensions[i]);
    }

    if (dvz_instance_create(&fixture->instance, VK_API_VERSION_1_3) != 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because Vulkan instance creation failed");
        return 0;
    }

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&fixture->instance, &gpu_count);
    if (gpus == NULL || gpu_count == 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because no Vulkan GPU was found");
        return 0;
    }

    DvzGpu* gpu = &gpus[0];
    DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
    ANN(caps);

    dvz_gpu_device(gpu, &fixture->device);
    fixture->device_initialized = true;
    dvz_queues(caps, &fixture->device.queues);

    VkPhysicalDeviceVulkan12Features* fet12 = dvz_device_request_features12(&fixture->device);
    fet12->timelineSemaphore = true;

    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(&fixture->device);
    features->synchronization2 = true;
    features->dynamicRendering = true;

    dvz_device_request_canvas_extensions(&fixture->device);
    if (dvz_device_create(&fixture->device) != 0)
    {
        *skipped = true;
        log_warn("canvas glfw fixture skipped because Vulkan device creation failed");
        return 0;
    }

    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
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
    cfg.device = &fixture->device;
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

    dvz_canvas_swapchain_test_fail_slot(-1);
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
    if (fixture->device_initialized)
    {
        dvz_device_destroy(&fixture->device);
        fixture->device_initialized = false;
    }
    dvz_instance_destroy(&fixture->instance);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Validate the default canvas configuration.
 */
int test_canvas_defaults(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasConfig cfg = dvz_canvas_default_config();
    AT(cfg.window == NULL);
    AT(cfg.device == NULL);
    AT(cfg.color_format == VK_FORMAT_UNDEFINED);
    AT(cfg.present_mode == VK_PRESENT_MODE_FIFO_KHR);
    AT(!cfg.enable_video_sink);
    AT(cfg.timing_history == DVZ_CANVAS_DEFAULT_TIMING_HISTORY);
    return 0;
}



/**
 * Ensure the frame pool rotates across entries.
 */
int test_canvas_frame_pool(TstSuite* suite, TstItem* item)
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
int test_canvas_timings(TstSuite* suite, TstItem* item)
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
 * Ensure slot initialization failures abort swapchain creation without partial-frame progression.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_swapchain_failfast_slot_init(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = &fixture.device,
        .format = DVZ_DEFAULT_COLOR_FORMAT,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    dvz_canvas_swapchain_test_fail_slot(0);
    dvz_window_host_poll(fixture.host);
    int frame_rc = dvz_canvas_frame(canvas);
    AT(frame_rc < 0);

    dvz_canvas_swapchain_test_fail_slot(-1);
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
int test_canvas_glfw_present_recovery(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    CanvasGlfwFixture fixture = {0};
    bool skipped = false;
    AT(canvas_glfw_fixture_create(&fixture, &skipped) == 0);
    if (skipped)
    {
        canvas_glfw_fixture_destroy(&fixture);
        return 0;
    }

    DvzCanvas* canvas = fixture.canvas;
    ANN(canvas);
    uint64_t frame_id_before = canvas->frame_id;
    uint64_t timeline_before = canvas->timeline_value;

    CanvasGlfwClearContext clear_ctx = {
        .device = &fixture.device,
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
 * Exercise the GLFW-backed canvas and ensure the frame submission path works.
 *
 * @param suite The owning test suite.
 * @param item  The test item (unused).
 * @return int  Zero on success.
 */
int test_canvas_glfw(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

#if DVZ_HAS_GLFW
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzDevice device = {0};
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    bool device_initialized = false;

    // Instance extensions.
    dvz_instance_request_extension(&instance, VK_KHR_SURFACE_EXTENSION_NAME);

    // Additional ones for glfw.
    if (!dvz_window_glfw_init())
    {
        log_warn("canvas glfw test skipped because GLFW could not initialize");
        goto canvas_glfw_cleanup;
    }

    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (extensions == NULL || ext_count == 0)
    {
        log_warn("canvas glfw test skipped because GLFW returned no Vulkan instance extensions");
        goto canvas_glfw_cleanup;
    }

    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_request_extension(&instance, extensions[i]);
    }

    if (dvz_instance_create(&instance, VK_API_VERSION_1_3) != 0)
    {
        log_warn("canvas glfw test skipped because Vulkan instance creation failed");
        goto canvas_glfw_cleanup;
    }

    AT(dvz_instance_has_extension(&instance, VK_KHR_SURFACE_EXTENSION_NAME));

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &gpu_count);
    if (gpus == NULL || gpu_count == 0)
    {
        log_warn("canvas glfw test skipped because no Vulkan GPU was found");
        goto canvas_glfw_cleanup;
    }

    DvzGpu* gpu = &gpus[0];
    ANN(gpu);

    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(gpu);
    log_debug("device name: %s", props->deviceName);

    DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
    ANN(caps);

    // Create the device.
    dvz_gpu_device(gpu, &device);
    device_initialized = true;
    dvz_queues(caps, &device.queues);

    VkPhysicalDeviceVulkan12Features* fet12 = dvz_device_request_features12(&device);
    fet12->timelineSemaphore = true;

    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(&device);
    features->synchronization2 = true;
    features->dynamicRendering = true;

    // Device extensions required for the canvas.
    dvz_device_request_canvas_extensions(&device);

    if (dvz_device_create(&device) != 0)
    {
        log_warn("canvas glfw test skipped because Vulkan device creation failed");
        goto canvas_glfw_cleanup;
    }

    log_trace("creating window");
    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
    window = dvz_window_create(host, DVZ_BACKEND_GLFW, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        log_warn("canvas glfw test skipped because GLFW window creation failed");
        goto canvas_glfw_cleanup;
    }

    dvz_window_host_poll(host);

    DvzCanvasConfig cfg = dvz_canvas_default_config();
    cfg.window = window;
    cfg.device = &device;
    cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    cfg.timing_history = 1;

    log_trace("creating canvas");
    canvas = dvz_canvas_create(&cfg);
    if (canvas == NULL)
    {
        log_warn("canvas glfw test skipped because canvas creation failed");
        goto canvas_glfw_cleanup;
    }

    CanvasGlfwClearContext clear_ctx = {
        .device = &device,
        .format = cfg.color_format,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool record_video = false;
    const char* video_env = getenv("DVZ_CANVAS_GLFW_VIDEO");
    if (video_env && video_env[0] != '\0' && video_env[0] != '0')
    {
        DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
        bool has_external_memory =
            canvas->supports_external_memory && canvas->allocator.external != 0;
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
    } while ((interactive_loop && keep_running) || (!interactive_loop && submit_count < target_submits));

    AT(recovery_forced);
    AT(recovery_resumed);

    dvz_device_wait(&device);

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
    if (device_initialized)
    {
        dvz_device_destroy(&device);
    }
    dvz_instance_destroy(&instance);

#else
    log_warn("canvas glfw test skipped because Datoviz was not build with glfw support");
#endif


    return 0;
}



/**
 * Register the canvas tests.
 */
int test_canvas(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "canvas";
    TEST_SIMPLE(test_canvas_defaults);
    TEST_SIMPLE(test_canvas_frame_pool);
    TEST_SIMPLE(test_canvas_timings);
    TEST_SIMPLE(test_canvas_swapchain_failfast_slot_init);
    TEST_SIMPLE(test_canvas_glfw_present_recovery);
    TEST_SIMPLE(test_canvas_glfw);
    return 0;
}
