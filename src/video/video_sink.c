/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video frame sink                                                                             */
/*************************************************************************************************/

#include "datoviz/video.h"

#include "datoviz/stream.h"

#include <stdlib.h>

#include "_alloc.h"
#include "_log.h"
#include "encoder.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzVideoEncoder* encoder;
    DvzVideoSinkConfig cfg;
    DvzVideoCaptureMode resolved_capture_mode;
} DvzVideoSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure the sink has an allocated encoder state structure.
 *
 * @param sink frame sink requesting the state
 * @returns pointer to the backend state (freshly allocated if needed)
 */
static DvzVideoSinkState* video_sink_state(DvzStreamSink* sink)
{
    ANN(sink);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state)
    {
        state = (DvzVideoSinkState*)dvz_calloc(1, sizeof(DvzVideoSinkState));
        ANN(state);
        sink->backend_data = state;
    }
    return state;
}



/**
 * Resolve the provided configuration or fall back to a static default.
 *
 * @param config optional user-provided sink configuration
 * @returns the configuration that should drive this sink
 */
static const DvzVideoSinkConfig* video_sink_config(const void* config)
{
    if (config)
    {
        return (const DvzVideoSinkConfig*)config;
    }
    static DvzVideoSinkConfig default_cfg;
    static bool initialized = false;
    if (!initialized)
    {
        default_cfg = dvz_video_sink_default_config();
        initialized = true;
    }
    return &default_cfg;
}



/**
 * Resolve sink capture mode from the requested configuration.
 *
 * @param cfg sink configuration
 * @returns requested capture mode when explicit, or EXTERNAL as default for AUTO
 */
static DvzVideoCaptureMode video_sink_resolve_capture_mode(const DvzVideoSinkConfig* cfg)
{
    ANN(cfg);
    if (cfg->capture_mode == DVZ_VIDEO_CAPTURE_AUTO)
    {
        return DVZ_VIDEO_CAPTURE_EXTERNAL;
    }
    return cfg->capture_mode;
}



/**
 * Release the encoder instance and drop the backend data blob.
 *
 * @param sink stream sink whose state should be freed
 */
static void video_sink_cleanup(DvzStreamSink* sink)
{
    if (!sink || !sink->backend_data)
    {
        return;
    }
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (state->encoder)
    {
        dvz_video_encoder_destroy(state->encoder);
        state->encoder = NULL;
    }
    dvz_free(state);
    sink->backend_data = NULL;
}



/*************************************************************************************************/
/*  Backend callbacks                                                                            */
/*************************************************************************************************/

/**
 * Always return true, the video sink remains available when linked in.
 *
 * @param config unused probe configuration
 * @returns true indicating the backend can be used
 */
static bool video_sink_probe(const void* config)
{
    (void)config;
    return true;
}



/**
 * Initialize the video encoder and associate it with the sink state.
 *
 * @param sink stream sink owning the encoder
 * @param config backend-config pointer
 * @returns 0 when the encoder is ready or -1 when creation fails
 */
static int video_sink_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    DvzVideoSinkState* state = video_sink_state(sink);
    ANN(state);

    const DvzVideoSinkConfig* requested = video_sink_config(config);
    state->cfg = *requested;
    state->resolved_capture_mode = video_sink_resolve_capture_mode(&state->cfg);
    if (state->resolved_capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK)
    {
        const char* backend = state->cfg.encoder.backend;
        if (backend == NULL || backend[0] == '\0' || strcmp(backend, "auto") == 0)
        {
            state->cfg.encoder.backend = "kvazaar";
        }
    }

    const DvzStreamConfig* stream_cfg = dvz_stream_config(sink->stream);
    if (stream_cfg)
    {
        state->cfg.encoder.color_format = stream_cfg->color_format;
    }

    state->encoder =
        dvz_video_encoder_create(dvz_stream_device(sink->stream), &state->cfg.encoder);
    if (!state->encoder)
    {
        log_error("failed to create video encoder for frame sink");
        return -1;
    }
    return 0;
}



/**
 * Start encoding by passing the Vulkan frame to the encoder backend.
 *
 * @param sink stream sink that owns the encoder
 * @param frame frame metadata describing the image and sync handles
 * @returns backend return code (0 on success)
 */
static int video_sink_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state || !state->encoder)
    {
        return -1;
    }
    if (state->resolved_capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK)
    {
        return dvz_video_encoder_start(
            state->encoder, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, -1, -1, state->cfg.bitstream);
    }
    return dvz_video_encoder_start(
        state->encoder, frame->image, frame->memory, frame->memory_size, frame->memory_fd,
        frame->wait_semaphore_fd, state->cfg.bitstream);
}



/**
 * Forward the timeline submission to the encoder backend.
 *
 * @param sink stream sink containing the encoder
 * @param wait_value timeline semaphore value
 * @returns backend return code (0 on success)
 */
static int video_sink_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state || !state->encoder)
    {
        return -1;
    }
    if (state->resolved_capture_mode == DVZ_VIDEO_CAPTURE_CPU_READBACK)
    {
        if (!state->cfg.capture_rgba)
        {
            log_error("CPU readback capture mode requires capture_rgba callback");
            return -1;
        }
        uint32_t width = 0;
        uint32_t height = 0;
        size_t stride = 0;
        uint8_t* rgba = NULL;
        if (state->cfg.capture_rgba(
                state->cfg.capture_user_data, &width, &height, &stride, &rgba) != 0)
        {
            return -1;
        }
        if (rgba == NULL || width == 0 || height == 0)
        {
            log_error("CPU readback capture callback returned invalid frame");
            if (rgba)
            {
                if (state->cfg.release_rgba)
                {
                    state->cfg.release_rgba(state->cfg.capture_user_data, rgba);
                }
                else
                {
                    dvz_free(rgba);
                }
            }
            return -1;
        }
        if (stride == 0)
        {
            stride = (size_t)width * 4;
        }
        int rc =
            dvz_video_encoder_submit_rgba(state->encoder, rgba, width, height, stride, wait_value);
        if (state->cfg.release_rgba)
        {
            state->cfg.release_rgba(state->cfg.capture_user_data, rgba);
        }
        else
        {
            dvz_free(rgba);
        }
        return rc;
    }
    return dvz_video_encoder_submit(state->encoder, wait_value);
}



/**
 * Stop the encoder and return any backend error.
 *
 * @param sink stream sink owning the encoder
 * @returns 0 when stopped successfully or a negative backend error
 */
static int video_sink_stop(DvzStreamSink* sink)
{
    if (!sink)
    {
        return 0;
    }
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (state && state->encoder)
    {
        return dvz_video_encoder_stop(state->encoder);
    }
    return 0;
}



/**
 * Destroy the sink and clean up resources.
 *
 * @param sink stream sink being destroyed
 */
static void video_sink_destroy(DvzStreamSink* sink) { video_sink_cleanup(sink); }



/**
 * Restart encoding when a new frame description is provided.
 *
 * @param sink stream sink owning the encoder
 * @param frame updated frame metadata
 * @returns backend return code from `dvz_video_sink_start`
 */
static int video_sink_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state || !state->encoder)
    {
        return -1;
    }
    state->resolved_capture_mode = video_sink_resolve_capture_mode(&state->cfg);
    dvz_video_encoder_stop(state->encoder);
    sink->started = false;
    return video_sink_start(sink, frame);
}



const DvzStreamSinkBackend DVZ_STREAM_SINK_VIDEO = {
    .name = "sink.video",
    .probe = video_sink_probe,
    .create = video_sink_create,
    .start = video_sink_start,
    .submit = video_sink_submit,
    .stop = video_sink_stop,
    .update = video_sink_update,
    .destroy = video_sink_destroy,
};



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

/**
 * Return the default sink configuration wrapping the encoder defaults.
 *
 * @returns sink configuration with default encoder settings and no bitstream file
 */
DVZ_EXPORT DvzVideoSinkConfig dvz_video_sink_default_config(void)
{
    DvzVideoSinkConfig cfg = {
        .encoder = dvz_video_encoder_default_config(),
        .bitstream = NULL,
        .capture_mode = DVZ_VIDEO_CAPTURE_AUTO,
        .capture_rgba = NULL,
        .release_rgba = NULL,
        .capture_user_data = NULL,
    };
    return cfg;
}



/**
 * Return the video sink backend descriptor.
 *
 * @returns pointer to the `sink.video` backend
 */
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_video(void)
{
    return &DVZ_STREAM_SINK_VIDEO;
}
