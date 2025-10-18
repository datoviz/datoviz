/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU data allocation utils                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_DATALLOC_UTILS
#define DVZ_HEADER_DATALLOC_UTILS



/*************************************************************************************************/
/*  Make GPU data allocation                                                                     */
/*************************************************************************************************/

#include "datalloc.h"
#include "resources.h"
#include "resources_utils.h"



/*************************************************************************************************/
/*  Allocs utils                                                                                 */
/*************************************************************************************************/

static DvzAlloc** _get_alloc(DvzDatAlloc* datalloc, DvzBufferType type, bool mappable)
{
    ANN(datalloc);
    CHECK_BUFFER_TYPE

    uint32_t idx = 2 * (uint32_t)(type - 1) + (uint32_t)mappable - 1;
    ASSERT(idx < 2 * DVZ_BUFFER_TYPE_COUNT - 1);
    return &datalloc->allocators[idx];
}



static DvzAlloc* _make_allocator(
    DvzDatAlloc* datalloc, DvzResources* res, DvzBufferType type, bool mappable, DvzSize size)
{
    ANN(datalloc);
    CHECK_BUFFER_TYPE

    DvzAlloc** alloc = _get_alloc(datalloc, type, mappable);

    // Find alignment by looking at the buffers themselves.

    // WARNING: currently, a side-effect of requesting a buffer just to get the alignment is that
    // all shared buffers are automatically created here.

    DvzBuffer* buffer = _get_shared_buffer(res, type, mappable);
    DvzSize alignment = buffer->vma.alignment;
    ASSERT(alignment > 0);

    *alloc = dvz_alloc(size, alignment);
    return *alloc;
}



#endif
