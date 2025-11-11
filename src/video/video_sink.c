/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video frame sink                                                                             */
/*************************************************************************************************/

#include "datoviz/video.h"

#include "datoviz/stream/frame_stream.h"

#include <stdlib.h>

#include "_alloc.h"
#include "_log.h"
#include "encoder.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct
{
    DvzVideoEncoder* encoder;
    DvzVideoSinkConfig cfg;
} DvzVideoSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzVideoSinkState* dvz_video_sink_state(DvzFrameSink* sink)
{
    ANN(sink);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state)
    {
        state = (DvzVideoSinkState*)calloc(1, sizeof(DvzVideoSinkState));
        ANN(state);
        sink->backend_data = state;
    }
    return state;
}

static const DvzVideoSinkConfig* dvz_video_sink_config(const void* config)
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

static void dvz_video_sink_cleanup(DvzFrameSink* sink)
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
    free(state);
    sink->backend_data = NULL;
}



/*************************************************************************************************/
/*  Backend callbacks                                                                            */
/*************************************************************************************************/

static bool dvz_video_sink_probe(const void* config)
{
    (void)config;
    return true;
}

static int dvz_video_sink_create(DvzFrameSink* sink, const void* config)
{
    ANN(sink);
    DvzVideoSinkState* state = dvz_video_sink_state(sink);
    ANN(state);

    const DvzVideoSinkConfig* requested = dvz_video_sink_config(config);
    state->cfg = *requested;

    state->encoder = dvz_video_encoder_create(dvz_frame_stream_device(sink->stream), &state->cfg.encoder);
    if (!state->encoder)
    {
        log_error("failed to create video encoder for frame sink");
        return -1;
    }
    return 0;
}

static int dvz_video_sink_start(DvzFrameSink* sink, const DvzFrameStreamResources* resources)
{
    ANN(sink);
    ANN(resources);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state || !state->encoder)
    {
        return -1;
    }
    return dvz_video_encoder_start(
        state->encoder,
        resources->image,
        resources->memory,
        resources->memory_size,
        resources->memory_fd,
        resources->wait_semaphore_fd,
        state->cfg.bitstream);
}

static int dvz_video_sink_submit(DvzFrameSink* sink, uint64_t wait_value)
{
    ANN(sink);
    DvzVideoSinkState* state = (DvzVideoSinkState*)sink->backend_data;
    if (!state || !state->encoder)
    {
        return -1;
    }
    return dvz_video_encoder_submit(state->encoder, wait_value);
}

static int dvz_video_sink_stop(DvzFrameSink* sink)
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

static void dvz_video_sink_destroy(DvzFrameSink* sink)
{
    dvz_video_sink_cleanup(sink);
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

const DvzFrameSinkBackend DVZ_FRAME_SINK_VIDEO_ENCODER = {
    .name = "video_encoder",
    .probe = dvz_video_sink_probe,
    .create = dvz_video_sink_create,
    .start = dvz_video_sink_start,
    .submit = dvz_video_sink_submit,
    .stop = dvz_video_sink_stop,
    .destroy = dvz_video_sink_destroy,
};

DVZ_EXPORT DvzVideoSinkConfig dvz_video_sink_default_config(void)
{
    DvzVideoSinkConfig cfg = {
        .encoder = dvz_video_encoder_default_config(),
        .bitstream = NULL,
    };
    return cfg;
}

DVZ_EXPORT const DvzFrameSinkBackend* dvz_video_sink_backend(void)
{
    return &DVZ_FRAME_SINK_VIDEO_ENCODER;
}
