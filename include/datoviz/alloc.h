/*************************************************************************************************/
/*  Allocation algorithm                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ALLOC
#define DVZ_HEADER_ALLOC



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ALLOC_DEFAULT_COUNT 16



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAlloc DvzAlloc;



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline DvzSize _align(DvzSize size, DvzSize alignment)
{
    if (alignment == 0)
        return size;
    ASSERT(alignment > 0);
    if (size % alignment == 0)
        return size;
    ASSERT(size % alignment < alignment);
    size += (alignment - (size % alignment));
    ASSERT(size % alignment == 0);
    return size;
}



static inline void _check_align(DvzSize n, DvzSize alignment)
{
    ASSERT(alignment == 0 || n % alignment == 0);
}



EXTERN_C_ON


/*************************************************************************************************/
/*  Functions */
/*************************************************************************************************/

/**
 * Create an abstract allocation object.
 *
 * This object handles allocation on a virtual buffer of a given size. It takes care of alignment.
 *
 * @param size the total size of the underlying buffer
 * @param alignment the required alignment for allocations
 */
DVZ_EXPORT DvzAlloc* dvz_alloc(DvzSize size, DvzSize alignment);

/**
 * Make a new allocation.
 *
 * @param alloc the DvzAlloc pointer
 * @param req_size the requested size for the allocation
 * @param[out] resized if the underlying virtual buffer had to be resized, the new size
 * @returns the offset of the allocated item within the virtual buffer
 */
DVZ_EXPORT DvzSize dvz_alloc_new(DvzAlloc* alloc, DvzSize req_size, DvzSize* resized);

/**
 * Remove an allocated item.
 *
 * @param alloc the DvzAlloc pointer
 * @param offset the offset of the allocated item to be removed
 */
DVZ_EXPORT void dvz_alloc_free(DvzAlloc* alloc, DvzSize offset);

/**
 * Return the size of one allocation at a given offset.
 *
 * @param alloc the DvzAlloc pointer
 * @param offset the offset of the allocation
 * @returns the size of the allocation
 */
DVZ_EXPORT DvzSize dvz_alloc_get(DvzAlloc* alloc, DvzSize offset);

/**
 * Return the total allocated size.
 *
 * @param alloc the DvzAlloc pointer
 * @returns the total allocated size
 */
DVZ_EXPORT DvzSize dvz_alloc_size(DvzAlloc* alloc);

/**
 * Show information about the allocations.
 *
 * @param alloc the DvzAlloc pointer
 */
DVZ_EXPORT void dvz_alloc_stats(DvzAlloc* alloc);

/**
 * Clear all allocations.
 *
 * @param alloc the DvzAlloc pointer
 */
DVZ_EXPORT void dvz_alloc_clear(DvzAlloc* alloc);


/**
 * Destroy a DvzAlloc instance.
 *
 * @param alloc the DvzAlloc pointer
 */
DVZ_EXPORT void dvz_alloc_destroy(DvzAlloc* alloc);



EXTERN_C_OFF

#endif
