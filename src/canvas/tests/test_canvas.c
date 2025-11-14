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

#include "_assertions.h"
#include "datoviz/canvas.h"
#include "datoviz/window.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "test_canvas.h"
#include "testing.h"



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

#ifndef DVZ_WITH_GLFW
#define DVZ_WITH_GLFW 0
#endif

#if DVZ_WITH_GLFW
int test_canvas_glfw(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &gpu_count);
    ANN(gpus);
    DvzGpu* gpu = &gpus[0];

    DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
    ANN(caps);

    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);
    dvz_queues(caps, &device.queues);
    dvz_device_request_canvas_extensions(&device);
    AT(dvz_device_create(&device) == 0);

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "canvas-glfw-test";
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_GLFW, &window_cfg);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_GLFW);
    dvz_window_host_poll(host);

    DvzCanvasConfig cfg = dvz_canvas_default_config();
    cfg.window = window;
    cfg.device = &device;
    cfg.timing_history = 1;
    DvzCanvas* canvas = dvz_canvas_create(&cfg);
    ANN(canvas);

    int frame_rc = DVZ_CANVAS_FRAME_WAIT_SURFACE;
    for (int tries = 0; tries < 5 && frame_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE; ++tries)
    {
        dvz_window_host_poll(host);
        frame_rc = dvz_canvas_frame(canvas);
    }
    AT(frame_rc == DVZ_CANVAS_FRAME_READY);
    AT(dvz_canvas_submit(canvas) == 0);

    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);
    return 0;
}
#else
int test_canvas_glfw(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return 0;
}
#endif



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
