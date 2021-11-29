/*************************************************************************************************/
/*  GPU data allocation                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ALLOCS
#define DVZ_HEADER_ALLOCS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "alloc.h"
#include "common.h"
#include "resources.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDatAlloc DvzDatAlloc;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDatAlloc
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzAlloc* allocators[2 * DVZ_BUFFER_TYPE_COUNT - 1]; // one dat allocator for each buffer(each
                                                         // type may be mappable or not)
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
DVZ_EXPORT void dvz_datalloc(DvzGpu* gpu, DvzResources* res, DvzDatAlloc* datalloc);



DVZ_EXPORT DvzSize dvz_datalloc_alloc(
    DvzDatAlloc* datalloc, DvzResources* res, DvzBufferType type, bool mappable, DvzSize req_size);



DVZ_EXPORT void
dvz_datalloc_dealloc(DvzDatAlloc* datalloc, DvzBufferType type, bool mappable, DvzSize offset);



/**
 * Show information about the allocations.
 *
 * @param datalloc the DvzDatalloc pointer
 */
DVZ_EXPORT void dvz_datalloc_stats(DvzDatAlloc* datalloc);



/**
 * Destroy a datalloc object.
 *
 * @param datalloc the datalloc
 */
DVZ_EXPORT void dvz_datalloc_destroy(DvzDatAlloc* datalloc);



EXTERN_C_OFF

#endif
