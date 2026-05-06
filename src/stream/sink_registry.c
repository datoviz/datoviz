/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream sink registry                                                                         */
/*************************************************************************************************/

#include "datoviz/stream.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzStreamSinkRegistry
{
    const DvzStreamSinkBackend** items;
    size_t count;
    size_t capacity;
};


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool stream_registry_reserve(DvzStreamSinkRegistry* registry, size_t capacity)
{
    if (!registry || registry->capacity >= capacity)
    {
        return registry != NULL;
    }
    size_t new_cap = (registry->capacity == 0) ? 4 : registry->capacity;
    while (new_cap < capacity)
    {
        if (new_cap > SIZE_MAX / 2)
        {
            log_error("stream sink registry capacity overflow");
            return false;
        }
        new_cap *= 2;
    }
    if (new_cap > SIZE_MAX / sizeof(const DvzStreamSinkBackend*))
    {
        log_error("stream sink registry byte-size overflow");
        return false;
    }
    const DvzStreamSinkBackend** ptr =
        (const DvzStreamSinkBackend**)dvz_realloc(registry->items, new_cap * sizeof(*ptr));
    if (!ptr)
    {
        log_error("failed to resize stream sink registry");
        return false;
    }
    dvz_memset(
        ptr + registry->capacity, (new_cap - registry->capacity) * sizeof(*ptr), 0,
        (new_cap - registry->capacity) * sizeof(*ptr));
    registry->items = ptr;
    registry->capacity = new_cap;
    return true;
}


static bool stream_sink_registered(DvzStreamSinkRegistry* registry, const char* name)
{
    if (!registry || !name)
    {
        return false;
    }
    for (size_t i = 0; i < registry->count; ++i)
    {
        const DvzStreamSinkBackend* backend = registry->items[i];
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

/**
 * Allocate a stream sink registry instance.
 *
 * @returns allocated registry or NULL on failure
 */
DVZ_EXPORT DvzStreamSinkRegistry* dvz_stream_sink_registry_create(void)
{
    DvzStreamSinkRegistry* registry =
        (DvzStreamSinkRegistry*)dvz_calloc(1, sizeof(DvzStreamSinkRegistry));
    if (!registry)
    {
        log_error("failed to allocate stream sink registry");
        return NULL;
    }
    return registry;
}


/**
 * Destroy a stream sink registry and free its internal storage.
 *
 * @param registry registry to destroy (NULL-safe)
 */
DVZ_EXPORT void dvz_stream_sink_registry_destroy(DvzStreamSinkRegistry* registry)
{
    if (!registry)
    {
        return;
    }
    dvz_free(registry->items);
    dvz_free(registry);
}


/**
 * Register a sink backend for later attachment by name or automatic selection.
 *
 * @param registry registry that owns the backend database
 * @param backend backend descriptor with callbacks and a unique name
 */
DVZ_EXPORT void dvz_stream_sink_registry_register(
    DvzStreamSinkRegistry* registry, const DvzStreamSinkBackend* backend)
{
    if (!registry || !backend || !backend->name || backend->name[0] == '\0')
    {
        return;
    }
    if (stream_sink_registered(registry, backend->name))
    {
        return;
    }
    if (!stream_registry_reserve(registry, registry->count + 1))
    {
        return;
    }
    registry->items[registry->count++] = backend;
}


/**
 * Find a registered sink backend by name.
 *
 * @param registry registry to query
 * @param name backend name to look up
 * @returns the backend descriptor or NULL when no match is found
 */
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_registry_find(
    DvzStreamSinkRegistry* registry, const char* name)
{
    if (!registry || !name || name[0] == '\0')
    {
        return NULL;
    }
    for (size_t i = 0; i < registry->count; ++i)
    {
        const DvzStreamSinkBackend* backend = registry->items[i];
        if (backend && backend->name && strcmp(backend->name, name) == 0)
        {
            return backend;
        }
    }
    return NULL;
}


/**
 * Pick a sink backend by name or probe registered backends automatically.
 *
 * @param registry registry to query
 * @param name requested backend name, "auto", or NULL for automatic selection
 * @param config configuration forwarded to the backend probe callbacks
 * @returns the selected backend or NULL when none are available
 */
DVZ_EXPORT const DvzStreamSinkBackend*
dvz_stream_sink_registry_pick(
    DvzStreamSinkRegistry* registry, const char* name, const void* config)
{
    if (!registry)
    {
        return NULL;
    }

    bool auto_pick = (name == NULL) || (strcmp(name, "auto") == 0);

    if (!auto_pick)
    {
        const DvzStreamSinkBackend* backend =
            dvz_stream_sink_registry_find(registry, name);
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
        for (size_t i = 0; i < registry->count; ++i)
        {
            const DvzStreamSinkBackend* backend = registry->items[i];
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
