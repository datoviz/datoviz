/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU data allocation                                                                          */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datalloc.h"
#include "datalloc_utils.h"

#include <stdlib.h>



/*************************************************************************************************/
/*  Allocs                                                                                       */
/*************************************************************************************************/

void dvz_datalloc(DvzGpu* gpu, DvzResources* res, DvzDatAlloc* datalloc)
{
    ANN(gpu);
    ASSERT(dvz_obj_is_created(&gpu->obj));
    ANN(datalloc);
    ASSERT(!dvz_obj_is_created(&datalloc->obj));
    // NOTE: this function should only be called once, at context creation.

    log_trace("creating datalloc");

    // Create the resources.
    datalloc->gpu = gpu;

    // Initialize the allocators for all possible types of shared buffers.
    _make_allocator(datalloc, res, DVZ_BUFFER_TYPE_STAGING, true, DVZ_BUFFER_DEFAULT_SIZE);
    for (uint32_t i = 2; i <= DVZ_BUFFER_TYPE_COUNT; i++)
    {
        _make_allocator(datalloc, res, (DvzBufferType)i, false, DVZ_BUFFER_DEFAULT_SIZE);
        _make_allocator(datalloc, res, (DvzBufferType)i, true, DVZ_BUFFER_DEFAULT_SIZE);
    }

    dvz_obj_created(&datalloc->obj);
}



DvzSize dvz_datalloc_alloc(
    DvzDatAlloc* datalloc, DvzResources* res, DvzBufferType type, bool mappable, DvzSize req_size)
{
    ANN(datalloc);
    ASSERT(req_size > 0);
    CHECK_BUFFER_TYPE

    DvzSize resized = 0; // will be non-zero if the buffer must be resized
    DvzAlloc** alloc = _get_alloc(datalloc, type, mappable);
    if (!alloc)
    {
        log_error("could not find alloc type %d %s", type, mappable ? "mappable" : "");
        return 0;
    }
    // Make the allocation.
    DvzSize offset = dvz_alloc_new(*alloc, req_size, &resized);

    // Need to resize the underlying DvzBuffer.
    if (resized)
    {
        DvzBuffer* buffer = _get_shared_buffer(res, type, mappable);
        log_info(
            "resizing buffer %u type %d (mappable: %d) to %s", //
            (uint64_t)buffer->buffer, type, mappable, pretty_size(resized));
        dvz_buffer_resize(buffer, resized);
    }

    return offset;
}



void dvz_datalloc_dealloc(DvzDatAlloc* datalloc, DvzBufferType type, bool mappable, DvzSize offset)
{
    ANN(datalloc);
    CHECK_BUFFER_TYPE

    // Get the abstract DvzAlloc object associated to the dat's buffer.
    DvzAlloc** alloc = _get_alloc(datalloc, type, mappable);
    dvz_alloc_free(*alloc, offset);
}



void dvz_datalloc_stats(DvzDatAlloc* datalloc)
{
    ANN(datalloc);
    for (uint32_t i = 0; i < sizeof(datalloc->allocators) / sizeof(DvzAlloc*); i++)
    {
        printf("Buffer #%d\n", i);
        dvz_alloc_stats(datalloc->allocators[i]);
    }
}



void dvz_datalloc_monitoring(DvzDatAlloc* datalloc, DvzAllocMonitor* out)
{
    ANN(datalloc);

    DvzAlloc* alloc = NULL;

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_STAGING, true);
    dvz_alloc_size(alloc, &out->staging[0], &out->staging[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_VERTEX, false);
    dvz_alloc_size(alloc, &out->vertex[0], &out->vertex[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_VERTEX, true);
    dvz_alloc_size(alloc, &out->vertex_map[0], &out->vertex_map[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_INDEX, false);
    dvz_alloc_size(alloc, &out->index[0], &out->index[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_INDEX, true);
    dvz_alloc_size(alloc, &out->index_map[0], &out->index_map[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_STORAGE, false);
    dvz_alloc_size(alloc, &out->storage[0], &out->storage[1]);

    alloc = *_get_alloc(datalloc, DVZ_BUFFER_TYPE_STORAGE, true);
    dvz_alloc_size(alloc, &out->storage_map[0], &out->storage_map[1]);
}



void dvz_datalloc_destroy(DvzDatAlloc* datalloc)
{
    if (datalloc == NULL)
    {
        log_error("skip destruction of null datalloc");
        return;
    }
    log_trace("destroying datalloc");
    ANN(datalloc);
    ANN(datalloc->gpu);

    // Destroy the DvzDat allocators.
    for (uint32_t i = 0; i < 2 * DVZ_BUFFER_TYPE_COUNT - 1; i++)
        dvz_alloc_destroy(datalloc->allocators[i]);

    dvz_obj_destroyed(&datalloc->obj);
}
