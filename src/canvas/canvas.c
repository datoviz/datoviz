/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas public API                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzStreamConfig canvas_stream_config(const DvzCanvas* canvas)
{
    ANN(canvas);
    DvzStreamConfig cfg = dvz_stream_default_config();
    if (canvas->surface)
    {
        cfg.width = canvas->surface->extent.width;
        cfg.height = canvas->surface->extent.height;
        cfg.color_format = (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
                               ? canvas->cfg.color_format
                               : canvas->surface->format;
    }
    else if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        cfg.color_format = canvas->cfg.color_format;
    }
    return cfg;
}



/*************************************************************************************************/
/*  Frame pool                                                                                   */
/*************************************************************************************************/

void dvz_canvas_frame_pool_init(DvzCanvasFramePool* pool, uint32_t frame_count)
{
    ANN(pool);
    dvz_canvas_frame_pool_release(pool);
    pool->frame_count = frame_count > 0 ? frame_count : 1;
    pool->frames = (DvzStreamFrame*)calloc(pool->frame_count, sizeof(DvzStreamFrame));
    ANN(pool->frames);
    pool->current_index = 0;
}



void dvz_canvas_frame_pool_release(DvzCanvasFramePool* pool)
{
    if (!pool)
    {
        return;
    }
    if (pool->frames)
    {
        free(pool->frames);
        pool->frames = NULL;
    }
    pool->frame_count = 0;
    pool->current_index = 0;
}



DvzStreamFrame* dvz_canvas_frame_pool_current(DvzCanvasFramePool* pool)
{
    ANN(pool);
    if (!pool->frames || pool->frame_count == 0)
    {
        return NULL;
    }
    return &pool->frames[pool->current_index];
}



DvzStreamFrame* dvz_canvas_frame_pool_rotate(DvzCanvasFramePool* pool)
{
    ANN(pool);
    if (!pool->frames || pool->frame_count == 0)
    {
        return NULL;
    }
    pool->current_index = (pool->current_index + 1) % pool->frame_count;
    return &pool->frames[pool->current_index];
}



/*************************************************************************************************/
/*  Timings                                                                                      */
/*************************************************************************************************/

void dvz_canvas_timings_init(DvzCanvasTimingState* timings, size_t capacity)
{
    ANN(timings);
    dvz_canvas_timings_release(timings);
    timings->capacity = capacity;
    timings->count = 0;
    timings->head = 0;
    if (capacity > 0)
    {
        timings->samples = (DvzFrameTiming*)calloc(capacity, sizeof(DvzFrameTiming));
        ANN(timings->samples);
    }
}



void dvz_canvas_timings_release(DvzCanvasTimingState* timings)
{
    if (!timings)
    {
        return;
    }
    if (timings->samples)
    {
        free(timings->samples);
        timings->samples = NULL;
    }
    timings->capacity = 0;
    timings->count = 0;
    timings->head = 0;
}



void dvz_canvas_timings_record(
    DvzCanvasTimingState* timings, uint64_t frame_id, double cpu_submit_us)
{
    ANN(timings);
    if (!timings->samples || timings->capacity == 0)
    {
        return;
    }
    DvzFrameTiming* timing = &timings->samples[timings->head];
    *timing = (DvzFrameTiming){
        .frame_id = frame_id,
        .cpu_submit_us = cpu_submit_us,
        .gpu_complete_us = 0.0,
        .present_start_us = 0.0,
        .present_done_us = 0.0,
    };
    timings->head = (timings->head + 1) % timings->capacity;
    if (timings->count < timings->capacity)
    {
        timings->count++;
    }
}



const DvzFrameTiming* dvz_canvas_timings_view(const DvzCanvasTimingState* timings, size_t* count)
{
    ANN(timings);
    if (count)
    {
        *count = timings->count;
    }
    return timings->samples;
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

/**
 * Return the default canvas configuration.
 *
 * @returns a configuration initialized with sensible defaults
 */
DvzCanvasConfig dvz_canvas_default_config(void)
{
    DvzCanvasConfig cfg = {
        .window = NULL,
        .device = NULL,
        .color_format = VK_FORMAT_B8G8R8A8_UNORM,
        .enable_video_sink = false,
        .timing_history = DVZ_CANVAS_DEFAULT_TIMING_HISTORY,
    };
    return cfg;
}



/**
 * Create a canvas instance associated with the provided window/device pair.
 *
 * @param cfg configuration describing the canvas requirements
 */
DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg)
{
    DvzCanvasConfig resolved = cfg ? *cfg : dvz_canvas_default_config();
    if (!resolved.window)
    {
        log_error("canvas creation requires a valid window handle");
        return NULL;
    }

    DvzCanvas* canvas = (DvzCanvas*)calloc(1, sizeof(DvzCanvas));
    ANN(canvas);
    canvas->cfg = resolved;
    canvas->window = resolved.window;
    canvas->device = resolved.device;
    canvas->draw_callback = NULL;
    canvas->draw_user_data = NULL;
    canvas->frame_id = 0;
    canvas->video_sink_enabled = false;
    canvas->stream_started = false;
    canvas->swapchain_sink_attached = false;

    dvz_canvas_window_surface_refresh(canvas);
    DvzStreamConfig stream_cfg = canvas_stream_config(canvas);
    canvas->stream = dvz_stream_create(canvas->device, &stream_cfg);
    if (!canvas->stream)
    {
        log_error("failed to allocate stream for canvas");
        free(canvas);
        return NULL;
    }

    dvz_canvas_frame_pool_init(&canvas->frame_pool, 1);
    size_t timing_history = resolved.timing_history > 0 ? resolved.timing_history
                                                        : DVZ_CANVAS_DEFAULT_TIMING_HISTORY;
    dvz_canvas_timings_init(&canvas->timings, timing_history);

    if (dvz_canvas_stream_prepare(canvas) != 0)
    {
        log_warn("canvas stream preparation failed; swapchain sink unavailable");
    }

    if (resolved.enable_video_sink)
    {
        dvz_canvas_configure_video_sink(canvas, true, NULL);
    }
    return canvas;
}



/**
 * Destroy a canvas and its owned resources.
 *
 * @param canvas canvas returned by dvz_canvas_create()
 */
void dvz_canvas_destroy(DvzCanvas* canvas)
{
    if (!canvas)
    {
        return;
    }
    dvz_canvas_stream_enable_video(canvas, false, NULL);
    if (canvas->stream)
    {
        dvz_stream_destroy(canvas->stream);
        canvas->stream = NULL;
    }
    dvz_canvas_frame_pool_release(&canvas->frame_pool);
    dvz_canvas_timings_release(&canvas->timings);
    free(canvas);
}



/**
 * Register a draw callback invoked during dvz_canvas_frame().
 *
 * @param canvas canvas whose callback should be updated
 * @param callback draw callback pointer
 * @param user_data user data forwarded to the callback
 */
void
dvz_canvas_set_draw_callback(DvzCanvas* canvas, DvzCanvasDraw callback, void* user_data)
{
    ANN(canvas);
    canvas->draw_callback = callback;
    canvas->draw_user_data = user_data;
}



/**
 * Acquire the next frame and execute the registered draw callback.
 *
 * @param canvas canvas to update
 * @returns 0 when the frame is ready or -1 on error
 */
int dvz_canvas_frame(DvzCanvas* canvas)
{
    ANN(canvas);
    dvz_canvas_window_surface_refresh(canvas);
    if (dvz_canvas_stream_prepare(canvas) != 0)
    {
        return -1;
    }

    DvzStreamFrame* frame = dvz_canvas_frame_pool_rotate(&canvas->frame_pool);
    if (!frame)
    {
        log_error("canvas frame pool unavailable");
        return -1;
    }

    if (dvz_canvas_stream_start(canvas, frame) != 0)
    {
        return -1;
    }

    if (canvas->draw_callback)
    {
        canvas->draw_callback(canvas, frame, canvas->draw_user_data);
    }
    canvas->frame_id++;
    return 0;
}



/**
 * Submit the current frame to the canvas stream.
 *
 * @param canvas canvas to submit
 * @returns 0 on success, negative error otherwise
 */
int dvz_canvas_submit(DvzCanvas* canvas)
{
    ANN(canvas);
    DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
    if (!frame)
    {
        log_error("no canvas frame available for submission");
        return -1;
    }

    DvzClock clock = dvz_clock();
    dvz_clock_tick(&clock);
    int result = dvz_canvas_stream_submit(canvas, canvas->frame_id);
    double elapsed = dvz_clock_interval(&clock) * 1e6;
    dvz_canvas_timings_record(&canvas->timings, canvas->frame_id, elapsed);
    return result;
}



/**
 * Return the input router tied to the canvas window.
 *
 * @param canvas canvas owning the router
 */
DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas)
{
    ANN(canvas);
    if (!canvas->window)
    {
        return NULL;
    }
    return dvz_window_router(canvas->window);
}



/**
 * Enable or disable the video sink on the canvas stream.
 *
 * @param canvas canvas associated with the stream
 * @param enable true to enable, false to disable
 * @param cfg optional sink configuration
 */
int dvz_canvas_configure_video_sink(
    DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);
    return dvz_canvas_stream_enable_video(canvas, enable, cfg);
}



/**
 * Return the underlying stream pointer.
 *
 * @param canvas target canvas
 */
DvzStream* dvz_canvas_stream(DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->stream;
}



/**
 * Return the recorded timings collected so far.
 *
 * @param canvas canvas handle
 * @param count optional destination for the number of samples
 */
const DvzFrameTiming* dvz_canvas_timings(const DvzCanvas* canvas, size_t* count)
{
    ANN(canvas);
    return dvz_canvas_timings_view(&canvas->timings, count);
}
