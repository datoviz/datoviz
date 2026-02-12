/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas stream helpers                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include "_assertions.h"
#include "_log.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void canvas_register_swapchain_sink(void)
{
    const DvzStreamSinkBackend* backend = dvz_canvas_swapchain_sink_backend();
    if (backend)
    {
        dvz_stream_sink_registry_register(dvz_stream_sink_registry_default(), backend);
    }
    else
    {
        log_warn("swapchain sink backend unavailable");
    }
}



/**
 * Build stream configuration from the current canvas surface metadata.
 *
 * @param canvas canvas owning the stream configuration
 * @returns stream configuration matching the current canvas extent and format
 */
static DvzStreamConfig canvas_stream_config(const DvzCanvas* canvas)
{
    ANN(canvas);
    DvzStreamConfig cfg = dvz_stream_default_config();
    if (canvas->surface)
    {
        cfg.width = canvas->surface->extent.width;
        cfg.height = canvas->surface->extent.height;
    }
    if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        cfg.color_format = canvas->cfg.color_format;
    }
    return cfg;
}



/**
 * Create a new canvas stream with required sinks.
 *
 * @param canvas canvas owning the stream
 * @param enable_video true to attach the video sink
 * @param cfg optional video sink configuration when enable_video is true
 * @param out_stream destination pointer receiving the new stream on success
 * @returns 0 on success or -1 when stream/sink setup fails
 */
static int canvas_create_stream_with_sinks(
    DvzCanvas* canvas, bool enable_video, const DvzVideoSinkConfig* cfg, DvzStream** out_stream)
{
    ANN(canvas);
    ANN(out_stream);
    *out_stream = NULL;

    dvz_canvas_window_surface_refresh(canvas);
    DvzStreamConfig stream_cfg = canvas_stream_config(canvas);
    DvzStream* stream =
        dvz_stream_create(canvas->device, dvz_stream_sink_registry_default(), &stream_cfg);
    if (!stream)
    {
        log_error("failed to create canvas stream");
        return -1;
    }

    canvas_register_swapchain_sink();
    const DvzStreamSinkBackend* swapchain_backend = dvz_canvas_swapchain_sink_backend();
    if (!swapchain_backend)
    {
        log_error("swapchain sink backend unavailable");
        dvz_stream_destroy(stream);
        return -1;
    }
    if (dvz_stream_attach_sink(stream, swapchain_backend, canvas) != 0)
    {
        log_error("failed to attach swapchain sink to canvas stream");
        dvz_stream_destroy(stream);
        return -1;
    }

    if (enable_video)
    {
        dvz_stream_sink_registry_register(
            dvz_stream_sink_registry_default(), dvz_stream_sink_video());
        const DvzStreamSinkBackend* video_backend = dvz_stream_sink_video();
        if (!video_backend)
        {
            log_error("video sink backend unavailable");
            dvz_stream_destroy(stream);
            return -1;
        }
        if (dvz_stream_attach_sink(stream, video_backend, cfg) != 0)
        {
            log_error("failed to attach video sink to canvas stream");
            dvz_stream_destroy(stream);
            return -1;
        }
    }

    *out_stream = stream;
    return 0;
}



/**
 * Replace the current canvas stream with a rebuilt stream and refreshed sink set.
 *
 * @param canvas canvas owning the stream
 * @param enable_video true to keep/attach video sink, false to detach it
 * @param cfg optional video sink configuration used when enable_video is true
 * @returns 0 on success or -1 when rebuilding fails
 */
static int
canvas_rebuild_stream(DvzCanvas* canvas, bool enable_video, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);

    DvzStream* replacement = NULL;
    if (canvas_create_stream_with_sinks(canvas, enable_video, cfg, &replacement) != 0)
    {
        return -1;
    }

    DvzStream* previous = canvas->stream;
    bool previous_started = canvas->stream_started;
    if (previous_started && previous)
    {
        dvz_stream_stop(previous);
    }

    canvas->stream = replacement;
    canvas->stream_started = false;
    canvas->swapchain_sink_attached = true;
    canvas->video_sink_enabled = enable_video;

    if (previous)
    {
        dvz_stream_destroy(previous);
    }
    return 0;
}



/*************************************************************************************************/
/*  API                                                                                          */
/*************************************************************************************************/

/**
 * Prepare the canvas stream by attaching required sinks.
 *
 * @param canvas canvas that owns the stream
 * @returns 0 on success or -1 when attachment fails
 */
int dvz_canvas_stream_prepare(DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->stream);
    canvas_register_swapchain_sink();
    if (!canvas->swapchain_sink_attached)
    {
        const DvzStreamSinkBackend* backend = dvz_canvas_swapchain_sink_backend();
        if (!backend)
        {
            log_error("swapchain sink backend unavailable");
            return -1;
        }
        if (dvz_stream_attach_sink(canvas->stream, backend, canvas) != 0)
        {
            log_error("failed to attach swapchain sink to canvas stream");
            return -1;
        }
        canvas->swapchain_sink_attached = true;
    }
    return 0;
}



/**
 * Start the stream when the first frame is ready.
 *
 * @param canvas canvas owning the stream
 * @param frame frame metadata to pass to the sinks
 * @returns 0 on success or -1 on failure
 */
int dvz_canvas_stream_start(DvzCanvas* canvas, const DvzStreamFrame* frame)
{
    ANN(canvas);
    ANN(canvas->stream);
    ANN(frame);
    if (canvas->stream_started)
    {
        return 0;
    }
    int result = dvz_stream_start(canvas->stream, frame);
    if (result == 0)
    {
        canvas->stream_started = true;
    }
    return result;
}



/**
 * Submit a frame to every attached sink.
 *
 * @param canvas canvas owning the stream
 * @param wait_value timeline wait value forwarded to the sinks
 * @returns stream submission result
 */
int dvz_canvas_stream_submit(DvzCanvas* canvas, uint64_t wait_value)
{
    ANN(canvas);
    ANN(canvas->stream);
    if (!canvas->stream_started)
    {
        log_error("canvas stream must be started before calling submit");
        return -1;
    }
    return dvz_stream_submit(canvas->stream, wait_value);
}



/**
 * Enable or disable the video sink on the canvas stream.
 *
 * @param canvas canvas owning the stream
 * @param enable requested state
 * @param cfg optional sink configuration
 * @returns 0 on success or -1 when enabling/disabling fails
 */
int dvz_canvas_stream_enable_video(
    DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);
    ANN(canvas->stream);

    if (enable)
    {
        if (canvas->allocator.external == 0 || !canvas->supports_external_semaphore)
        {
            log_error("video sink requires external memory/semaphore support");
            return -1;
        }
        if (canvas->video_sink_enabled)
        {
            return 0;
        }
        return canvas_rebuild_stream(canvas, true, cfg);
    }

    if (!canvas->video_sink_enabled)
    {
        return 0;
    }

    return canvas_rebuild_stream(canvas, false, NULL);
}
