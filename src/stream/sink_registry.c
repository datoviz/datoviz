/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame sink registry                                                                          */
/*************************************************************************************************/

#include "datoviz/stream/frame_stream.h"

#include <string.h>

#include "_log.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Built-in sinks                                                                               */
/*************************************************************************************************/

extern const DvzFrameSinkBackend DVZ_FRAME_SINK_VIDEO_ENCODER;

static const DvzFrameSinkBackend* const STREAM_SINKS[] = {
    &DVZ_FRAME_SINK_VIDEO_ENCODER,
};
static const size_t STREAM_SINK_COUNT = sizeof(STREAM_SINKS) / sizeof(STREAM_SINKS[0]);



/*************************************************************************************************/
/*  API                                                                                           */
/*************************************************************************************************/

const DvzFrameSinkBackend* dvz_frame_sink_backend_find(const char* name)
{
    if (!name || name[0] == '\0')
    {
        return NULL;
    }
    for (size_t i = 0; i < STREAM_SINK_COUNT; ++i)
    {
        const DvzFrameSinkBackend* backend = STREAM_SINKS[i];
        if (backend && backend->name && strcmp(backend->name, name) == 0)
        {
            return backend;
        }
    }
    return NULL;
}

const DvzFrameSinkBackend* dvz_frame_sink_backend_pick(const char* name, const void* config)
{
    bool auto_pick = (name == NULL) || (strcmp(name, "auto") == 0);

    if (!auto_pick)
    {
        const DvzFrameSinkBackend* backend = dvz_frame_sink_backend_find(name);
        if (backend)
        {
            if (!backend->probe || backend->probe(config))
            {
                return backend;
            }
            log_warn("frame sink backend '%s' unavailable, falling back to auto", name);
            auto_pick = true;
        }
    }

    if (auto_pick)
    {
        for (size_t i = 0; i < STREAM_SINK_COUNT; ++i)
        {
            const DvzFrameSinkBackend* backend = STREAM_SINKS[i];
            if (!backend)
            {
                continue;
            }
            if (!backend->probe || backend->probe(config))
            {
                return backend;
            }
        }
    }

    return NULL;
}
