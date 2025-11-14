/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream sink registry                                                                         */
/*************************************************************************************************/

#include "datoviz/stream.h"

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// Stream sink registry.
typedef struct
{
    const DvzStreamSinkBackend** items;
    size_t count;
    size_t capacity;
} DvzStreamSinkRegistry;

static DvzStreamSinkRegistry g_sink_registry = {0};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void stream_registry_reserve(size_t capacity)
{
    if (g_sink_registry.capacity >= capacity)
    {
        return;
    }
    size_t new_cap = g_sink_registry.capacity == 0 ? 4 : g_sink_registry.capacity;
    while (new_cap < capacity)
    {
        new_cap *= 2;
    }
    const DvzStreamSinkBackend** ptr =
        (const DvzStreamSinkBackend**)realloc(g_sink_registry.items, new_cap * sizeof(*ptr));
    if (!ptr)
    {
        log_error("failed to resize stream sink registry");
        return;
    }
    dvz_memset(
        ptr + g_sink_registry.capacity,
        (new_cap - g_sink_registry.capacity) * sizeof(*ptr),
        0,
        (new_cap - g_sink_registry.capacity) * sizeof(*ptr));
    g_sink_registry.items = ptr;
    g_sink_registry.capacity = new_cap;
}



static bool stream_sink_registered(const char* name)
{
    if (!name)
    {
        return false;
    }
    for (size_t i = 0; i < g_sink_registry.count; ++i)
    {
        const DvzStreamSinkBackend* backend = g_sink_registry.items[i];
        if (backend && backend->name && strcmp(backend->name, name) == 0)
        {
            return true;
        }
    }
    return false;
}



/*************************************************************************************************/
/*  API                                                                                          */
/*************************************************************************************************/

void dvz_stream_register_sink(const DvzStreamSinkBackend* backend)
{
    if (!backend || !backend->name || backend->name[0] == '\0')
    {
        return;
    }
    if (stream_sink_registered(backend->name))
    {
        return;
    }
    stream_registry_reserve(g_sink_registry.count + 1);
    g_sink_registry.items[g_sink_registry.count++] = backend;
}



const DvzStreamSinkBackend* dvz_stream_sink_find(const char* name)
{
    if (!name || name[0] == '\0')
    {
        return NULL;
    }
    for (size_t i = 0; i < g_sink_registry.count; ++i)
    {
        const DvzStreamSinkBackend* backend = g_sink_registry.items[i];
        if (backend && backend->name && strcmp(backend->name, name) == 0)
        {
            return backend;
        }
    }
    return NULL;
}



const DvzStreamSinkBackend* dvz_stream_sink_pick(const char* name, const void* config)
{
    bool auto_pick = (name == NULL) || (strcmp(name, "auto") == 0);

    if (!auto_pick)
    {
        const DvzStreamSinkBackend* backend = dvz_stream_sink_find(name);
        if (backend)
        {
            if (!backend->probe || backend->probe(config))
            {
                return backend;
            }
            log_warn("stream sink backend '%s' unavailable, falling back to auto", name);
            auto_pick = true;
        }
    }

    if (auto_pick)
    {
        for (size_t i = 0; i < g_sink_registry.count; ++i)
        {
            const DvzStreamSinkBackend* backend = g_sink_registry.items[i];
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
