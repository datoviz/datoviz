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
#include "datoviz/font.h"
#include "datoviz/input/enums.h"
#include "datoviz/input/keycodes.h"
#include "datoviz/scene/types.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzApp       DvzApp;
typedef struct DvzAppCaptureConfig DvzAppCaptureConfig;
typedef struct DvzAppConfig DvzAppConfig;
typedef struct DvzAppResources DvzAppResources;
typedef struct DvzViewDesc DvzViewDesc;
typedef struct DvzView DvzView;
typedef struct DvzArcball DvzArcball;
typedef struct DvzArcballDesc DvzArcballDesc;
typedef struct DvzDrp2Runtime DvzDrp2Runtime;
typedef struct DvzFly DvzFly;
typedef struct DvzFlyDesc DvzFlyDesc;
typedef struct DvzGpuCtx DvzGpuCtx;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzPanzoomDesc DvzPanzoomDesc;
typedef struct DvzOrbitCamera DvzOrbitCamera;
typedef struct DvzOrbitCameraDesc DvzOrbitCameraDesc;
typedef struct DvzTurntable DvzTurntable;
typedef struct DvzTurntableDesc DvzTurntableDesc;
typedef struct DvzWindowHost DvzWindowHost;
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

typedef void (*DvzViewFrameCallback)(DvzView* view, void* user_data);
typedef void (*DvzViewRequestFrameCallback)(DvzView* view, void* user_data);
typedef void (*DvzViewPostCallback)(DvzView* view, void* user_data);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzAppScheduleMode
{
    DVZ_APP_SCHEDULE_ON_DEMAND,
    DVZ_APP_SCHEDULE_CONTINUOUS,
} DvzAppScheduleMode;


typedef enum DvzAppCaptureFlags
{
    DVZ_APP_CAPTURE_NONE = 0,
    DVZ_APP_CAPTURE_DVZR = 1 << 0,
    DVZ_APP_CAPTURE_VIDEO = 1 << 1,
    DVZ_APP_CAPTURE_PNG = 1 << 2,
} DvzAppCaptureFlags;


typedef enum DvzViewKind
{
    DVZ_VIEW_OFFSCREEN,
    DVZ_VIEW_GLFW,
    DVZ_VIEW_EXTERNAL_SURFACE,
} DvzViewKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAppConfig
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t instance_extension_count;
    const char* const* instance_extensions;
    bool enable_canvas_extensions;
    bool enable_glfw_extensions;
    DvzAppScheduleMode schedule_mode;
    double fps_cap;
    DvzFontDefaults font_defaults;
};


struct DvzAppCaptureConfig
{
    uint32_t struct_size;
    uint32_t flags;
    const char* directory;
    const char* basename;
    double fps;
    const char* video_backend;
    DvzVideoCaptureMode video_capture_mode;
};


struct DvzAppResources
{
    uint32_t struct_size;
    uint32_t flags;

    /* Optional borrowed GPU context.  The app creates and owns one when NULL. */
    DvzGpuCtx* gpu_ctx;

    /* Optional borrowed DRP2 runtime compatible with gpu_ctx.  The app creates one when NULL. */
    DvzDrp2Runtime* runtime;

    /* Optional borrowed window host.  The app creates and owns one when NULL. */
    DvzWindowHost* window_host;
};


struct DvzViewDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzViewKind kind;

    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;

    float device_scale;
    float user_scale;
    float render_scale;

    const char* title;
    const DvzWindowExternalSurfaceInfo* external_surface;
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
 * Return an empty app resource bundle.
 *
 * @return the empty app resources bundle
 */
DVZ_EXPORT DvzAppResources dvz_app_resources(void);


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
 * Create an app bound to a scene with optional caller-provided host resources.
 *
 * NULL resources make the app allocate the same stack as dvz_app_with_config().  Individual NULL
 * fields are created and owned by the app.  Non-NULL fields are borrowed exclusively for the app
 * lifetime and must outlive the app.  When a runtime is provided, a GPU context must also be
 * provided, and the runtime must have been created from that GPU context's device and allocator.
 * App config instance-extension fields only affect app-created GPU contexts.
 *
 * @param scene the scene (borrowed — must outlive the app)
 * @param config optional app configuration, or NULL for dvz_app_config()
 * @param resources optional borrowed resource bundle
 * @return the app, or NULL on failure
 */
DVZ_EXPORT DvzApp* dvz_app_with_resources(
    DvzScene* scene, const DvzAppConfig* config, const DvzAppResources* resources);


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
/*  View management                                                                            */
/*************************************************************************************************/

/**
 * Return the default descriptor for one view kind.
 *
 * @param kind view kind
 * @return initialized view descriptor
 */
DVZ_EXPORT DvzViewDesc dvz_view_desc(DvzViewKind kind);


/**
 * Create a view for a figure from an explicit descriptor.
 *
 * Descriptor dimensions distinguish logical pixels from framebuffer pixels.  For offscreen views,
 * setting only framebuffer_width/framebuffer_height preserves exact-pixel output; setting
 * logical_width/logical_height plus device_scale derives the framebuffer size.  render_scale is
 * tracked separately for future supersampling and does not currently change the framebuffer size.
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param desc view descriptor
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_view(DvzApp* app, DvzFigure* figure, const DvzViewDesc* desc);

/**
 * Create an offscreen view for a figure.
 *
 * The figure's panels and visuals are rendered into an offscreen framebuffer of the given size
 * on every call to dvz_app_run().
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param width framebuffer width in pixels
 * @param height framebuffer height in pixels
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView*
dvz_view_offscreen(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height);


/**
 * Create an interactive GLFW view for a figure.
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
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_view_glfw(
    DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height, const char* title);


/**
 * Create a hosted present view around an externally-owned Vulkan surface.
 *
 * The caller owns the native event loop and must create the Vulkan surface using the instance
 * extensions passed to dvz_app_with_config().  Datoviz owns only the rendering objects built on
 * top of the supplied surface unless surface->owned_by_datoviz is true.
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param surface external Vulkan surface description
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_view_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Create a hosted present view around an external Vulkan surface from FFI-friendly handles.
 *
 * Foreign-function-interface adapters may use this helper when constructing
 * DvzWindowExternalSurfaceInfo directly is undesirable. Native C and C++ callers should prefer
 * dvz_view_external_surface().
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param instance borrowed VkInstance handle as an opaque pointer
 * @param surface borrowed or Datoviz-owned VkSurfaceKHR handle value
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_view_external_surface_ffi(
    DvzApp* app, DvzFigure* figure, void* instance, uint64_t surface,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float scale_x, float scale_y,
    bool owned_by_datoviz);


/**
 * Update the hosted external surface associated with a view.
 *
 * Use this when the host toolkit recreates or resizes its native surface.  A NULL surface handle is
 * accepted to mark the surface temporarily unavailable; rendering then returns
 * DVZ_CANVAS_FRAME_WAIT_SURFACE until a valid surface is supplied again.
 *
 * @param view view created with dvz_view_external_surface()
 * @param surface external Vulkan surface description
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_update_external_surface(
    DvzView* view, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Update a hosted external surface from FFI-friendly handles.
 *
 * Foreign-function-interface adapters may use this helper when constructing
 * DvzWindowExternalSurfaceInfo directly is undesirable. Native C and C++ callers should prefer
 * dvz_view_update_external_surface().
 *
 * @param view view created with dvz_view_external_surface() or dvz_view_external_surface_ffi()
 * @param instance borrowed VkInstance handle as an opaque pointer, or NULL for surface loss
 * @param surface borrowed or Datoviz-owned VkSurfaceKHR handle value, or zero for surface loss
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_update_external_surface_ffi(
    DvzView* view, void* instance, uint64_t surface, uint32_t framebuffer_width,
    uint32_t framebuffer_height, float scale_x, float scale_y, bool owned_by_datoviz);


/**
 * Release a hosted external surface before the host destroys it.
 *
 * Clears the request-frame callback, marks the surface temporarily unavailable, and runs one
 * render-once step so the present swapchain observes the unavailable surface and releases borrowed
 * surface-dependent objects. The host remains responsible for destroying the VkSurfaceKHR.
 *
 * @param view view created with dvz_view_external_surface()
 * @return DVZ_CANVAS_FRAME_WAIT_SURFACE on clean release, or a negative error code
 */
DVZ_EXPORT int dvz_view_release_external_surface(DvzView* view);


/**
 * Emit a hosted resize event for a view.
 *
 * External UI adapters call this after host resize notifications so Datoviz controllers and figure
 * sizing see the host's logical and framebuffer dimensions.
 *
 * @param view the view
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_emit_resize(
    DvzView* view, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y);


/**
 * Emit a hosted pointer position/button event for a view.
 *
 * @param view the view
 * @param type pointer event type
 * @param x pointer x position in host-window coordinates
 * @param y pointer y position in host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param button pointer button, or DVZ_POINTER_BUTTON_NONE
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_emit_pointer(
    DvzView* view, DvzPointerEventType type, float x, float y, float window_width,
    float window_height, DvzPointerButton button, int mods);


/**
 * Emit a hosted pointer wheel event for a view.
 *
 * @param view the view
 * @param x pointer x position in host-window coordinates
 * @param y pointer y position in host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param dx horizontal wheel delta
 * @param dy vertical wheel delta
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_emit_wheel(
    DvzView* view, float x, float y, float window_width, float window_height, float dx,
    float dy, int mods);


/**
 * Emit a hosted keyboard event for a view.
 *
 * @param view the view
 * @param type keyboard event type
 * @param key Datoviz key code
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int
dvz_view_emit_key(DvzView* view, DvzKeyboardEventType type, DvzKeyCode key, int mods);


/**
 * Return the underlying DvzCanvas for a window.
 *
 * Gives access to the full canvas API (capture, video sink, live-image sink, etc.).
 *
 * @param view the view
 * @return the canvas, or NULL if the window was not created with GPU support
 */
DVZ_EXPORT struct DvzCanvas* dvz_view_canvas(DvzView* view);


/**
 * Return the input router for a view.
 *
 * Pass the returned router to dvz_panel_connect_input() to route panel-local input through
 * scene-owned controller bindings. Returns NULL when GPU support is absent.
 *
 * @param view the view
 * @return the input router, or NULL
 */
DVZ_EXPORT struct DvzInputRouter* dvz_view_input(DvzView* view);


/**
 * Return the current device scale for a view.
 *
 * @param view the view
 * @return physical pixels per logical pixel, or 1 when unavailable
 */
DVZ_EXPORT float dvz_view_device_scale(const DvzView* view);


/**
 * Return the current logical view size.
 *
 * @param view the view
 * @param out_width output logical width in pixels, may be NULL
 * @param out_height output logical height in pixels, may be NULL
 */
DVZ_EXPORT void
dvz_view_logical_size(const DvzView* view, uint32_t* out_width, uint32_t* out_height);


/**
 * Return the current framebuffer view size.
 *
 * @param view the view
 * @param out_width output framebuffer width in physical pixels, may be NULL
 * @param out_height output framebuffer height in physical pixels, may be NULL
 */
DVZ_EXPORT void
dvz_view_framebuffer_size(const DvzView* view, uint32_t* out_width, uint32_t* out_height);


/**
 * Return the current render scale.
 *
 * @param view the view
 * @return render scale, defaulting to 1
 */
DVZ_EXPORT float dvz_view_render_scale(const DvzView* view);


/**
 * Return the current user scale for UI-like scene quantities.
 *
 * @param view the view
 * @return user scale, defaulting to 1
 */
DVZ_EXPORT float dvz_view_user_scale(const DvzView* view);


/**
 * Set the user scale for UI-like scene quantities.
 *
 * @param view the view
 * @param scale positive user scale
 */
DVZ_EXPORT void dvz_view_set_user_scale(DvzView* view, float scale);


/**
 * Connect a panel's bound controllers to a view input router.
 *
 * @param view the view
 * @param panel the panel
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_view_connect_panel(DvzView* view, DvzPanel* panel);


/**
 * Bind a controller to a panel and connect the panel to a view input router.
 *
 * @param view the view
 * @param panel the panel
 * @param controller the scene-owned controller
 * @param dims dimension mask
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_view_bind_controller(
    DvzView* view, DvzPanel* panel, DvzController* controller, DvzDimMask dims);


/**
 * Create, bind, and connect a panzoom controller for one panel.
 *
 * @param view the view
 * @param panel the panel
 * @param desc panzoom descriptor, or NULL for defaults
 * @return the panzoom payload, or NULL on validation error
 */
DVZ_EXPORT DvzPanzoom*
dvz_view_panzoom(DvzView* view, DvzPanel* panel, const DvzPanzoomDesc* desc);


/**
 * Create, bind, and connect an arcball controller for one panel.
 *
 * @param view the view
 * @param panel the panel
 * @param desc arcball descriptor, or NULL for defaults
 * @return the arcball payload, or NULL on validation error
 */
DVZ_EXPORT DvzArcball*
dvz_view_arcball(DvzView* view, DvzPanel* panel, const DvzArcballDesc* desc);


/**
 * Create, bind, and connect an orbit-camera controller for one panel.
 *
 * @param view the view
 * @param panel the panel
 * @param desc orbit-camera descriptor, or NULL for defaults
 * @return the orbit-camera payload, or NULL on validation error
 */
DVZ_EXPORT DvzOrbitCamera* dvz_view_orbit_camera(
    DvzView* view, DvzPanel* panel, const DvzOrbitCameraDesc* desc);


/**
 * Create, bind, and connect a fly controller for one panel.
 *
 * @param view the view
 * @param panel the panel
 * @param desc fly descriptor, or NULL for defaults
 * @return the fly payload, or NULL on validation error
 */
DVZ_EXPORT DvzFly*
dvz_view_fly(DvzView* view, DvzPanel* panel, const DvzFlyDesc* desc);


/**
 * Create, bind, and connect a turntable controller for one panel.
 *
 * @param view the view
 * @param panel the panel
 * @param desc turntable descriptor, or NULL for defaults
 * @return the turntable payload, or NULL on validation error
 */
DVZ_EXPORT DvzTurntable* dvz_view_turntable(
    DvzView* view, DvzPanel* panel, const DvzTurntableDesc* desc);


/**
 * Capture the last rendered frame and write it to a PNG file.
 *
 * Convenience wrapper around dvz_view_canvas() + dvz_canvas_capture_png().
 * Call after at least one dvz_app_run() iteration.
 *
 * @param view the view
 * @param path output file path
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_capture_png(DvzView* view, const char* path);


/**
 * Return the default app capture configuration.
 *
 * The returned config enables no capture by default. When enabled, outputs are written in the
 * current directory with basename "capture" at 60 FPS using the automatic video backend.
 *
 * @return the default capture configuration
 */
DVZ_EXPORT DvzAppCaptureConfig dvz_app_capture_config(void);


/**
 * Return an app capture configuration derived from environment variables.
 *
 * `DVZ_CAPTURE` accepts comma-, semicolon-, plus-, colon-, pipe-, or space-separated tokens:
 * `dvzr`, `mp4`/`video`, `png`, `all`, or false-like values (`0`, `false`, `off`, `none`).
 * Optional overrides are `DVZ_CAPTURE_DIR`, `DVZ_CAPTURE_BASENAME`, `DVZ_CAPTURE_FPS`,
 * `DVZ_CAPTURE_VIDEO_BACKEND`, and `DVZ_CAPTURE_VIDEO_MODE` (`auto`, `external`, `cpu`).
 *
 * @param basename fallback output basename, or NULL for "capture"
 * @return the environment-derived capture configuration
 */
DVZ_EXPORT DvzAppCaptureConfig dvz_app_capture_config_from_env(const char* basename);


/**
 * Start configured captures for a view.
 *
 * DVZR capture records emitted app DRP2 frame streams, video capture enables the canvas video
 * sink, and PNG capture is written when dvz_view_capture_stop() is called after rendering.
 *
 * @param view the view
 * @param config capture configuration, or NULL for dvz_app_capture_config()
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_capture_start(
    DvzView* view, const DvzAppCaptureConfig* config);


/**
 * Start captures for a view from environment variables.
 *
 * This is a convenience wrapper around dvz_app_capture_config_from_env() and
 * dvz_view_capture_start(). If `DVZ_CAPTURE` is unset or disables capture, the function is a
 * no-op and returns success.
 *
 * @param view the view
 * @param basename fallback output basename, or NULL for "capture"
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_capture_from_env(DvzView* view, const char* basename);


/**
 * Stop active captures started by dvz_view_capture_start().
 *
 * Video capture is disabled, DVZR recordings are closed, and pending PNG capture is written from
 * the last rendered frame.
 *
 * @param view the view
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_capture_stop(DvzView* view);


/**
 * Start recording emitted scene DRP2 frame streams for a view.
 *
 * The recording is a `.dvzr` directory written by the DRP2 linear recorder. Frames are appended
 * from the app draw path after successful runtime execution. Call
 * dvz_view_record_stop() to close the recording before replaying it.
 *
 * @param view the view
 * @param path output recording directory path
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_record_start(DvzView* view, const char* path);


/**
 * Stop recording emitted scene DRP2 frame streams for a view.
 *
 * @param view the view
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_record_stop(DvzView* view);


/**
 * Start live replay of a DRP2 recording in a view.
 *
 * The replay path executes recorded DRP2 frame streams directly. App recordings render into the
 * current view frame by attaching that borrowed frame under the recording's target id.
 *
 * @param view the view
 * @param path input `.dvzr` recording directory
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_replay_start(DvzView* view, const char* path);


/**
 * Stop live replay and release the loaded recording.
 *
 * @param view the view
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_replay_stop(DvzView* view);


/**
 * Enable or disable timestamp-paced replay.
 *
 * @param view the view
 * @param paced whether replay waits for recorded timestamps
 */
DVZ_EXPORT void dvz_view_replay_set_paced(DvzView* view, bool paced);


/**
 * Set replay speed multiplier.
 *
 * Values below or equal to zero are ignored. A value of 2 plays twice as fast.
 *
 * @param view the view
 * @param speed replay speed multiplier
 */
DVZ_EXPORT void dvz_view_replay_set_speed(DvzView* view, double speed);


/**
 * Enable or disable replay looping.
 *
 * Looping resets the app DRP2 runtime before replay starts from frame zero again.
 *
 * @param view the view
 * @param loop whether the recording should loop
 */
DVZ_EXPORT void dvz_view_replay_set_loop(DvzView* view, bool loop);


/**
 * Return the number of frames in the active replay recording.
 *
 * @param view the view
 * @return replay frame count, or 0 when no replay is active
 */
DVZ_EXPORT uint32_t dvz_view_replay_frame_count(const DvzView* view);


/**
 * Resize a view's logical and framebuffer extent.
 *
 * Intended for offscreen or externally-hosted windows whose size is controlled by another UI
 * toolkit. GLFW windows should normally be resized by the platform window itself.
 *
 * @param view the view
 * @param width width in pixels
 * @param height height in pixels
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_resize(DvzView* view, uint32_t width, uint32_t height);


/**
 * Enable or disable rendering for a view.
 *
 * Disabled windows remain owned by the app but dvz_view_render_once() skips them. This is
 * intended for hosted/offscreen integrations such as hidden dock tabs.
 *
 * @param view the view
 * @param enabled whether rendering should be enabled
 */
DVZ_EXPORT void dvz_view_set_render_enabled(DvzView* view, bool enabled);


/**
 * Return whether rendering is enabled for a view.
 *
 * @param view the view
 * @return whether rendering is enabled
 */
DVZ_EXPORT bool dvz_view_render_enabled(const DvzView* view);


/**
 * Request that the host schedules another frame for a view.
 *
 * Hosted UI adapters should map the registered callback to their native repaint primitive, for
 * example QWindow::requestUpdate(), QWidget::update(), an SDL wakeup, or a Tk idle callback.
 *
 * @param view the view
 */
DVZ_EXPORT void dvz_view_request_frame(DvzView* view);


/**
 * Wake the host scheduler for a view.
 *
 * This function is safe to call from another thread. It requests scheduler attention without
 * executing user-posted callbacks immediately; posted callbacks are drained on the view owner
 * thread by dvz_view_render_once().
 *
 * @param view the view
 */
DVZ_EXPORT void dvz_view_wake(DvzView* view);


/**
 * Post a callback to run on the view owner thread.
 *
 * The callback is queued from any thread and drained near the start of dvz_view_render_once().
 * The queue stores the callback pointer and user_data value verbatim; the caller must ensure both
 * remain valid until the callback has run.
 *
 * @param view the view
 * @param callback callback to run on the owner thread
 * @param user_data opaque pointer forwarded to the callback
 * @return 0 on success, negative on invalid input or queue overflow
 */
DVZ_EXPORT int
dvz_view_post(DvzView* view, DvzViewPostCallback callback, void* user_data);


/**
 * Register a callback invoked whenever Datoviz requests another frame.
 *
 * This is a passive scheduling signal: it does not change dvz_app_run() behavior and does not
 * render by itself. The host remains responsible for calling dvz_view_render_once().
 *
 * @param view the view
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DVZ_EXPORT void dvz_view_set_request_frame_callback(
    DvzView* view, DvzViewRequestFrameCallback callback, void* user_data);


/**
 * Register a callback invoked after each successful frame for one view.
 *
 * The callback runs after the scene frame artifact has been emitted, executed, request processing
 * has completed, and the artifact-owned stream snapshot has been destroyed. Scene mutations from
 * the callback are therefore allowed and become visible on the next frame.
 *
 * @param view the view
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DVZ_EXPORT void
dvz_view_set_frame_callback(
    DvzView* view, DvzViewFrameCallback callback, void* user_data);



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

/**
 * Render one frame for a single view without polling any Datoviz-owned event loop.
 *
 * This is the primary hosted-loop primitive for Qt, SDL, Tk, IPython, and other integrations where
 * the caller owns scheduling.  Returns the dvz_canvas_frame() status when no frame was submitted.
 *
 * @param view the view
 * @return DVZ_CANVAS_FRAME_READY after a submitted frame, DVZ_CANVAS_FRAME_WAIT_SURFACE while the
 * surface is unavailable, or a negative error code
 */
DVZ_EXPORT int dvz_view_render_once(DvzView* view);


/**
 * Render one frame for every view without polling any Datoviz-owned event loop.
 *
 * @param app the app
 * @return 0 on success, DVZ_CANVAS_FRAME_WAIT_SURFACE if any surface is unavailable, or negative on
 * error
 */
DVZ_EXPORT int dvz_app_render_once(DvzApp* app);


/**
 * Run the frame loop.
 *
 * Finite runs render the requested number of frames. Interactive runs (`frame_count == 0`) use
 * the app scheduler: on-demand mode waits for resize/input/request-frame invalidation, while
 * continuous mode renders active windows until every interactive window closes.
 *
 * @param app the app
 * @param frame_count number of frames to render (0 = interactive loop until all windows close)
 */
DVZ_EXPORT void dvz_app_run(DvzApp* app, uint32_t frame_count);


EXTERN_C_OFF
