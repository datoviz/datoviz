/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App — presentation layer                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#include "datoviz/canvas.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_APP_MAX_WINDOWS        16
#define DVZ_APP_CANVAS_TARGET_BASE 0xF000ULL



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAppWindow
{
    DvzApp*    app;
    DvzFigure* figure;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzWindow* window;
    DvzCanvas* canvas;
#endif
    uint64_t   target_id;
    bool       is_interactive;
};


struct DvzApp
{
    DvzScene* scene;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx*      gpu_ctx;
    DvzDrp2Runtime* runtime;
    DvzWindowHost*  window_host;
#endif
    uint32_t     window_count;
    DvzAppWindow windows[DVZ_APP_MAX_WINDOWS];
};



/*************************************************************************************************/
/*  Draw callback                                                                                */
/*************************************************************************************************/

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE

static void _app_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzAppWindow* win = (DvzAppWindow*)user_data;
    ANN(win);
    DvzApp* app = win->app;
    ANN(app);

    /* Attach the canvas frame to the reserved DRP2 texture ID. */
    if (!dvz_drp2_runtime_attach_frame_target(app->runtime, win->target_id, frame))
    {
        log_error("_app_draw failed to attach canvas frame target");
        return;
    }

    /* Emit the DRP2 command stream with the canvas as external color target. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format         = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.external_color_target = true;
    cfg.color_target_id       = win->target_id;
    cfg.clear_color[0]        = 0.05f;
    cfg.clear_color[1]        = 0.05f;
    cfg.clear_color[2]        = 0.08f;
    cfg.clear_color[3]        = 1.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(win->figure, &caps, &report, &cfg);
    if (stream == NULL)
    {
        uint32_t n = dvz_diagnostic_report_count(&report);
        for (uint32_t i = 0; i < n; i++)
            log_error("_app_draw emit failed: %s", dvz_diagnostic_report_get(&report, i));
        return;
    }

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(app->runtime, stream);
    if (!result.ok)
        log_error("_app_draw runtime execution failed at command %" PRIu32, result.command_index);
    dvz_drp2_stream_destroy(stream);
}

#endif



/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

DvzApp* dvz_app(DvzScene* scene)
{
    ANN(scene);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzApp* app = (DvzApp*)dvz_calloc(1, sizeof(DvzApp));
    if (app == NULL)
        return NULL;
    app->scene = scene;

    /* Window host first — needed to query GLFW surface extensions before building the instance. */
    app->window_host = dvz_window_host();
    if (app->window_host == NULL)
    {
        dvz_free(app);
        return NULL;
    }

    /* GPU context — request dynamic rendering, synchronization2, and timeline semaphores. */
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);

#if DVZ_HAS_GLFW
    /* If GLFW is available, add surface extensions so the same instance supports windowed mode. */
    if (dvz_window_glfw_init())
    {
        uint32_t ext_count =
            dvz_window_host_required_extension_count(app->window_host, DVZ_BACKEND_GLFW);
        if (ext_count > 0)
        {
            const char* extensions[16] = {0};
            int written = dvz_window_host_required_extensions(
                app->window_host, DVZ_BACKEND_GLFW, ext_count, extensions);
            if (written == (int)ext_count)
            {
                for (uint32_t i = 0; i < ext_count; i++)
                    dvz_gpu_ctx_config_add_instance_extension(&gpu_cfg, extensions[i]);
                dvz_gpu_ctx_config_enable_canvas_extensions(&gpu_cfg, true);
            }
        }
    }
#endif

    app->gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
    if (app->gpu_ctx == NULL)
    {
        dvz_window_host_destroy(app->window_host);
        dvz_free(app);
        return NULL;
    }

    /* DRP2 runtime backed by vklite. */
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(app->gpu_ctx), dvz_gpu_ctx_alloc(app->gpu_ctx));
    app->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (app->runtime == NULL)
    {
        dvz_gpu_ctx_destroy(app->gpu_ctx);
        dvz_window_host_destroy(app->window_host);
        dvz_free(app);
        return NULL;
    }

    return app;
#else
    (void)scene;
    return NULL;
#endif
}


void dvz_app_destroy(DvzApp* app)
{
    if (app == NULL)
        return;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    for (uint32_t i = 0; i < app->window_count; i++)
    {
        DvzAppWindow* win = &app->windows[i];
        if (win->canvas != NULL)
        {
            dvz_canvas_destroy(win->canvas);
            win->canvas = NULL;
        }
        if (win->window != NULL)
        {
            dvz_window_destroy(win->window);
            win->window = NULL;
        }
    }
    if (app->window_host != NULL)
    {
        dvz_window_host_destroy(app->window_host);
        app->window_host = NULL;
    }
    if (app->runtime != NULL)
    {
        dvz_drp2_runtime_destroy(app->runtime);
        app->runtime = NULL;
    }
    if (app->gpu_ctx != NULL)
    {
        dvz_gpu_ctx_destroy(app->gpu_ctx);
        app->gpu_ctx = NULL;
    }
#endif

    dvz_free(app);
}



/*************************************************************************************************/
/*  Window management                                                                            */
/*************************************************************************************************/

DvzAppWindow*
dvz_app_window(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->window_count >= DVZ_APP_MAX_WINDOWS)
        return NULL;

    /* Create the offscreen window. */
    DvzWindowConfig wcfg = dvz_window_default_config();
    wcfg.width           = width;
    wcfg.height          = height;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_OFFSCREEN, &wcfg);
    if (window == NULL)
        return NULL;

    /* Create an offscreen canvas. */
    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window          = window;
    ccfg.device          = dvz_gpu_ctx_device(app->gpu_ctx);
    ccfg.render_mode     = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzAppWindow* win = &app->windows[app->window_count];
    win->app           = app;
    win->figure        = figure;
    win->window        = window;
    win->canvas        = canvas;
    win->target_id     = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->window_count;
    app->window_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    return win;
#else
    (void)width;
    (void)height;
    return NULL;
#endif
}



DvzAppWindow*
dvz_app_window_glfw(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height,
                    const char* title)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE && DVZ_HAS_GLFW
    if (app->window_count >= DVZ_APP_MAX_WINDOWS)
        return NULL;

    DvzWindowConfig wcfg = dvz_window_default_config();
    wcfg.width  = width;
    wcfg.height = height;
    if (title != NULL)
        wcfg.title = title;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_GLFW, &wcfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        if (window != NULL)
            dvz_window_destroy(window);
        return NULL;
    }

    /* Poll once so the initial resize event sets the surface extent. */
    dvz_window_host_poll(app->window_host);

    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window = window;
    ccfg.device = dvz_gpu_ctx_device(app->gpu_ctx);
    /* render_mode defaults to DVZ_CANVAS_RENDER_MODE_PRESENT */
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzAppWindow* win = &app->windows[app->window_count];
    win->app            = app;
    win->figure         = figure;
    win->window         = window;
    win->canvas         = canvas;
    win->target_id      = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->window_count;
    win->is_interactive = true;
    app->window_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    return win;
#else
    (void)width;
    (void)height;
    (void)title;
    return NULL;
#endif
}



/*************************************************************************************************/
/*  Window accessors                                                                             */
/*************************************************************************************************/

struct DvzCanvas* dvz_app_window_canvas(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return win->canvas;
#else
    return NULL;
#endif
}


int dvz_app_window_capture_png(DvzAppWindow* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return dvz_canvas_capture_png(win->canvas, path);
#else
    return -1;
#endif
}



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

void dvz_app_run(DvzApp* app, uint32_t frame_count)
{
    ANN(app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (frame_count == 0)
    {
        /* Interactive mode: loop until every interactive window requests close. */
        for (;;)
        {
            dvz_window_host_poll(app->window_host);
            bool any_open = false;
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                if (win->is_interactive && !dvz_window_should_close(win->window))
                    any_open = true;
            }
            if (!any_open)
                break;
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                int rc = dvz_canvas_frame(win->canvas);
                if (rc == DVZ_CANVAS_FRAME_READY)
                    dvz_canvas_submit(win->canvas);
            }
        }
    }
    else
    {
        for (uint32_t f = 0; f < frame_count; f++)
        {
            dvz_window_host_poll(app->window_host);
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                int rc = dvz_canvas_frame(win->canvas);
                if (rc == DVZ_CANVAS_FRAME_READY)
                    dvz_canvas_submit(win->canvas);
            }
        }
    }
#else
    (void)frame_count;
#endif
}
