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

#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/canvas.h"
#include "datoviz/input/keycodes.h"
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
    cmds.cmds[0] = cmd;
    cmds.count = 1;
    cmds.current = 0;

    DvzRendering rendering = {0};
    dvz_rendering(&rendering);
    dvz_rendering_area(&rendering, 0, 0, frame->extent.width, frame->extent.height);
    DvzAttachment* catt = dvz_rendering_color(&rendering, 0);
    dvz_attachment_image(catt, image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(catt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(catt, (VkClearValue){.color.float32 = {0.18f, 0.25f, 0.4f, 1.0f}});

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
    AT(cfg.color_format == VK_FORMAT_B8G8R8A8_UNORM);
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

    // Instance extensions.
    dvz_instance_request_extension(&instance, VK_KHR_SURFACE_EXTENSION_NAME);

    // Additional ones for glfw.
    dvz_window_glfw_init();
    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_request_extension(&instance, extensions[i]);
    }

    dvz_instance_create(&instance, VK_API_VERSION_1_3);
    AT(dvz_instance_has_extension(&instance, VK_KHR_SURFACE_EXTENSION_NAME));

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &gpu_count);
    ANN(gpus);

    DvzGpu* gpu = &gpus[0];
    ANN(gpu);

    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(gpu);
    log_debug("device name: %s", props->deviceName);

    DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
    ANN(caps);

    // Create the device.
    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);
    dvz_queues(caps, &device.queues);

    VkPhysicalDeviceVulkan12Features* fet12 = dvz_device_request_features12(&device);
    fet12->timelineSemaphore = true;

    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(&device);
    features->synchronization2 = true;
    features->dynamicRendering = true;

    // Device extensions required for the canvas.
    dvz_device_request_canvas_extensions(&device);

    AT(dvz_device_create(&device) == 0);

    log_trace("creating window");
    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_GLFW, &window_cfg);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_GLFW);
    dvz_window_host_poll(host);

    DvzCanvasConfig cfg = dvz_canvas_default_config();
    cfg.window = window;
    cfg.device = &device;
    cfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    cfg.timing_history = 1;

    log_trace("creating canvas");
    DvzCanvas* canvas = dvz_canvas_create(&cfg);
    ANN(canvas);

    CanvasGlfwClearContext clear_ctx = {
        .device = &device,
        .format = cfg.color_format,
    };
    dvz_canvas_set_draw_callback(canvas, canvas_glfw_clear_draw, &clear_ctx);

    bool interactive_loop = false;
    bool keep_running = true;
    DvzInputRouter* router = dvz_canvas_input(canvas);
    // Set DVZ_CANVAS_GLFW_LOOP=1 to keep drawing until Escape requests exit.
    const char* loop_env = getenv("DVZ_CANVAS_GLFW_LOOP");
    if (loop_env && loop_env[0] != '\0' && loop_env[0] != '0' && router)
    {
        interactive_loop = true;
        dvz_input_subscribe_keyboard(router, canvas_glfw_keyboard_callback, &keep_running);
    }

    DvzClock loop_clock = dvz_clock();
    dvz_clock_tick(&loop_clock);
    size_t submit_count = 0;

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
    } while (interactive_loop && keep_running);

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

    dvz_canvas_set_draw_callback(canvas, NULL, NULL);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_device_destroy(&device);
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
    TEST_SIMPLE(test_canvas_glfw);
    return 0;
}
