/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App — presentation layer                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzApp       DvzApp;
typedef struct DvzAppConfig DvzAppConfig;
typedef struct DvzAppWindow DvzAppWindow;
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

typedef void (*DvzAppFrameCallback)(DvzAppWindow* win, void* user_data);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAppConfig
{
    uint32_t instance_extension_count;
    const char* const* instance_extensions;
    bool enable_canvas_extensions;
    bool enable_glfw_extensions;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

/**
 * Return the default app configuration.
 *
 * @return the default app configuration
 */
DVZ_EXPORT DvzAppConfig dvz_app_config(void);


/**
 * Create an app bound to a scene.
 *
 * Allocates a GPU context, a DRP2 runtime, and a window host.  Returns NULL when no suitable
 * GPU is available.
 *
 * @param scene the scene (borrowed — must outlive the app)
 * @return the app, or NULL on failure
 */
DVZ_EXPORT DvzApp* dvz_app(DvzScene* scene);


/**
 * Create an app bound to a scene with explicit host integration requirements.
 *
 * Use this entry point when an external UI toolkit owns the native window and requires Vulkan
 * instance extensions before Datoviz creates its GPU context.
 *
 * @param scene the scene (borrowed — must outlive the app)
 * @param config optional app configuration, or NULL for dvz_app_config()
 * @return the app, or NULL on failure
 */
DVZ_EXPORT DvzApp* dvz_app_with_config(DvzScene* scene, const DvzAppConfig* config);


/**
 * Destroy the app and all owned resources (canvases, windows, runtime, GPU context).
 *
 * @param app the app
 */
DVZ_EXPORT void dvz_app_destroy(DvzApp* app);



/*************************************************************************************************/
/*  Window management                                                                            */
/*************************************************************************************************/

/**
 * Create an offscreen canvas window for a figure.
 *
 * The figure's panels and visuals are rendered into an offscreen framebuffer of the given size
 * on every call to dvz_app_run().
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param width framebuffer width in pixels
 * @param height framebuffer height in pixels
 * @return the app-window handle, or NULL on failure
 */
DVZ_EXPORT DvzAppWindow*
dvz_app_window(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height);


/**
 * Create an interactive GLFW window for a figure.
 *
 * Opens a visible window backed by a present (swapchain) canvas.  The frame loop started by
 * dvz_app_run(app, 0) drives rendering until the user closes the window.
 *
 * Requires that the platform supports GLFW and that a display is available.  Returns NULL when
 * GLFW is unavailable or window creation fails.
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param width window width in pixels
 * @param height window height in pixels
 * @param title window title string, or NULL for a default title
 * @return the app-window handle, or NULL on failure
 */
DVZ_EXPORT DvzAppWindow* dvz_app_window_glfw(
    DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height, const char* title);


/**
 * Create a hosted present window around an externally-owned Vulkan surface.
 *
 * The caller owns the native event loop and must create the Vulkan surface using the instance
 * extensions passed to dvz_app_with_config().  Datoviz owns only the rendering objects built on
 * top of the supplied surface unless surface->owned_by_datoviz is true.
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param surface external Vulkan surface description
 * @return the app-window handle, or NULL on failure
 */
DVZ_EXPORT DvzAppWindow* dvz_app_window_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Update the hosted external surface associated with an app-window.
 *
 * Use this when the host toolkit recreates or resizes its native surface.  A NULL surface handle is
 * accepted to mark the surface temporarily unavailable; rendering then returns
 * DVZ_CANVAS_FRAME_WAIT_SURFACE until a valid surface is supplied again.
 *
 * @param win app-window created with dvz_app_window_external_surface()
 * @param surface external Vulkan surface description
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_update_external_surface(
    DvzAppWindow* win, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Return the underlying DvzCanvas for a window.
 *
 * Gives access to the full canvas API (capture, video sink, live-image sink, etc.).
 *
 * @param win the app-window
 * @return the canvas, or NULL if the window was not created with GPU support
 */
DVZ_EXPORT struct DvzCanvas* dvz_app_window_canvas(DvzAppWindow* win);


/**
 * Return the input router for a GLFW app-window.
 *
 * Pass the returned router to dvz_panel_set_panzoom() or dvz_panel_set_arcball() to attach
 * interactive controllers.  Returns NULL for offscreen windows or when GPU support is absent.
 *
 * @param win the app-window
 * @return the input router, or NULL
 */
DVZ_EXPORT struct DvzInputRouter* dvz_app_window_input(DvzAppWindow* win);


/**
 * Capture the last rendered frame and write it to a PNG file.
 *
 * Convenience wrapper around dvz_app_window_canvas() + dvz_canvas_capture_png().
 * Call after at least one dvz_app_run() iteration.
 *
 * @param win the app-window
 * @param path output file path
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_capture_png(DvzAppWindow* win, const char* path);


/**
 * Register a callback invoked after each successful frame for one app-window.
 *
 * The callback runs after the scene stream has been emitted, executed, request processing has
 * completed, and the emitted stream has been destroyed. Scene mutations from the callback are
 * therefore allowed and become visible on the next frame.
 *
 * @param win the app-window
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DVZ_EXPORT void
dvz_app_window_set_frame_callback(
    DvzAppWindow* win, DvzAppFrameCallback callback, void* user_data);



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

/**
 * Render one frame for a single app-window without polling any Datoviz-owned event loop.
 *
 * This is the primary hosted-loop primitive for Qt, SDL, Tk, IPython, and other integrations where
 * the caller owns scheduling.  Returns the dvz_canvas_frame() status when no frame was submitted.
 *
 * @param win the app-window
 * @return DVZ_CANVAS_FRAME_READY after a submitted frame, DVZ_CANVAS_FRAME_WAIT_SURFACE while the
 * surface is unavailable, or a negative error code
 */
DVZ_EXPORT int dvz_app_window_render_once(DvzAppWindow* win);


/**
 * Render one frame for every app-window without polling any Datoviz-owned event loop.
 *
 * @param app the app
 * @return 0 on success, DVZ_CANVAS_FRAME_WAIT_SURFACE if any surface is unavailable, or negative on
 * error
 */
DVZ_EXPORT int dvz_app_render_once(DvzApp* app);


/**
 * Run the frame loop.
 *
 * Each iteration polls events, then calls dvz_canvas_frame() / dvz_canvas_submit() for every
 * registered window.  The draw callback attached to each canvas emits and executes the DRP2
 * command stream for its associated figure.
 *
 * @param app the app
 * @param frame_count number of frames to render (0 = interactive loop until all windows close)
 */
DVZ_EXPORT void dvz_app_run(DvzApp* app, uint32_t frame_count);


EXTERN_C_OFF
