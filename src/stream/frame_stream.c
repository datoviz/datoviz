/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame stream implementation                                                                  */
/*************************************************************************************************/

#include "datoviz/stream/frame_stream.h"

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_log.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

struct DvzFrameStream
{
    DvzDevice* device;
    DvzFrameStreamConfig cfg;
    DvzFrameSink* sinks;
    size_t sink_count;
    size_t sink_capacity;
    bool started;
    DvzFrameStreamResources resources;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzFrameSink* dvz_frame_stream_sink_slot(DvzFrameStream* stream)
{
    ANN(stream);
    if (stream->sink_count == stream->sink_capacity)
    {
        size_t new_cap = stream->sink_capacity == 0 ? 2 : stream->sink_capacity * 2;
        DvzFrameSink* sinks =
            (DvzFrameSink*)realloc(stream->sinks, new_cap * sizeof(DvzFrameSink));
        if (!sinks)
        {
            log_error("failed to grow frame sink array");
            return NULL;
        }
        memset(sinks + stream->sink_capacity, 0, (new_cap - stream->sink_capacity) * sizeof(DvzFrameSink));
        stream->sinks = sinks;
        stream->sink_capacity = new_cap;
    }
    DvzFrameSink* sink = &stream->sinks[stream->sink_count++];
    memset(sink, 0, sizeof(*sink));
    sink->stream = stream;
    return sink;
}

static void dvz_frame_stream_release_sinks(DvzFrameStream* stream)
{
    if (!stream || !stream->sinks)
    {
        return;
    }
    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzFrameSink* sink = &stream->sinks[i];
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
/*  API                                                                                           */
/*************************************************************************************************/

DvzFrameStreamConfig dvz_frame_stream_default_config(void)
{
    DvzFrameStreamConfig cfg = {
        .width = 1920,
        .height = 1080,
        .fps = 60,
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
    };
    return cfg;
}

DvzFrameStream* dvz_frame_stream_create(DvzDevice* device, const DvzFrameStreamConfig* cfg)
{
    DvzFrameStream* stream = (DvzFrameStream*)calloc(1, sizeof(DvzFrameStream));
    ANN(stream);
    stream->device = device;
    stream->cfg = cfg ? *cfg : dvz_frame_stream_default_config();
    stream->resources.image = VK_NULL_HANDLE;
    stream->resources.memory = VK_NULL_HANDLE;
    stream->resources.memory_size = 0;
    stream->resources.memory_fd = -1;
    stream->resources.wait_semaphore_fd = -1;
    return stream;
}

void dvz_frame_stream_destroy(DvzFrameStream* stream)
{
    if (!stream)
    {
        return;
    }
    dvz_frame_stream_stop(stream);
    dvz_frame_stream_release_sinks(stream);
    free(stream);
}

int dvz_frame_stream_attach_sink(
    DvzFrameStream* stream, const DvzFrameSinkBackend* backend, const void* config)
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

    DvzFrameSink* sink = dvz_frame_stream_sink_slot(stream);
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

int dvz_frame_stream_attach_sink_named(
    DvzFrameStream* stream, const char* backend_name, const void* config)
{
    const DvzFrameSinkBackend* backend = dvz_frame_sink_backend_pick(backend_name, config);
    if (!backend)
    {
        log_error("frame sink backend '%s' not found", backend_name ? backend_name : "auto");
        return -1;
    }
    return dvz_frame_stream_attach_sink(stream, backend, config);
}

int dvz_frame_stream_start(DvzFrameStream* stream, const DvzFrameStreamResources* resources)
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
    if (resources)
    {
        stream->resources = *resources;
    }

    for (size_t i = 0; i < stream->sink_count; ++i)
    {
        DvzFrameSink* sink = &stream->sinks[i];
        if (!sink->backend || !sink->backend->start)
        {
            continue;
        }
        if (sink->backend->start(sink, &stream->resources) != 0)
        {
            log_error("frame sink '%s' failed to start", sink->backend->name ? sink->backend->name : "?");
            return -1;
        }
        sink->started = true;
    }
    stream->started = true;
    return 0;
}

int dvz_frame_stream_submit(DvzFrameStream* stream, uint64_t wait_value)
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
        DvzFrameSink* sink = &stream->sinks[i];
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

int dvz_frame_stream_stop(DvzFrameStream* stream)
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
        DvzFrameSink* sink = &stream->sinks[i];
        if (sink->started && sink->backend && sink->backend->stop)
        {
            sink->backend->stop(sink);
        }
        sink->started = false;
    }
    stream->started = false;
    return 0;
}

DvzDevice* dvz_frame_stream_device(DvzFrameStream* stream)
{
    return stream ? stream->device : NULL;
}

const DvzFrameStreamConfig* dvz_frame_stream_config(DvzFrameStream* stream)
{
    return stream ? &stream->cfg : NULL;
}

