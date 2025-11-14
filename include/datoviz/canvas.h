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

#include <volk.h>

#include "canvas/enums.h"
#include "datoviz/common/macros.h"
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
    DvzWindow* window;
    DvzDevice* device;
    VkFormat color_format;
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



typedef void (*DvzCanvasDraw)(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data);



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default canvas configuration used when callers do not override fields.
 *
 * @returns a configuration with null handles, RGBA8 color format, and empty timing history
 */
DVZ_EXPORT DvzCanvasConfig dvz_canvas_default_config(void);



/**
 * Allocate a new canvas tied to a window surface and device.
 *
 * @param cfg canvas configuration or NULL for defaults
 * @returns pointer to the created canvas or NULL on failure
 */
DVZ_EXPORT DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg);



/**
 * Destroy the canvas and any stream resources it owns.
 *
 * @param canvas canvas handle returned by dvz_canvas_create()
 */
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);



/**
 * Register a draw callback executed whenever dvz_canvas_frame() succeeds.
 *
 * @param canvas target canvas
 * @param callback draw callback (NULL removes the callback)
 * @param user_data opaque pointer supplied to the callback on every invocation
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
 * Expose the input router owned by the canvas window.
 *
 * @param canvas canvas owning the router
 * @returns pointer to the router or NULL when the canvas/window is invalid
 */
DVZ_EXPORT DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas);



/**
 * Enable or disable the video sink attached to the canvas stream.
 *
 * @param canvas canvas owning the stream
 * @param enable true to enable, false to detach an existing sink
 * @param cfg optional configuration passed to the sink (NULL uses defaults)
 * @returns 0 on success or a negative sink error
 */
DVZ_EXPORT int
dvz_canvas_configure_video_sink(DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg);



/**
 * Access the stream underpinning the canvas.
 *
 * @param canvas canvas handle
 * @returns stream pointer or NULL when the canvas is invalid
 */
DVZ_EXPORT DvzStream* dvz_canvas_stream(DvzCanvas* canvas);



/**
 * Read the recorded frame timings.
 *
 * @param canvas canvas handle
 * @param count optional output storing the number of samples tracked
 * @returns pointer to the internal ring buffer with the latest timings
 */
DVZ_EXPORT const DvzFrameTiming* dvz_canvas_timings(const DvzCanvas* canvas, size_t* count);



EXTERN_C_OFF
