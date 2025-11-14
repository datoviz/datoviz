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
    static bool registered = false;
    if (registered)
    {
        return;
    }
    const DvzStreamSinkBackend* backend = dvz_canvas_swapchain_sink_backend();
    if (backend)
    {
        dvz_stream_register_sink(backend);
    }
    else
    {
        log_warn("swapchain sink backend unavailable");
    }
    registered = true;
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
 * @returns 0 on success or -1 when enabling fails
 */
int dvz_canvas_stream_enable_video(
    DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);
    ANN(canvas->stream);
    if (enable)
    {
        if (canvas->video_sink_enabled)
        {
            return 0;
        }
        if (canvas->stream_started)
        {
            log_error("cannot enable video sink after canvas stream start");
            return -1;
        }
        const DvzStreamSinkBackend* backend = dvz_stream_sink_video();
        if (!backend)
        {
            log_error("video sink backend unavailable");
            return -1;
        }
        if (dvz_stream_attach_sink(canvas->stream, backend, cfg) != 0)
        {
            log_error("failed to attach video sink to canvas stream");
            return -1;
        }
        canvas->video_sink_enabled = true;
        return 0;
    }

    if (!canvas->video_sink_enabled)
    {
        return 0;
    }

    log_warn("detaching a video sink requires rebuilding the canvas stream");
    canvas->video_sink_enabled = false;
    return 0;
}
