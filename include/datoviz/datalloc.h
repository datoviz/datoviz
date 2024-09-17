/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU data allocation                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ALLOCS
#define DVZ_HEADER_ALLOCS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "alloc.h"
#include "resources.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef DvzSize DvzSizePair[2];
typedef struct DvzDatAlloc DvzDatAlloc;
typedef struct DvzAllocMonitor DvzAllocMonitor;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDatAlloc
{
    DvzObject obj;
    DvzGpu* gpu;

    // one dat allocator for each buffer (each type may be mappable or not)
    DvzAlloc* allocators[2 * DVZ_BUFFER_TYPE_COUNT - 1];
};



struct DvzAllocMonitor
{
    DvzSizePair staging;
    DvzSizePair vertex;
    DvzSizePair vertex_map;
    DvzSizePair index;
    DvzSizePair index_map;
    DvzSizePair storage;
    DvzSizePair storage_map;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

/**
 * Create a datalloc object.
 *
 * This object is responsible for the allocation of buffer regions on GPU buffers. It is used by
 * the context when allocating new Dats.
 *
 * @param gpu the GPU
 * @param res the DvzResources object
 * @param datalloc the DatAlloc object
 */
void dvz_datalloc(DvzGpu* gpu, DvzResources* res, DvzDatAlloc* datalloc);



DvzSize dvz_datalloc_alloc(
    DvzDatAlloc* datalloc, DvzResources* res, DvzBufferType type, bool mappable, DvzSize req_size);



void dvz_datalloc_dealloc(
    DvzDatAlloc* datalloc, DvzBufferType type, bool mappable, DvzSize offset);



void dvz_datalloc_monitoring(DvzDatAlloc* datalloc, DvzAllocMonitor* out);



/**
 * Show information about the allocations.
 *
 * @param datalloc the DvzDatalloc pointer
 */
void dvz_datalloc_stats(DvzDatAlloc* datalloc);



/**
 * Destroy a datalloc object.
 *
 * @param datalloc the datalloc
 */
void dvz_datalloc_destroy(DvzDatAlloc* datalloc);



EXTERN_C_OFF

#endif
