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

static void canvas_register_swapchain_sink(DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->sink_registry);
    const DvzStreamSinkBackend* backend = dvz_canvas_swapchain_sink_backend();
    if (backend)
    {
        dvz_stream_sink_registry_register(canvas->sink_registry, backend);
    }
    else
    {
        log_warn("swapchain sink backend unavailable");
    }
}



static void canvas_register_offscreen_sink(DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->sink_registry);
    const DvzStreamSinkBackend* backend = dvz_canvas_offscreen_sink_backend();
    if (backend)
    {
        dvz_stream_sink_registry_register(canvas->sink_registry, backend);
    }
    else
    {
        log_warn("offscreen sink backend unavailable");
    }
}



static void canvas_register_live_image_sink(DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->sink_registry);
    const DvzStreamSinkBackend* backend = dvz_canvas_live_image_sink_backend();
    if (backend)
    {
        dvz_stream_sink_registry_register(canvas->sink_registry, backend);
    }
    else
    {
        log_warn("live-image sink backend unavailable");
    }
}



static const DvzStreamSinkBackend* canvas_primary_sink_backend(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->cfg.render_mode == DVZ_CANVAS_RENDER_MODE_OFFSCREEN)
    {
        canvas_register_offscreen_sink(canvas);
        return dvz_canvas_offscreen_sink_backend();
    }
    canvas_register_swapchain_sink(canvas);
    return dvz_canvas_swapchain_sink_backend();
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
 * Capture the current canvas frame into a temporary RGBA buffer.
 *
 * @param user_data canvas pointer
 * @param out_width destination width
 * @param out_height destination height
 * @param out_stride destination row stride in bytes
 * @param out_rgba destination pointer receiving a freshly allocated RGBA buffer
 * @returns 0 on success or -1 when capture fails
 */
static int canvas_capture_rgba_callback(
    void* user_data, uint32_t* out_width, uint32_t* out_height, size_t* out_stride,
    uint8_t** out_rgba)
{
    DvzCanvas* canvas = (DvzCanvas*)user_data;
    ANN(canvas);
    ANN(out_width);
    ANN(out_height);
    ANN(out_stride);
    ANN(out_rgba);
    int rc = dvz_canvas_capture_rgba(canvas, out_width, out_height, out_rgba);
    if (rc == 0)
    {
        *out_stride = (size_t)(*out_width) * 4;
    }
    return rc;
}



/**
 * Return whether the canvas can run external-handle video capture.
 *
 * @param canvas canvas instance
 * @returns true when external memory and semaphore exports are available
 */
static bool canvas_has_external_video_support(const DvzCanvas* canvas)
{
    ANN(canvas);
#if OS_UNIX
    return (
        canvas->allocator != NULL && dvz_allocator_external(canvas->allocator) != 0 &&
        canvas->supports_external_semaphore);
#else
    return false;
#endif
}



/**
 * Resolve the requested capture mode against runtime canvas capabilities.
 *
 * @param canvas canvas instance
 * @param cfg optional sink configuration
 * @returns the effective capture mode to use
 */
static DvzVideoCaptureMode
canvas_resolve_video_capture_mode(const DvzCanvas* canvas, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);
    DvzVideoCaptureMode requested = DVZ_VIDEO_CAPTURE_AUTO;
    if (cfg)
    {
        requested = cfg->capture_mode;
    }
    if (requested == DVZ_VIDEO_CAPTURE_AUTO)
    {
        return canvas_has_external_video_support(canvas) ? DVZ_VIDEO_CAPTURE_EXTERNAL
                                                         : DVZ_VIDEO_CAPTURE_CPU_READBACK;
    }
    return requested;
}



/**
 * Create a new canvas stream with required sinks.
 *
 * @param canvas canvas owning the stream
 * @param enable_video true to attach the video sink
 * @param cfg optional video sink configuration when enable_video is true
 * @param capture_mode resolved capture mode for the video sink
 * @param out_stream destination pointer receiving the new stream on success
 * @returns 0 on success or -1 when stream/sink setup fails
 */
static int canvas_create_stream_with_sinks(
    DvzCanvas* canvas, bool enable_video, const DvzVideoSinkConfig* cfg,
    DvzVideoCaptureMode capture_mode, bool enable_live,
    const DvzCanvasLiveImageSinkConfig* live_cfg, DvzStream** out_stream)
{
    ANN(canvas);
    ANN(out_stream);
    *out_stream = NULL;
    ANN(canvas->sink_registry);

    dvz_canvas_window_surface_refresh(canvas);
    DvzStreamConfig stream_cfg = canvas_stream_config(canvas);
    DvzStream* stream = dvz_stream_create(canvas->device, canvas->sink_registry, &stream_cfg);
    if (!stream)
    {
        log_error("failed to create canvas stream");
        return -1;
    }

    const DvzStreamSinkBackend* primary_backend = canvas_primary_sink_backend(canvas);
    if (!primary_backend)
    {
        log_error("canvas primary sink backend unavailable");
        dvz_stream_destroy(stream);
        return -1;
    }
    if (dvz_stream_attach_sink(stream, primary_backend, canvas) != 0)
    {
        log_error("failed to attach primary sink to canvas stream");
        dvz_stream_destroy(stream);
        return -1;
    }

    if (enable_video)
    {
        dvz_stream_sink_registry_register(canvas->sink_registry, dvz_stream_sink_video());
        const DvzStreamSinkBackend* video_backend = dvz_stream_sink_video();
        if (!video_backend)
        {
            log_error("video sink backend unavailable");
            dvz_stream_destroy(stream);
            return -1;
        }
        DvzVideoSinkConfig sink_cfg = cfg ? *cfg : dvz_video_sink_default_config();
        sink_cfg.capture_mode = capture_mode;
        if (cfg == NULL)
        {
            if (stream_cfg.width > 0)
            {
                sink_cfg.encoder.width = stream_cfg.width;
            }
            if (stream_cfg.height > 0)
            {
                sink_cfg.encoder.height = stream_cfg.height;
            }
        }
        else
        {
            if (sink_cfg.encoder.width == 0 && stream_cfg.width > 0)
            {
                sink_cfg.encoder.width = stream_cfg.width;
            }
            if (sink_cfg.encoder.height == 0 && stream_cfg.height > 0)
            {
                sink_cfg.encoder.height = stream_cfg.height;
            }
        }
        if (capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK && sink_cfg.capture_rgba == NULL)
        {
            sink_cfg.capture_rgba = canvas_capture_rgba_callback;
            sink_cfg.capture_user_data = canvas;
        }
        if (dvz_stream_attach_sink(stream, video_backend, &sink_cfg) != 0)
        {
            log_error("failed to attach video sink to canvas stream");
            dvz_stream_destroy(stream);
            return -1;
        }
    }

    if (enable_live)
    {
        canvas_register_live_image_sink(canvas);
        const DvzStreamSinkBackend* live_backend = dvz_canvas_live_image_sink_backend();
        if (!live_backend)
        {
            log_error("live-image sink backend unavailable");
            dvz_stream_destroy(stream);
            return -1;
        }
        if (dvz_stream_attach_sink(stream, live_backend, live_cfg) != 0)
        {
            log_error("failed to attach live-image sink to canvas stream");
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
 * @param capture_mode resolved capture mode for the rebuilt video sink
 * @returns 0 on success or -1 when rebuilding fails
 */
static int
canvas_rebuild_stream(
    DvzCanvas* canvas, bool enable_video, const DvzVideoSinkConfig* cfg,
    DvzVideoCaptureMode capture_mode, bool enable_live,
    const DvzCanvasLiveImageSinkConfig* live_cfg)
{
    ANN(canvas);

    DvzStream* replacement = NULL;
    if (canvas_create_stream_with_sinks(
            canvas, enable_video, cfg, capture_mode, enable_live, live_cfg, &replacement) != 0)
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
    canvas->primary_sink_attached = true;
    canvas->video_sink_enabled = enable_video;
    canvas->video_capture_mode = enable_video ? capture_mode : DVZ_VIDEO_CAPTURE_AUTO;
    canvas->live_image_sink_enabled = enable_live;

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
    if (!canvas->primary_sink_attached)
    {
        const DvzStreamSinkBackend* backend = canvas_primary_sink_backend(canvas);
        if (!backend)
        {
            log_error("primary sink backend unavailable");
            return -1;
        }
        if (dvz_stream_attach_sink(canvas->stream, backend, canvas) != 0)
        {
            log_error("failed to attach primary sink to canvas stream");
            return -1;
        }
        canvas->primary_sink_attached = true;
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
        DvzVideoCaptureMode capture_mode = canvas_resolve_video_capture_mode(canvas, cfg);
        if (
            capture_mode == DVZ_VIDEO_CAPTURE_EXTERNAL &&
            !canvas_has_external_video_support(canvas))
        {
            log_error(
                "video sink external capture requires exportable external memory and semaphore "
                "handles on this platform");
            return -1;
        }
        if (canvas->video_sink_enabled && canvas->video_capture_mode == capture_mode)
        {
            return 0;
        }
        canvas->video_sink_cfg = cfg ? *cfg : dvz_video_sink_default_config();
        canvas->video_sink_cfg.capture_mode = capture_mode;
        canvas->video_sink_cfg_valid = true;
        return canvas_rebuild_stream(
            canvas, true, &canvas->video_sink_cfg, capture_mode, canvas->live_image_sink_enabled,
            canvas->live_image_sink_enabled ? &canvas->live_image_sink_cfg : NULL);
    }

    if (!canvas->video_sink_enabled)
    {
        return 0;
    }

    canvas->video_sink_cfg_valid = false;
    return canvas_rebuild_stream(
        canvas, false, NULL, DVZ_VIDEO_CAPTURE_AUTO, canvas->live_image_sink_enabled,
        canvas->live_image_sink_enabled ? &canvas->live_image_sink_cfg : NULL);
}



/**
 * Enable or disable the live-image sink on the canvas stream.
 *
 * @param canvas canvas owning the stream
 * @param enable requested state
 * @param cfg sink configuration when enabling
 * @returns 0 on success or -1 when enabling/disabling fails
 */
int dvz_canvas_stream_enable_live_image(
    DvzCanvas* canvas, bool enable, const DvzCanvasLiveImageSinkConfig* cfg)
{
    ANN(canvas);
    ANN(canvas->stream);

    if (enable)
    {
        if (!cfg || !cfg->callback)
        {
            log_error("live-image sink requires a valid callback");
            return -1;
        }
        canvas->live_image_sink_cfg = *cfg;
        if (canvas->live_image_sink_enabled)
        {
            return 0;
        }
        return canvas_rebuild_stream(
            canvas, canvas->video_sink_enabled,
            canvas->video_sink_enabled && canvas->video_sink_cfg_valid ? &canvas->video_sink_cfg
                                                                       : NULL,
            canvas->video_sink_enabled ? canvas->video_capture_mode : DVZ_VIDEO_CAPTURE_AUTO, true,
            &canvas->live_image_sink_cfg);
    }

    if (!canvas->live_image_sink_enabled)
    {
        return 0;
    }

    return canvas_rebuild_stream(
        canvas, canvas->video_sink_enabled,
        canvas->video_sink_enabled && canvas->video_sink_cfg_valid ? &canvas->video_sink_cfg
                                                                   : NULL,
        canvas->video_sink_enabled ? canvas->video_capture_mode : DVZ_VIDEO_CAPTURE_AUTO, false,
        NULL);
}
