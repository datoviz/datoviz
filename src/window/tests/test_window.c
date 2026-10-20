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
#include <math.h>
#include "_assertions.h"
#include "datoviz/common.h"
#include "datoviz/input/router.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#include "testing.h"
#include "../window_internal.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResizeRecorder
{
    uint32_t resize_count;
    uint32_t width;
    uint32_t height;
} ResizeRecorder;



typedef struct WaitRecorder
{
    uint32_t wait_count;
    uint32_t wait_timeout_count;
} WaitRecorder;



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



/**
 * Create one CPU-only test window.
 *
 * @param backend test backend descriptor
 * @param window window being created
 * @param config requested window configuration
 * @return always true
 */
static bool _window_wait_backend_create(
    DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    ANN(backend);
    ANN(window);
    ANN(config);
    return true;
}



/**
 * Record one unbounded wait on the CPU-only test backend.
 *
 * @param backend test backend descriptor
 * @param host host requesting the wait
 */
static void _window_wait_backend_wait(DvzWindowBackend* backend, DvzWindowHost* host)
{
    ANN(backend);
    ANN(host);
    WaitRecorder* recorder = (WaitRecorder*)backend->user_data;
    ANN(recorder);
    recorder->wait_count++;
}



/**
 * Record one timeout wait on the CPU-only test backend.
 *
 * @param backend test backend descriptor
 * @param host host requesting the wait
 * @param seconds requested timeout
 */
static void _window_wait_backend_wait_timeout(
    DvzWindowBackend* backend, DvzWindowHost* host, double seconds)
{
    ANN(backend);
    ANN(host);
    ASSERT(seconds > 0.0);
    WaitRecorder* recorder = (WaitRecorder*)backend->user_data;
    ANN(recorder);
    recorder->wait_timeout_count++;
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
    AT(dvz_window_metrics(window)->refresh_rate_hz == 0);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify workarea fitting uses window coordinates, decorations, and monitor origins.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
static int test_window_fit_workarea(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    const int32_t workarea[4] = {-1920, 40, 1920, 1040};
    const int32_t frame[4] = {8, 30, 8, 8};
    int32_t bounds[4] = {-2000, 0, 2800, 2000};
    _dvz_window_fit_workarea(bounds, workarea, frame);
    AT(bounds[0] == -1912 && bounds[1] == 70);
    AT(bounds[2] == 1904 && bounds[3] == 1002);
    // A smaller requested size is preserved; only its position needs fitting.
    int32_t exact[4] = {-1800, 100, 800, 600};
    _dvz_window_fit_workarea(exact, workarea, frame);
    AT(exact[0] == -1800 && exact[1] == 100);
    AT(exact[2] == 800 && exact[3] == 600);
    // Native HiDPI windows may use twice the logical extent. Fit in native coordinates.
    int32_t hidpi[4] = {300, 200, 5600, 4000};
    const int32_t native_workarea[4] = {0, 0, 3840, 2080};
    _dvz_window_fit_workarea(hidpi, native_workarea, frame);
    AT(hidpi[2] == 3824 && hidpi[3] == 2042);
    AT(hidpi[0] == 8 && hidpi[1] == 30);
    return 0;
}



/**
 * Verify the opt-in interactive fitting policy preserves exact offscreen dimensions.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
static int test_window_fit_workarea_offscreen(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindowConfig config = dvz_window_config();
    AT(!config.fit_workarea);
    config.fit_workarea = true;
    config.width = 2800;
    config.height = 2000;
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &config);
    ANN(window);
    const DvzWindowMetrics* metrics = dvz_window_metrics(window);
    AT(metrics->logical_size.width == 2800 && metrics->logical_size.height == 2000);
    AT(metrics->surface_size.width == 2800 && metrics->surface_size.height == 2000);
    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify window ownership preserves canonical history when another activation is released.
 */
static void _window_record_pointer_event(
    DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    uint32_t* counts = (uint32_t*)user_data;
    if (event->type == DVZ_INPUT_EVENT_POINTER &&
        event->content.pointer.type >= DVZ_POINTER_EVENT_RELEASE &&
        event->content.pointer.type <= DVZ_POINTER_EVENT_WHEEL)
        counts[event->content.pointer.type]++;
}



/**
 * Exercise the window-owned interpreter through real raw pointer emissions.
 */
int test_window_canonical_pointer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);
    DvzInputRouter* router = dvz_window_router(window);
    uint32_t counts[DVZ_POINTER_EVENT_WHEEL + 1] = {0};
    dvz_input_subscribe_event(router, _window_record_pointer_event, counts);
    DvzPointerGestureHandler* extra = dvz_pointer_gesture_handler(router);
    ANN(extra);
    dvz_pointer_emit_position(
        router, DVZ_POINTER_EVENT_PRESS, 10, 10, 100, 100, DVZ_POINTER_BUTTON_LEFT,
        0, 1, 1000000000, NULL);
    dvz_pointer_gesture_handler_destroy(extra);
    dvz_pointer_emit_position(
        router, DVZ_POINTER_EVENT_RELEASE, 10, 10, 100, 100, DVZ_POINTER_BUTTON_LEFT,
        0, 1, 1010000000, NULL);
    dvz_pointer_emit_wheel(router, 10, 10, 100, 100, 0, -1, 0, 1, 1020000000, NULL);
    AT(counts[DVZ_POINTER_EVENT_PRESS] == 1);
    AT(counts[DVZ_POINTER_EVENT_RELEASE] == 1);
    AT(counts[DVZ_POINTER_EVENT_CLICK] == 1);
    AT(counts[DVZ_POINTER_EVENT_WHEEL] == 1);
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

    cfg = dvz_window_config();
    AT(!cfg.has_position);
    AT(cfg.x == 0);
    AT(cfg.y == 0);

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



int test_window_backend_registration_preserves_live_windows(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_OFFSCREEN);

    uint32_t initial_capacity = host->backend_capacity;
    for (uint32_t i = 0; i <= initial_capacity; i++)
    {
        DvzWindowBackend backend = {
            .name = "inert-test-backend",
            .type = (DvzBackend)(100 + i),
        };
        dvz_window_host_register_backend(host, &backend);
    }
    AT(host->backend_capacity > initial_capacity);
    AT(dvz_window_backend_type(window) == DVZ_BACKEND_OFFSCREEN);
    dvz_window_host_request_frame(host, window);
    AT(dvz_window_frame_pending(window));
    AT(!dvz_window_should_close(window));

    dvz_window_destroy(window);
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



/**
 * Ensure interactive wait hooks take precedence when offscreen and interactive windows coexist.
 */
int test_window_wait_prefers_interactive_backend(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    WaitRecorder recorder = {0};
    DvzWindowBackend backend = {
        .name = "window-wait-test",
        .type = DVZ_BACKEND_QT,
        .user_data = &recorder,
        .procs =
            {
                .create = _window_wait_backend_create,
                .wait = _window_wait_backend_wait,
                .wait_timeout = _window_wait_backend_wait_timeout,
            },
    };

    DvzWindowHost* host = dvz_window_host();
    ANN(host);
    dvz_window_host_register_backend(host, &backend);
    DvzWindow* offscreen = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    DvzWindow* interactive = dvz_window_create(host, DVZ_BACKEND_QT, NULL);
    ANN(offscreen);
    ANN(interactive);

    dvz_window_host_wait_timeout(host, 0.001);
    AT(recorder.wait_timeout_count == 1);
    AT(recorder.wait_count == 0);

    dvz_window_host_wait(host);
    AT(recorder.wait_timeout_count == 1);
    AT(recorder.wait_count == 1);

    dvz_window_host_destroy(host);
    return 0;
}



/**
 * Verify the effective scale policy uses the explicit override first.
 */
int test_window_effective_scale_override(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowScaleInputs inputs = {
        .window_scale_x = 1.0f,
        .window_scale_y = 1.0f,
        .framebuffer_width = 3200,
        .framebuffer_height = 1800,
        .window_width = 1600,
        .window_height = 900,
        .monitor_scale_x = 2.0f,
        .monitor_scale_y = 2.0f,
        .override_scale = 1.5f,
    };
    float sx = 0.0f;
    float sy = 0.0f;
    _dvz_window_effective_content_scale(&inputs, &sx, &sy);
    AC(sx, 1.5f, 1e-6f);
    AC(sy, 1.5f, 1e-6f);
    return 0;
}



/**
 * Verify framebuffer/window ratio fixes a stale unit content scale.
 */
int test_window_effective_scale_framebuffer_ratio(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowScaleInputs inputs = {
        .window_scale_x = 1.0f,
        .window_scale_y = 1.0f,
        .framebuffer_width = 2400,
        .framebuffer_height = 1350,
        .window_width = 1600,
        .window_height = 900,
    };
    float sx = 0.0f;
    float sy = 0.0f;
    _dvz_window_effective_content_scale(&inputs, &sx, &sy);
    AC(sx, 1.5f, 1e-6f);
    AC(sy, 1.5f, 1e-6f);
    return 0;
}



/**
 * Verify monitor content scale is used when window scale and framebuffer ratio are unavailable.
 */
int test_window_effective_scale_monitor(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowScaleInputs inputs = {
        .window_scale_x = 1.0f,
        .window_scale_y = 1.0f,
        .monitor_scale_x = 2.0f,
        .monitor_scale_y = 2.0f,
    };
    float sx = 0.0f;
    float sy = 0.0f;
    _dvz_window_effective_content_scale(&inputs, &sx, &sy);
    AC(sx, 2.0f, 1e-6f);
    AC(sy, 2.0f, 1e-6f);
    return 0;
}



/**
 * Verify raw monitor DPI does not automatically become a window scale.
 */
int test_window_effective_scale_ignores_raw_dpi(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowScaleInputs inputs = {
        .window_scale_x = 1.0f,
        .window_scale_y = 1.0f,
        .monitor_pixel_width = 3024,
        .monitor_pixel_height = 1964,
        .monitor_width_mm = 302,
        .monitor_height_mm = 196,
    };
    float sx = 0.0f;
    float sy = 0.0f;
    _dvz_window_effective_content_scale(&inputs, &sx, &sy);
    AC(sx, 1.0f, 1e-6f);
    AC(sy, 1.0f, 1e-6f);

    inputs.monitor_pixel_width = 1920;
    inputs.monitor_pixel_height = 1080;
    inputs.monitor_width_mm = 509;
    inputs.monitor_height_mm = 286;
    _dvz_window_effective_content_scale(&inputs, &sx, &sy);
    AC(sx, 1.0f, 1e-6f);
    AC(sy, 1.0f, 1e-6f);
    return 0;
}



/**
 * Verify framebuffer HiDPI metrics preserve the logical window size.
 */
int test_window_metrics_framebuffer_policy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowMetricsInputs inputs = {
        .requested_logical_size = {.width = 400, .height = 300},
        .native_size = {.width = 400, .height = 300},
        .framebuffer_size = {.width = 800, .height = 600},
        .content_scale = {.x = 2.0f, .y = 2.0f},
        .requested_policy = DVZ_HIDPI_AUTO,
    };
    DvzWindowMetrics metrics = {0};
    _dvz_window_metrics_resolve(&inputs, &metrics);
    AT(metrics.active_hidpi_policy == DVZ_HIDPI_FRAMEBUFFER);
    AT(metrics.logical_size.width == 400);
    AT(metrics.logical_size.height == 300);
    AT(metrics.native_size.width == 400);
    AT(metrics.surface_size.width == 800);
    AT(metrics.render_size.width == 800);
    AC(metrics.device_scale.x, 2.0f, 1e-6f);
    AC(metrics.native_to_logical.x, 1.0f, 1e-6f);
    return 0;
}



/**
 * Verify native-window HiDPI metrics request an enlarged backend window.
 */
int test_window_metrics_native_window_policy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowMetricsInputs inputs = {
        .requested_logical_size = {.width = 900, .height = 650},
        .native_size = {.width = 900, .height = 650},
        .framebuffer_size = {.width = 900, .height = 650},
        .content_scale = {.x = 1.4549f, .y = 1.4549f},
        .requested_policy = DVZ_HIDPI_AUTO,
    };
    DvzWindowMetrics metrics = {0};
    _dvz_window_metrics_resolve(&inputs, &metrics);
    AT(metrics.active_hidpi_policy == DVZ_HIDPI_NATIVE_WINDOW);
    AT(metrics.logical_size.width == 900);
    AT(metrics.logical_size.height == 650);
    AT(metrics.native_size.width == 1309);
    AT(metrics.surface_size.width == 1309);
    AT(metrics.render_size.width == 1309);
    AC(metrics.device_scale.x, 1.4549f, 1e-6f);
    AC(metrics.native_to_logical.x, 900.0f / 1309.0f, 1e-6f);
    return 0;
}



/**
 * Verify disabled metrics keep all size spaces equal.
 */
int test_window_metrics_disabled_policy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowMetricsInputs inputs = {
        .requested_logical_size = {.width = 320, .height = 240},
        .native_size = {.width = 320, .height = 240},
        .framebuffer_size = {.width = 320, .height = 240},
        .content_scale = {.x = 1.0f, .y = 1.0f},
        .requested_policy = DVZ_HIDPI_AUTO,
    };
    DvzWindowMetrics metrics = {0};
    _dvz_window_metrics_resolve(&inputs, &metrics);
    AT(metrics.active_hidpi_policy == DVZ_HIDPI_DISABLED);
    AT(metrics.logical_size.width == 320);
    AT(metrics.native_size.width == 320);
    AT(metrics.surface_size.width == 320);
    AT(metrics.render_size.width == 320);
    AC(metrics.device_scale.x, 1.0f, 1e-6f);
    AC(metrics.native_to_logical.x, 1.0f, 1e-6f);
    return 0;
}



/**
 * Verify metrics retain an available monitor refresh rate.
 */
int test_window_metrics_refresh_rate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzWindowMetricsInputs inputs = {
        .requested_logical_size = {.width = 640, .height = 480},
        .native_size = {.width = 640, .height = 480},
        .framebuffer_size = {.width = 640, .height = 480},
        .content_scale = {.x = 1.0f, .y = 1.0f},
        .refresh_rate_hz = 144,
        .requested_policy = DVZ_HIDPI_AUTO,
    };
    DvzWindowMetrics metrics = {0};
    _dvz_window_metrics_resolve(&inputs, &metrics);
    AT(metrics.refresh_rate_hz == 144);
    return 0;
}



/**
 * Verify public metrics query tracks backend resize emissions.
 */
int test_window_metrics_query_updates_on_resize(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    ANN(window);

    const DvzWindowMetrics* metrics = dvz_window_metrics(window);
    ANN(metrics);
    DvzWindowMetrics emitted = *metrics;
    emitted.refresh_rate_hz = 144;
    _dvz_window_backend_emit_metrics(window, &emitted);
    metrics = dvz_window_metrics(window);
    uint64_t generation = metrics->generation;

    dvz_window_backend_emit_resize(window, 800, 600, 400, 300, 2.0f, 2.0f);
    metrics = dvz_window_metrics(window);
    AT(metrics->generation == generation + 1);
    AT(metrics->logical_size.width == 400);
    AT(metrics->surface_size.width == 800);
    AT(metrics->render_size.width == 800);
    AT(metrics->refresh_rate_hz == 144);
    AC(metrics->device_scale.x, 2.0f, 1e-6f);

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



/** Host observation/GUI/scene order and state captured by the regression fixture. */
typedef struct HostInputRecorder
{
    uint32_t observed;
    uint32_t arbitrated;
    uint32_t routed;
    uint32_t clicks;
    uint32_t cancels;
    uint32_t releases;
    uint32_t text_size;
    DvzPointerEvent pointer;
    DvzCallbackId remove_id;
    DvzCallbackId add_id;
    bool mutate;
    bool nested_rejected;
} HostInputRecorder;



/** Observe originals before arbitration, including mutation and nested-emission safeguards. */
static void _window_host_observe(
    DvzWindow* window, const DvzHostInputEvent* event, DvzHostInputOrigin origin, void* user_data)
{
    HostInputRecorder* recorder = (HostInputRecorder*)user_data;
    recorder->observed++;
    if (recorder->mutate)
    {
        recorder->mutate = false;
        dvz_window_unsubscribe_host_input(window, recorder->remove_id);
        recorder->add_id = dvz_window_subscribe_host_input(window, _window_host_observe, user_data);
        recorder->nested_rejected =
            dvz_window_emit_host_input(window, event, origin) == DVZ_ERROR;
    }
}



/** Consume an original after its observer has seen it, without routing scene input. */
static bool _window_host_arbitrate(
    DvzWindow* window, const DvzHostInputEvent* event, void* user_data)
{
    (void)window;
    (void)event;
    HostInputRecorder* recorder = (HostInputRecorder*)user_data;
    if (recorder->observed > recorder->arbitrated)
        recorder->arbitrated++;
    return true;
}



/** Record actual router outputs, separating cancellations from successful releases. */
static void _window_host_routed(
    DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    HostInputRecorder* recorder = (HostInputRecorder*)user_data;
    recorder->routed++;
    if (event->type == DVZ_INPUT_EVENT_POINTER)
    {
        recorder->pointer = event->content.pointer;
        recorder->clicks += event->content.pointer.type == DVZ_POINTER_EVENT_CLICK;
        recorder->cancels += event->content.pointer.type == DVZ_POINTER_EVENT_CANCEL;
        recorder->releases += event->content.pointer.type == DVZ_POINTER_EVENT_RELEASE;
    }
    if (event->type == DVZ_INPUT_EVENT_TEXT)
        recorder->text_size = event->content.text.byte_size;
}



/** Originals are observable before GUI consumption, with stable subscription mutation. */
static int test_window_host_input_ingress(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    HostInputRecorder recorder = {0};
    DvzCallbackId first = dvz_window_subscribe_host_input(window, _window_host_observe, &recorder);
    recorder.remove_id = dvz_window_subscribe_host_input(window, _window_host_observe, &recorder);
    DvzCallbackId routed = dvz_input_subscribe_event(window->router, _window_host_routed, &recorder);
    recorder.mutate = true;
    _dvz_window_set_host_input_dispatch(window, _window_host_arbitrate, NULL, NULL, &recorder);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_MOVE, .x = 12.375, .y = 23.125,
                              .timestamp_ns = 100};
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_OK);
    AT(recorder.observed == 1 && recorder.arbitrated == 1 && recorder.routed == 0);
    AT(recorder.nested_rejected);
    AT(dvz_window_host_input_state(window)->x == 12.375);
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_OK);
    AT(recorder.observed == 3 && recorder.arbitrated == 2 && recorder.routed == 0);
    AT(dvz_window_unsubscribe_host_input(window, first));
    AT(dvz_window_unsubscribe_host_input(window, recorder.add_id));
    AT(dvz_input_unsubscribe(window->router, routed));
    _dvz_window_set_host_input_dispatch(window, NULL, NULL, NULL, NULL);
    dvz_window_host_destroy(host);
    return 0;
}



/** Full-fidelity host state feeds the canonical router with exactly one normalization. */
static int test_window_host_input_routing(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    HostInputRecorder recorder = {0};
    DvzCallbackId id = dvz_input_subscribe_event(window->router, _window_host_routed, &recorder);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_MOVE, .x = 12.375, .y = 23.125,
                              .timestamp_ns = 1234567};
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_OK);
    AT(recorder.routed == 1 && recorder.pointer.timestamp_ns == event.timestamp_ns);
    AC(recorder.pointer.pos[0], 12.375f, 1e-6f);
    event.type = DVZ_HOST_INPUT_WHEEL;
    event.dx = 0.125;
    event.dy = -0.375;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_OK);
    AC(recorder.pointer.content.w.dir[0], 0.125f, 1e-6f);
#if defined(__APPLE__)
    AC(recorder.pointer.content.w.dir[1], 0.375f, 1e-6f);
#else
    AC(recorder.pointer.content.w.dir[1], -0.375f, 1e-6f);
#endif
    event.type = DVZ_HOST_INPUT_TEXT;
    event.utf8 = "\xc3\xa9\xe6\xb0\xb4";
    event.byte_size = 5;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_OK);
    AT(recorder.text_size == 5);
    dvz_input_unsubscribe(window->router, id);
    dvz_window_host_destroy(host);
    return 0;
}



/** Replay ownership rejects physical input and cancellation never completes a click. */
static int test_window_host_input_replay_cancel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    HostInputRecorder recorder = {0};
    DvzCallbackId id = dvz_input_subscribe_event(window->router, _window_host_routed, &recorder);
    AT(dvz_window_input_replay(window, true) == DVZ_OK);
    AT(dvz_window_input_frame(window, 1000000000, 0.01) == DVZ_OK);
    AT(dvz_window_input_pending(window) == 0);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_BUTTON, .button = 0, .action = 1,
                              .timestamp_ns = 1000000000};
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_ERROR);
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_REPLAY) == DVZ_OK);
    AT(dvz_window_host_input_state(window)->buttons == 1);
    event.type = DVZ_HOST_INPUT_KEY;
    event.key = 87;
    event.mods = DVZ_KEY_MODIFIER_SHIFT;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_REPLAY) == DVZ_OK);
    AT(dvz_window_host_input_state(window)->held_key_count == 1);
    event.type = DVZ_HOST_INPUT_FOCUS;
    event.focused = false;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_REPLAY) == DVZ_OK);
    AT(dvz_window_host_input_state(window)->buttons == 0);
    AT(dvz_window_host_input_state(window)->held_key_count == 0);
    AT(dvz_window_host_input_state(window)->mods == 0);
    AT(recorder.clicks == 0 && recorder.releases == 0 && recorder.cancels == 2);
    AT(_dvz_window_cancelled_release(window, true, 87));
    AT(!_dvz_window_cancelled_release(window, true, 87));
    AT(_dvz_window_cancelled_release(window, false, 0));
    AT(!_dvz_window_cancelled_release(window, false, 0));
    AT(dvz_window_input_replay(window, false) == DVZ_OK);
    AT(!dvz_window_host_input_state(window)->replaying);
    AT(dvz_window_input_frame(window, 1, 0.01) == DVZ_ERROR);
    dvz_input_unsubscribe(window->router, id);
    dvz_window_host_destroy(host);
    return 0;
}



/** Invalid observations fail before observers or virtual-state mutation. */
static int test_window_host_input_invalid(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzWindowHost* host = dvz_window_host();
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, NULL);
    HostInputRecorder recorder = {0};
    DvzCallbackId id = dvz_window_subscribe_host_input(window, _window_host_observe, &recorder);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_TEXT, .utf8 = "\xc0\xaf", .byte_size = 2};
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_ERROR);
    event.type = DVZ_HOST_INPUT_MOVE;
    event.x = NAN;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_ERROR);
    event.x = 0;
    event.type = DVZ_HOST_INPUT_METRICS;
    AT(dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE) == DVZ_ERROR);
    AT(recorder.observed == 0);
    dvz_window_unsubscribe_host_input(window, id);
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
    TST_CASE(test_window_host_input_ingress);
    TST_CASE(test_window_host_input_routing);
    TST_CASE(test_window_host_input_replay_cancel);
    TST_CASE(test_window_host_input_invalid);
    TST_CASE(test_window_headless);
    TST_CASE(test_window_canonical_pointer);
    TST_CASE(test_window_fit_workarea);
    TST_CASE(test_window_fit_workarea_offscreen);
    TST_CASE(test_window_config_rejects_invalid_abi);
    TST_CASE(test_window_resize_events);
    TST_CASE(test_window_frame_requests);
    TST_CASE(test_window_backend_registration_preserves_live_windows);
    TST_CASE(test_window_wait_hooks_headless);
    TST_CASE(test_window_wait_prefers_interactive_backend);
    TST_CASE(test_window_effective_scale_override);
    TST_CASE(test_window_effective_scale_framebuffer_ratio);
    TST_CASE(test_window_effective_scale_monitor);
    TST_CASE(test_window_effective_scale_ignores_raw_dpi);
    TST_CASE(test_window_metrics_framebuffer_policy);
    TST_CASE(test_window_metrics_native_window_policy);
    TST_CASE(test_window_metrics_disabled_policy);
    TST_CASE(test_window_metrics_refresh_rate);
    TST_CASE(test_window_metrics_query_updates_on_resize);
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
