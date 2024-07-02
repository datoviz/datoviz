/*************************************************************************************************/
/* Dual                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_DUAL
#define DVZ_HEADER_DUAL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_log.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDual DvzDual;

// Vulkan wrappers.
typedef struct DvzDrawIndirectCommand DvzDrawIndirectCommand;
typedef struct DvzDrawIndexedIndirectCommand DvzDrawIndexedIndirectCommand;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzArray DvzArray;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDual
{
    DvzBatch* batch;
    DvzArray* array;
    DvzId dat;
    uint32_t dirty_first;
    uint32_t dirty_last; // smallest contiguous interval encompassing all dirty intervals
    // dirty_last is the first non-dirty item (count=last-first)

    bool need_destroy; // whether the library is responsible for creating and thus destroying the
                       // dual
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

DvzDual dvz_dual(DvzBatch* batch, DvzArray* array, DvzId dat);

void dvz_dual_dirty(DvzDual* dual, uint32_t first, uint32_t count);

void dvz_dual_clear(DvzDual* dual);

void dvz_dual_data(DvzDual* dual, uint32_t first, uint32_t count, void* data);

void dvz_dual_column(
    DvzDual* dual, DvzSize offset, DvzSize col_size, uint32_t first, uint32_t count,
    uint32_t repeats, void* data);

void dvz_dual_resize(DvzDual* dual, uint32_t count);

void dvz_dual_update(DvzDual* dual);

void dvz_dual_destroy(DvzDual* dual);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

DvzDual dvz_dual_vertex(DvzBatch* batch, uint32_t vertex_count, DvzSize vertex_size, int flags);

DvzDual dvz_dual_index(DvzBatch* batch, uint32_t index_count, int flags);

DvzDual dvz_dual_indirect(DvzBatch* batch, bool indexed);

DvzDual dvz_dual_dat(DvzBatch* batch, DvzSize item_size, int flags);



EXTERN_C_OFF

#endif
