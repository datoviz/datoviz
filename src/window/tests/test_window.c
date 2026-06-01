/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing window module                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_window.h"
#include "_assertions.h"
#include "datoviz/common.h"
#include "datoviz/window.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResizeRecorder
{
    uint32_t resize_count;
    uint32_t width;
    uint32_t height;
} ResizeRecorder;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void
_window_resize_callback(DvzInputRouter* router, const DvzInputResizeEvent* event, void* user_data)
{
    (void)router;
    ANN(event);
    ResizeRecorder* recorder = (ResizeRecorder*)user_data;
    recorder->resize_count++;
    recorder->width = event->framebuffer_width;
    recorder->height = event->framebuffer_height;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Ensure headless windows can be created.
 */
int test_window_headless(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_OFFSCREEN);
    dvz_window_host_destroy(host);
    return 0;
}



int test_window_config_rejects_invalid_abi(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);

    DvzWindowConfig cfg = dvz_window_config();
    cfg.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &cfg) == NULL);

    cfg = dvz_window_config();
    cfg.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &cfg) == NULL);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify resize events propagate through the router.
 */
int test_window_resize_events(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);
    ResizeRecorder recorder = {0};
    DvzInputRouter* router = dvz_window_router(window);
    ANN(router);
    dvz_input_subscribe_resize(router, _window_resize_callback, &recorder);
    dvz_window_backend_emit_resize(window, 256, 192, 128, 96, 1.f, 1.f);
    AT(recorder.resize_count == 1);
    AT(recorder.width == 256);
    AT(recorder.height == 192);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Ensure frame requests toggle the pending flag.
 */
int test_window_frame_requests(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);
    AT(!dvz_window_frame_pending(window));
    dvz_window_host_request_frame(host, window);
    AT(dvz_window_frame_pending(window));
    dvz_window_host_poll(host);
    AT(!dvz_window_frame_pending(window));
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Ensure headless wait hooks clear pending frame requests without blocking indefinitely.
 */
int test_window_wait_hooks_headless(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);

    dvz_window_host_request_frame(host, window);
    AT(dvz_window_frame_pending(window));

    uint64_t start = dvz_time_monotonic_ns();
    dvz_window_host_wait_timeout(host, 0.001);
    uint64_t end = dvz_time_monotonic_ns();
    AT(!dvz_window_frame_pending(window));
    AT(end >= start);
    AT(end - start < 2000000000ULL);

    dvz_window_host_request_frame(host, window);
    AT(dvz_window_frame_pending(window));
    dvz_window_host_wait(host);
    AT(!dvz_window_frame_pending(window));

    dvz_window_host_destroy(host);
    return 0;
}



#ifndef DVZ_HAS_GLFW
#define DVZ_HAS_GLFW 0
#endif

#if !DVZ_HAS_GLFW
/**
 * Requesting GLFW should fall back to the headless backend when it is unavailable.
 */
int test_window_fallback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_GLFW, NULL);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_OFFSCREEN);
    dvz_window_host_destroy(host);
    return 0;
}
#else
int test_window_fallback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    // GLFW is enabled; fallback behaviour not exercised.
    return 0;
}
#endif



/**
 * Verify required-extension query returns empty set for the headless backend.
 */
int test_window_required_extensions_headless(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    AT(dvz_window_host_required_extension_count(host, DVZ_BACKEND_OFFSCREEN) == 0);
    AT(dvz_window_host_required_extensions(host, DVZ_BACKEND_OFFSCREEN, 0, NULL) == 0);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Register the window tests to the suite.
 */
int test_window(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "window";
    TST_MODULE(suite, tags);
    TST_CASE(test_window_headless);
    TST_CASE(test_window_config_rejects_invalid_abi);
    TST_CASE(test_window_resize_events);
    TST_CASE(test_window_frame_requests);
    TST_CASE(test_window_wait_hooks_headless);
    TST_CASE(test_window_fallback);
    TST_CASE(test_window_wrap_create);
    TST_CASE(test_window_wrap_attach_detach);
    TST_CASE(test_window_required_extensions_headless);
    TST_CASE(test_window_required_extensions_wrap);
    TST_CASE(test_window_wrap_invalid_args);
    TST_CASE(test_window_required_extensions_invalid_args);
    TST_CASE(test_window_wrap_replace_surface);
    TST_CASE(test_window_wrap_owned_surface_null_lifecycle);
    return 0;
}
