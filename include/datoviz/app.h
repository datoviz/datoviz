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

#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/enums.h"
#include "datoviz/input/keycodes.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzApp       DvzApp;
typedef struct DvzAppConfig DvzAppConfig;
typedef struct DvzAppWindow DvzAppWindow;
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

typedef void (*DvzAppFrameCallback)(DvzAppWindow* win, void* user_data);
typedef void (*DvzAppRequestFrameCallback)(DvzAppWindow* win, void* user_data);



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
 * Return the Vulkan instance owned by the app.
 *
 * Hosted integrations use this borrowed handle to create their native VkSurfaceKHR after passing
 * required instance extensions to dvz_app_with_config().
 *
 * @param app the app
 * @return borrowed Vulkan instance handle, or VK_NULL_HANDLE when unavailable
 */
DVZ_EXPORT VkInstance dvz_app_vk_instance(DvzApp* app);


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
 * Release a hosted external surface before the host destroys it.
 *
 * Clears the request-frame callback, marks the surface temporarily unavailable, and runs one
 * render-once step so the present swapchain observes the unavailable surface and releases borrowed
 * surface-dependent objects. The host remains responsible for destroying the VkSurfaceKHR.
 *
 * @param win app-window created with dvz_app_window_external_surface()
 * @return DVZ_CANVAS_FRAME_WAIT_SURFACE on clean release, or a negative error code
 */
DVZ_EXPORT int dvz_app_window_release_external_surface(DvzAppWindow* win);


/**
 * Emit a hosted resize event for an app-window.
 *
 * External UI adapters call this after host resize notifications so Datoviz controllers and figure
 * sizing see the host's logical and framebuffer dimensions.
 *
 * @param win the app-window
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_emit_resize(
    DvzAppWindow* win, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y);


/**
 * Emit a hosted pointer position/button event for an app-window.
 *
 * @param win the app-window
 * @param type pointer event type
 * @param x pointer x position in host-window coordinates
 * @param y pointer y position in host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param button pointer button, or DVZ_POINTER_BUTTON_NONE
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_emit_pointer(
    DvzAppWindow* win, DvzPointerEventType type, float x, float y, float window_width,
    float window_height, DvzPointerButton button, int mods);


/**
 * Emit a hosted pointer wheel event for an app-window.
 *
 * @param win the app-window
 * @param x pointer x position in host-window coordinates
 * @param y pointer y position in host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param dx horizontal wheel delta
 * @param dy vertical wheel delta
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_emit_wheel(
    DvzAppWindow* win, float x, float y, float window_width, float window_height, float dx,
    float dy, int mods);


/**
 * Emit a hosted keyboard event for an app-window.
 *
 * @param win the app-window
 * @param type keyboard event type
 * @param key Datoviz key code
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int
dvz_app_window_emit_key(DvzAppWindow* win, DvzKeyboardEventType type, DvzKeyCode key, int mods);


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
 * Return the input router for an app-window.
 *
 * Pass the returned router to dvz_panel_set_panzoom() or dvz_panel_set_arcball() to attach
 * interactive controllers. Returns NULL when GPU support is absent.
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
 * Resize an app-window's logical and framebuffer extent.
 *
 * Intended for offscreen or externally-hosted windows whose size is controlled by another UI
 * toolkit. GLFW windows should normally be resized by the platform window itself.
 *
 * @param win the app-window
 * @param width width in pixels
 * @param height height in pixels
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_app_window_resize(DvzAppWindow* win, uint32_t width, uint32_t height);


/**
 * Enable or disable rendering for an app-window.
 *
 * Disabled windows remain owned by the app but dvz_app_window_render_once() skips them. This is
 * intended for hosted/offscreen integrations such as hidden dock tabs.
 *
 * @param win the app-window
 * @param enabled whether rendering should be enabled
 */
DVZ_EXPORT void dvz_app_window_set_render_enabled(DvzAppWindow* win, bool enabled);


/**
 * Return whether rendering is enabled for an app-window.
 *
 * @param win the app-window
 * @return whether rendering is enabled
 */
DVZ_EXPORT bool dvz_app_window_render_enabled(const DvzAppWindow* win);


/**
 * Request that the host schedules another frame for an app-window.
 *
 * Hosted UI adapters should map the registered callback to their native repaint primitive, for
 * example QWindow::requestUpdate(), QWidget::update(), an SDL wakeup, or a Tk idle callback.
 *
 * @param win the app-window
 */
DVZ_EXPORT void dvz_app_window_request_frame(DvzAppWindow* win);


/**
 * Register a callback invoked whenever Datoviz requests another frame.
 *
 * This is a passive scheduling signal: it does not change dvz_app_run() behavior and does not
 * render by itself. The host remains responsible for calling dvz_app_window_render_once().
 *
 * @param win the app-window
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DVZ_EXPORT void dvz_app_window_set_request_frame_callback(
    DvzAppWindow* win, DvzAppRequestFrameCallback callback, void* user_data);


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
