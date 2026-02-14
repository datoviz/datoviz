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

#include <string.h>

#include "test_window.h"
#include "_assertions.h"
#include "datoviz/window.h"
#include "testing.h"
#include "wrap_surface_fixture.h"



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
int test_window_headless(TstSuite* suite, TstItem* item)
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



/**
 * Verify resize events propagate through the router.
 */
int test_window_resize_events(TstSuite* suite, TstItem* item)
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
int test_window_frame_requests(TstSuite* suite, TstItem* item)
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



#ifndef DVZ_HAS_GLFW
#define DVZ_HAS_GLFW 0
#endif

#if !DVZ_HAS_GLFW
/**
 * Requesting GLFW should fall back to the headless backend when it is unavailable.
 */
int test_window_fallback(TstSuite* suite, TstItem* item)
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
int test_window_fallback(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    // GLFW is enabled; fallback behaviour not exercised.
    return 0;
}
#endif



/**
 * Ensure wrap windows can be created and selected explicitly.
 */
int test_window_wrap_create(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_WRAP, NULL);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_WRAP);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify wrap external-surface attach, update, and detach APIs.
 */
int test_window_wrap_attach_detach(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_WRAP, NULL);
    ANN(window);

    DvzWindowExternalSurfaceInfo info = dvz_test_wrap_surface_info(
        (VkInstance)0x1, (VkSurfaceKHR)0x2, 640, 480, 2.0f, 1.5f, false);
    AT(dvz_window_wrap_attach_surface(window, &info) == 0);
    const DvzWindowSurface* surface = dvz_window_surface(window);
    ANN(surface);
    AT(surface->instance == info.instance);
    AT(surface->surface == info.surface);
    AT(surface->extent.width == info.extent.width);
    AT(surface->extent.height == info.extent.height);
    AT(surface->scale_x == info.scale_x);
    AT(surface->scale_y == info.scale_y);

    DvzWindowExternalSurfaceInfo update_loss =
        dvz_test_wrap_surface_info(VK_NULL_HANDLE, VK_NULL_HANDLE, 640, 480, 1.0f, 1.0f, false);
    AT(dvz_window_wrap_update_surface(window, &update_loss) == 0);
    surface = dvz_window_surface(window);
    AT(surface->instance == VK_NULL_HANDLE);
    AT(surface->surface == VK_NULL_HANDLE);

    AT(dvz_window_wrap_attach_surface(window, &info) == 0);
    dvz_window_wrap_detach_surface(window);
    surface = dvz_window_surface(window);
    AT(surface->instance == VK_NULL_HANDLE);
    AT(surface->surface == VK_NULL_HANDLE);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify required-extension query returns empty set for the headless backend.
 */
int test_window_required_extensions_headless(TstSuite* suite, TstItem* item)
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
 * Verify wrap backend extension list roundtrips through host query APIs.
 */
int test_window_required_extensions_wrap(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    const char* required[] = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_EXT_metal_surface"};
    AT(dvz_window_wrap_set_required_extensions(host, 2, required) == 0);
    AT(dvz_window_host_required_extension_count(host, DVZ_BACKEND_WRAP) == 2);
    const char* out[2] = {0};
    AT(dvz_window_host_required_extensions(host, DVZ_BACKEND_WRAP, 2, out) == 2);
    AT(strcmp(out[0], required[0]) == 0);
    AT(strcmp(out[1], required[1]) == 0);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify wrap API rejects invalid arguments and invalid handle tuples.
 */
int test_window_wrap_invalid_args(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* wrap_window = dvz_window_create(host, DVZ_BACKEND_WRAP, NULL);
    ANN(wrap_window);
    DvzWindow* offscreen_window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(offscreen_window);

    DvzWindowExternalSurfaceInfo valid = dvz_test_wrap_surface_info(
        (VkInstance)0x1, (VkSurfaceKHR)0x2, 320, 240, 1.0f, 1.0f, false);
    DvzWindowExternalSurfaceInfo invalid_tuple = valid;
    invalid_tuple.instance = VK_NULL_HANDLE;

    AT(dvz_window_wrap_attach_surface(NULL, &valid) == -1);
    AT(dvz_window_wrap_attach_surface(wrap_window, NULL) == -1);
    AT(dvz_window_wrap_attach_surface(wrap_window, &invalid_tuple) == -1);
    AT(dvz_window_wrap_attach_surface(wrap_window, &(DvzWindowExternalSurfaceInfo){0}) == -1);
    AT(dvz_window_wrap_attach_surface(offscreen_window, &valid) == -1);

    AT(dvz_window_wrap_update_surface(NULL, &valid) == -1);
    AT(dvz_window_wrap_update_surface(wrap_window, NULL) == -1);
    AT(dvz_window_wrap_update_surface(wrap_window, &invalid_tuple) == -1);
    AT(dvz_window_wrap_update_surface(offscreen_window, &valid) == -1);

    dvz_window_wrap_detach_surface(NULL);
    dvz_window_wrap_detach_surface(offscreen_window);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify extension query APIs reject invalid input and unavailable backends.
 */
int test_window_required_extensions_invalid_args(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    const char* name = VK_KHR_SURFACE_EXTENSION_NAME;
    AT(dvz_window_wrap_set_required_extensions(NULL, 1, &name) == -1);
    AT(dvz_window_wrap_set_required_extensions(host, 1, NULL) == -1);
    AT(dvz_window_wrap_set_required_extensions(host, 0, NULL) == 0);
    AT(dvz_window_host_required_extensions(NULL, DVZ_BACKEND_WRAP, 0, NULL) == -1);
    AT(dvz_window_host_required_extensions(host, DVZ_BACKEND_QT, 0, NULL) == -1);
    AT(dvz_window_host_required_extensions(host, DVZ_BACKEND_WRAP, 1, NULL) == -1);
    AT(dvz_window_host_required_extension_count(NULL, DVZ_BACKEND_WRAP) == 0);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify wrap update can replace an existing external surface and apply new metadata.
 */
int test_window_wrap_replace_surface(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_WRAP, NULL);
    ANN(window);

    DvzWindowExternalSurfaceInfo first = dvz_test_wrap_surface_info(
        (VkInstance)0x100, (VkSurfaceKHR)0x200, 640, 480, 1.0f, 1.0f, false);
    AT(dvz_window_wrap_attach_surface(window, &first) == 0);
    const DvzWindowSurface* surface = dvz_window_surface(window);
    ANN(surface);
    AT(surface->instance == first.instance);
    AT(surface->surface == first.surface);

    DvzWindowExternalSurfaceInfo second = dvz_test_wrap_surface_info(
        (VkInstance)0x300, (VkSurfaceKHR)0x400, 800, 600, 2.0f, 2.0f, false);
    AT(dvz_window_wrap_update_surface(window, &second) == 0);
    surface = dvz_window_surface(window);
    ANN(surface);
    AT(surface->instance == second.instance);
    AT(surface->surface == second.surface);
    AT(surface->extent.width == second.extent.width);
    AT(surface->extent.height == second.extent.height);
    AT(surface->scale_x == second.scale_x);
    AT(surface->scale_y == second.scale_y);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify owned-by-dataviz lifecycle is safe when the wrap surface transitions to null handles.
 */
int test_window_wrap_owned_surface_null_lifecycle(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_WRAP, NULL);
    ANN(window);

    DvzWindowExternalSurfaceInfo attached = dvz_test_wrap_surface_info(
        (VkInstance)0x111, (VkSurfaceKHR)0x222, 320, 240, 1.0f, 1.0f, false);
    AT(dvz_window_wrap_attach_surface(window, &attached) == 0);

    DvzWindowExternalSurfaceInfo owned_null =
        dvz_test_wrap_surface_info(VK_NULL_HANDLE, VK_NULL_HANDLE, 320, 240, 1.0f, 1.0f, true);
    AT(dvz_window_wrap_update_surface(window, &owned_null) == 0);
    const DvzWindowSurface* surface = dvz_window_surface(window);
    ANN(surface);
    AT(surface->instance == VK_NULL_HANDLE);
    AT(surface->surface == VK_NULL_HANDLE);

    dvz_window_destroy(window);
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
    TEST_SIMPLE(test_window_headless);
    TEST_SIMPLE(test_window_resize_events);
    TEST_SIMPLE(test_window_frame_requests);
    TEST_SIMPLE(test_window_fallback);
    TEST_SIMPLE(test_window_wrap_create);
    TEST_SIMPLE(test_window_wrap_attach_detach);
    TEST_SIMPLE(test_window_required_extensions_headless);
    TEST_SIMPLE(test_window_required_extensions_wrap);
    TEST_SIMPLE(test_window_wrap_invalid_args);
    TEST_SIMPLE(test_window_required_extensions_invalid_args);
    TEST_SIMPLE(test_window_wrap_replace_surface);
    TEST_SIMPLE(test_window_wrap_owned_surface_null_lifecycle);
    return 0;
}
