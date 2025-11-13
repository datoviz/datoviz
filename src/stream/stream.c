/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream implementation                                                                        */
/*************************************************************************************************/

#include "datoviz/stream/frame_stream.h"

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_log.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

struct DvzStream
{
    DvzDevice* device;
    DvzStreamConfig cfg;
    DvzStreamSink* sinks;
    size_t sink_count;
    size_t sink_capacity;
    bool started;
    DvzStreamFrame frame;
    bool frame_valid;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void dvz_stream_reset_frame(DvzStreamFrame* frame)
{
    ANN(frame);
    frame->image = VK_NULL_HANDLE;
    frame->memory = VK_NULL_HANDLE;
    frame->memory_size = 0;
    frame->memory_fd = -1;
    frame->wait_semaphore_fd = -1;
}

static void dvz_stream_set_frame(DvzStream* stream, const DvzStreamFrame* frame)
{
    ANN(stream);
    if (frame)
    {
        stream->frame = *frame;
        stream->frame_valid = true;
    }
    else
    {
        dvz_stream_reset_frame(&stream->frame);
        stream->frame_valid = false;
    }
}

static DvzStreamSink* dvz_stream_sink_slot(DvzStream* stream)
{
    ANN(stream);
    if (stream->sink_count == stream->sink_capacity)
    {
        size_t new_cap = stream->sink_capacity == 0 ? 2 : stream->sink_capacity * 2;
        DvzStreamSink* sinks =
            (DvzStreamSink*)realloc(stream->sinks, new_cap * sizeof(DvzStreamSink));
        if (!sinks)
        {
            log_error("failed to grow frame sink array");
            return NULL;
        }
        memset(sinks + stream->sink_capacity, 0, (new_cap - stream->sink_capacity) * sizeof(DvzStreamSink));
        stream->sinks = sinks;
        stream->sink_capacity = new_cap;
    }
    DvzStreamSink* sink = &stream->sinks[stream->sink_count++];
    memset(sink, 0, sizeof(*sink));
    sink->stream = stream;
    return sink;
}

static void dvz_stream_release_sinks(DvzStream* stream)
{
    if (!stream || !stream->sinks)
    {
        return;
    }
    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzStreamSink* sink = &stream->sinks[i];
        if (sink && sink->backend)
        {
            if (sink->started && sink->backend->stop)
            {
                sink->backend->stop(sink);
            }
            if (sink->backend->destroy)
            {
                sink->backend->destroy(sink);
            }
        }
    }
    free(stream->sinks);
    stream->sinks = NULL;
    stream->sink_count = 0;
    stream->sink_capacity = 0;
}



/*************************************************************************************************/
/*  API                                                                                          */
/*************************************************************************************************/

DvzStreamConfig dvz_stream_default_config(void)
{
    DvzStreamConfig cfg = {
        .width = 1920,
        .height = 1080,
        .fps = 60,
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
    };
    return cfg;
}

DvzStream* dvz_stream_create(DvzDevice* device, const DvzStreamConfig* cfg)
{
    DvzStream* stream = (DvzStream*)calloc(1, sizeof(DvzStream));
    ANN(stream);
    stream->device = device;
    stream->cfg = cfg ? *cfg : dvz_stream_default_config();
    dvz_stream_reset_frame(&stream->frame);
    stream->frame_valid = false;
    return stream;
}

void dvz_stream_destroy(DvzStream* stream)
{
    if (!stream)
    {
        return;
    }
    dvz_stream_stop(stream);
    dvz_stream_release_sinks(stream);
    free(stream);
}

int dvz_stream_attach_sink(DvzStream* stream, const DvzStreamSinkBackend* backend, const void* config)
{
    ANN(stream);
    if (!backend)
    {
        log_error("invalid frame sink backend");
        return -1;
    }
    if (stream->started)
    {
        log_error("cannot attach sinks while stream is running");
        return -1;
    }
    if (backend->probe && !backend->probe(config))
    {
        log_error("frame sink backend '%s' unavailable", backend->name ? backend->name : "?");
        return -1;
    }

    DvzStreamSink* sink = dvz_stream_sink_slot(stream);
    if (!sink)
    {
        return -1;
    }
    sink->backend = backend;
    sink->config = config;

    if (backend->create && backend->create(sink, config) != 0)
    {
        log_error("failed to create frame sink '%s'", backend->name ? backend->name : "?");
        stream->sink_count -= 1;
        memset(sink, 0, sizeof(*sink));
        return -1;
    }
    return 0;
}

int dvz_stream_attach_sink_name(DvzStream* stream, const char* backend_name, const void* config)
{
    const DvzStreamSinkBackend* backend = dvz_stream_sink_pick(backend_name, config);
    if (!backend)
    {
        log_error("frame sink backend '%s' not found", backend_name ? backend_name : "auto");
        return -1;
    }
    return dvz_stream_attach_sink(stream, backend, config);
}

int dvz_stream_start(DvzStream* stream, const DvzStreamFrame* frame)
{
    ANN(stream);
    if (stream->started)
    {
        log_error("frame stream already started");
        return -1;
    }
    if (stream->sink_count == 0)
    {
        log_warn("starting frame stream without sinks");
    }
    if (!frame)
    {
        log_error("stream start requires a frame description");
        return -1;
    }
    dvz_stream_set_frame(stream, frame);

    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzStreamSink* sink = &stream->sinks[i];
        if (!sink->backend || !sink->backend->start)
        {
            continue;
        }
        if (sink->backend->start(sink, &stream->frame) != 0)
        {
            log_error("frame sink '%s' failed to start", sink->backend->name ? sink->backend->name : "?");
            return -1;
        }
        sink->started = true;
    }
    stream->started = true;
    return 0;
}

int dvz_stream_submit(DvzStream* stream, uint64_t wait_value)
{
    ANN(stream);
    if (!stream->started)
    {
        log_error("frame stream not started");
        return -1;
    }

    int rc = 0;
    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzStreamSink* sink = &stream->sinks[i];
        if (!sink->backend || !sink->backend->submit)
        {
            continue;
        }
        int sink_rc = sink->backend->submit(sink, wait_value);
        if (sink_rc != 0)
        {
            rc = sink_rc;
        }
    }
    return rc;
}

int dvz_stream_update(DvzStream* stream, const DvzStreamFrame* frame)
{
    ANN(stream);
    if (!stream->started)
    {
        log_error("cannot update stream before start");
        return -1;
    }
    if (!frame)
    {
        log_error("stream update requires a frame description");
        return -1;
    }

    dvz_stream_set_frame(stream, frame);

    int rc = 0;
    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzStreamSink* sink = &stream->sinks[i];
        if (!sink->backend)
        {
            continue;
        }
        if (sink->backend->update)
        {
            int sink_rc = sink->backend->update(sink, &stream->frame);
            if (sink_rc != 0)
            {
                rc = sink_rc;
            }
            continue;
        }
        if (sink->backend->stop && sink->backend->start)
        {
            sink->backend->stop(sink);
            sink->started = false;
            if (sink->backend->start(sink, &stream->frame) != 0)
            {
                log_error("failed to restart sink '%s' after frame update", sink->backend->name ? sink->backend->name : "?");
                rc = -1;
                continue;
            }
            sink->started = true;
        }
    }
    return rc;
}

int dvz_stream_stop(DvzStream* stream)
{
    if (!stream)
    {
        return 0;
    }
    if (!stream->started)
    {
        return 0;
    }

    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzStreamSink* sink = &stream->sinks[i];
        if (sink->started && sink->backend && sink->backend->stop)
        {
            sink->backend->stop(sink);
        }
        sink->started = false;
    }
    stream->started = false;
    return 0;
}

DvzDevice* dvz_stream_device(DvzStream* stream)
{
    return stream ? stream->device : NULL;
}

const DvzStreamConfig* dvz_stream_config(DvzStream* stream)
{
    return stream ? &stream->cfg : NULL;
}
