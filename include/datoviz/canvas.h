/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/vk/vulkan.h"
#include <volk.h>

#include "canvas/enums.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/input/router.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/video.h"
#include "datoviz/window/types.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzCanvas DvzCanvas;
typedef struct DvzWindow DvzWindow;
typedef struct DvzDevice DvzDevice;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CANVAS_DEFAULT_TIMING_HISTORY 120



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// Canvas configuration.
typedef struct
{
    uint32_t struct_size;
    uint32_t flags;
    DvzWindow* window;
    DvzDevice* device;
    DvzCanvasRenderMode render_mode;
    VkFormat color_format;
    VkPresentModeKHR present_mode;
    bool enable_video_sink;
    size_t timing_history;
} DvzCanvasConfig;



// Timing metadata associated with submitted frames.
typedef struct
{
    uint64_t frame_id;
    double cpu_submit_us;
    double gpu_complete_us;
    double present_start_us;
    double present_done_us;
} DvzFrameTiming;



// Live-image frame payload forwarded by the optional canvas live sink.
//
// All Vulkan handles are borrowed from the canvas stream and are valid only for the duration of the
// live-image callback unless a later callback with the same resource_generation explicitly confirms
// unchanged handles. The callback must not destroy, reset, transition, or retain these handles.
// `command_buffer` is already recorded for the submitted frame. `image` is in the layout reported
// by the matching stream frame and should be treated as read-only by consumers. File descriptors are
// owned by Datoviz for the callback; duplicate them in the callback if they must outlive it, and do
// not close the original descriptors.
typedef struct
{
    uint64_t frame_id;
    uint64_t wait_value;
    VkFormat color_format;
    VkImage image;
    VkImageView image_view;
    VkCommandBuffer command_buffer;
    VkExtent2D extent;
    bool handles_dirty;
    uint64_t resource_generation;
    bool image_valid;
    int memory_fd;
    int wait_semaphore_fd;
} DvzCanvasLiveImageFrame;



typedef int (*DvzCanvasLiveImageCallback)(
    const DvzCanvasLiveImageFrame* frame, void* user_data);



// Configuration for the optional live-image sink.
typedef struct
{
    uint32_t struct_size;
    uint32_t flags;
    DvzCanvasLiveImageCallback callback;
    void* user_data;
} DvzCanvasLiveImageSinkConfig;



typedef void (*DvzCanvasDraw)(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data);



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default canvas configuration used when callers do not override fields.
 *
 * The caller must set the borrowed `window` and `device` fields before creating a canvas. The
 * returned configuration selects present rendering, FIFO presentation, the runtime default color
 * format, and `DVZ_CANVAS_DEFAULT_TIMING_HISTORY` timing samples.
 *
 * @returns the initialized configuration
 */
DVZ_EXPORT DvzCanvasConfig dvz_canvas_config(void);


/**
 * Return a default live-image sink configuration.
 *
 * @returns a configuration with no callback or user data
 */
DVZ_EXPORT DvzCanvasLiveImageSinkConfig dvz_canvas_live_image_sink_config(void);



/**
 * Allocate a new canvas tied to a window surface and device.
 *
 * `cfg->window` and `cfg->device` are required borrowed objects and must outlive the canvas. The
 * canvas copies the configuration and owns the stream, allocator, synchronization, and render
 * resources it creates; it does not destroy the window or device.
 *
 * @param cfg required canvas configuration with non-NULL `window` and `device`
 * @returns a newly allocated canvas, or NULL when the configuration or runtime setup is invalid
 */
DVZ_EXPORT DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg);



/**
 * Destroy the canvas and any stream resources it owns.
 *
 * @param canvas canvas returned by dvz_canvas_create(), or NULL
 */
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);



/**
 * Register a draw callback executed before each successful dvz_canvas_frame() returns.
 *
 * The callback and `user_data` are borrowed and must remain valid until replaced, cleared, or the
 * canvas is destroyed. The callback receives a borrowed frame whose Vulkan handles are valid only
 * for that invocation and must not be destroyed, reset, submitted, transitioned, or retained.
 *
 * @param canvas target canvas
 * @param callback draw callback (NULL removes the callback)
 * @param user_data borrowed opaque pointer supplied to the callback on every invocation
 */
DVZ_EXPORT void
dvz_canvas_set_draw_callback(DvzCanvas* canvas, DvzCanvasDraw callback, void* user_data);



/**
 * Acquire a new frame, refresh swapchain-backed metadata, and run the draw callback.
 *
 * @param canvas canvas handle
 * @returns DVZ_CANVAS_FRAME_READY when a frame is ready, DVZ_CANVAS_FRAME_WAIT_SURFACE when the
 * surface is unavailable, or a negative error code when acquisition fails
 */
DVZ_EXPORT int dvz_canvas_frame(DvzCanvas* canvas);



/**
 * Submit the current frame to the internal stream and attached sinks.
 *
 * @param canvas canvas handle
 * @returns 0 when submission succeeds, <0 when the stream submission fails
 */
DVZ_EXPORT int dvz_canvas_submit(DvzCanvas* canvas);



/**
 * Return the configured render mode for a canvas.
 *
 * @param canvas canvas handle
 * @returns render mode currently used by the canvas
 */
DVZ_EXPORT DvzCanvasRenderMode dvz_canvas_render_mode(const DvzCanvas* canvas);



/**
 * Return the present-mode runtime state for diagnostics.
 *
 * @param canvas canvas handle
 * @returns present runtime state, or UNINITIALIZED when unavailable
 */
DVZ_EXPORT DvzCanvasPresentRuntimeState dvz_canvas_present_runtime_state(const DvzCanvas* canvas);



/**
 * Return the offscreen-mode runtime state for diagnostics.
 *
 * @param canvas canvas handle
 * @returns offscreen runtime state, or UNINITIALIZED when unavailable
 */
DVZ_EXPORT DvzCanvasOffscreenRuntimeState
dvz_canvas_offscreen_runtime_state(const DvzCanvas* canvas);



/**
 * Expose the input router owned by the canvas window.
 *
 * @param canvas canvas owning the router
 * @returns the borrowed router, valid until the canvas window is destroyed, or NULL when absent
 */
DVZ_EXPORT DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas);



/**
 * Capture the latest presented canvas frame into a caller-provided RGBA buffer.
 *
 * The returned pixels are screenshot/export pixels: tightly packed RGBA8 with sRGB-encoded RGB
 * channels and straight linear alpha. Use a dedicated query/readback API for scientific values that
 * must remain in linear float or data space.
 *
 * @param canvas canvas handle
 * @param width expected frame width in pixels
 * @param height expected frame height in pixels
 * @param out_rgba destination buffer receiving sRGB RGBA8 pixels
 * @param out_size_bytes size of `out_rgba` in bytes
 * @returns 0 on success or a negative error code
 */
DVZ_EXPORT int dvz_canvas_capture_rgba_into(
    DvzCanvas* canvas, uint32_t width, uint32_t height, uint8_t* out_rgba,
    DvzSize out_size_bytes);



/**
 * Capture the latest presented canvas frame into a newly allocated RGBA buffer.
 *
 * The returned pixels are screenshot/export pixels: tightly packed RGBA8 with sRGB-encoded RGB
 * channels and straight linear alpha. Use a dedicated query/readback API for scientific values that
 * must remain in linear float or data space.
 *
 * @param canvas canvas handle
 * @param out_width destination width in pixels
 * @param out_height destination height in pixels
 * @param out_rgba destination pointer receiving an allocated sRGB RGBA8 buffer
 * @returns 0 on success or a negative error code
 * @note caller owns `*out_rgba` and must release it with `dvz_memory_free()`
 */
DVZ_EXPORT int
dvz_canvas_capture_rgba(
    DvzCanvas* canvas, uint32_t* out_width, uint32_t* out_height, uint8_t** out_rgba);



/**
 * Capture the latest presented canvas frame and write it to a PNG file.
 *
 * PNG capture uses the same screenshot/export contract as dvz_canvas_capture_rgba(): sRGB-encoded
 * RGBA8 color with straight linear alpha.
 *
 * @param canvas canvas handle
 * @param path output file path
 * @returns 0 on success or a negative error code
 */
DVZ_EXPORT int dvz_canvas_capture_png(DvzCanvas* canvas, const char* path);



/**
 * Enable or disable the video sink attached to the canvas stream.
 *
 * @param canvas canvas owning the stream
 * @param enable true to enable, false to detach an existing sink
 * @param cfg optional configuration passed to the sink (NULL uses defaults)
 * @note toggling this option rebuilds the internal stream so sinks restart on the next frame
 * @returns 0 on success or a negative sink error
 */
DVZ_EXPORT int
dvz_canvas_configure_video_sink(DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg);



/**
 * Enable or disable the live-image sink attached to the canvas stream.
 *
 * @param canvas canvas owning the stream
 * @param enable true to enable, false to detach the sink
 * @param cfg required configuration when enabling, ignored when disabling
 * @note toggling this option rebuilds the internal stream so sinks restart on the next frame
 * @returns 0 on success or a negative sink error
 */
DVZ_EXPORT int dvz_canvas_configure_live_image_sink(
    DvzCanvas* canvas, bool enable, const DvzCanvasLiveImageSinkConfig* cfg);



/**
 * Access the stream underpinning the canvas.
 *
 * The caller must not destroy the returned stream. It remains valid until the canvas is destroyed
 * or reconfiguring a sink rebuilds the canvas stream.
 *
 * @param canvas canvas handle
 * @returns the borrowed underlying stream, or NULL when unavailable
 */
DVZ_EXPORT DvzStream* dvz_canvas_stream(DvzCanvas* canvas);



/**
 * Read the recorded frame timings.
 *
 * The returned view is owned by the canvas and may be invalidated or overwritten by subsequent
 * frame submissions. It must not be freed and remains valid at most until canvas destruction.
 *
 * @param canvas canvas handle
 * @param count optional output receiving the number of readable samples
 * @returns the borrowed internal timing buffer, or NULL when no samples are available
 */
DVZ_EXPORT const DvzFrameTiming* dvz_canvas_timings(const DvzCanvas* canvas, size_t* count);



EXTERN_C_OFF
