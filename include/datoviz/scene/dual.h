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

// Vulkan wrappers.
typedef struct DvzDrawIndirectCommand DvzDrawIndirectCommand;
typedef struct DvzDrawIndexedIndirectCommand DvzDrawIndexedIndirectCommand;

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



struct DvzDrawIndexedIndirectCommand
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};

struct DvzDrawIndirectCommand
{
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
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

DVZ_EXPORT void dvz_dual_resize(DvzDual* dual, uint32_t count);

DVZ_EXPORT void dvz_dual_update(DvzDual* dual);

DVZ_EXPORT void dvz_dual_destroy(DvzDual* dual);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

DVZ_EXPORT DvzDual dvz_dual_vertex(DvzRequester* rqr, uint32_t vertex_count, DvzSize vertex_size);

DVZ_EXPORT DvzDual dvz_dual_index(DvzRequester* rqr, uint32_t index_count);

DVZ_EXPORT DvzDual dvz_dual_indirect(DvzRequester* rqr, bool indexed);

DVZ_EXPORT DvzDual dvz_dual_dat(DvzRequester* rqr, DvzSize item_size);



EXTERN_C_OFF

#endif
