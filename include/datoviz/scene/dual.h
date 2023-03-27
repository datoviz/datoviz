/*************************************************************************************************/
/* Dual                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_DUAL
#define DVZ_HEADER_DUAL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDual DvzDual;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzArray DvzArray;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDual
{
    DvzRequester* rqr;
    DvzArray* array;
    DvzId dat;
    uint32_t dirty_first;
    uint32_t dirty_last; // smallest contiguous interval encompassing all dirty intervals
    // dirty_last is the first non-dirty item (count=last-first)
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzDual dvz_dual(DvzRequester* rqr, DvzArray* array, DvzId dat);

DVZ_EXPORT void dvz_dual_dirty(DvzDual* dual, uint32_t first, uint32_t count);

DVZ_EXPORT void dvz_dual_clear(DvzDual* dual);

DVZ_EXPORT void dvz_dual_data(DvzDual* dual, uint32_t first, uint32_t count, void* data);

DVZ_EXPORT void dvz_dual_column(
    DvzDual* dual, DvzSize offset, DvzSize col_size, uint32_t first, uint32_t count,
    uint32_t repeats, void* data);

DVZ_EXPORT void dvz_dual_update(DvzDual* dual);

DVZ_EXPORT void dvz_dual_destroy(DvzDual* dual);



EXTERN_C_OFF

#endif
