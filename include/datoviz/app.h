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

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzApp       DvzApp;
typedef struct DvzAppWindow DvzAppWindow;



EXTERN_C_ON

/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

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
 * Return the underlying DvzCanvas for a window.
 *
 * Gives access to the full canvas API (capture, video sink, live-image sink, etc.).
 *
 * @param win the app-window
 * @return the canvas, or NULL if the window was not created with GPU support
 */
DVZ_EXPORT struct DvzCanvas* dvz_app_window_canvas(DvzAppWindow* win);


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



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

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
