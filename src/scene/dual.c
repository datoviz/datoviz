/*************************************************************************************************/
/*  Dual                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/dual.h"
#include "request.h"
#include "scene/array.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzDual dvz_dual(DvzRequester* rqr, DvzArray* array, DvzId dat)
{
    ANN(rqr);
    ANN(array);

    DvzDual dual = {0};
    dual.rqr = rqr;
    dual.array = array;
    dual.dat = dat;

    dvz_dual_clear(&dual);

    return dual;
}



void dvz_dual_dirty(DvzDual* dual, uint32_t first, uint32_t count)
{
    ANN(dual);
    ASSERT(count > 0);
    uint32_t last = first + count;

    dual->dirty_first = MIN(dual->dirty_first, first);
    dual->dirty_last = MAX(dual->dirty_last, last);

    ASSERT(dual->dirty_first < dual->dirty_last);
    ASSERT(dual->dirty_first < dual->array->item_count);
    ASSERT(dual->dirty_last <= dual->array->item_count);
}



void dvz_dual_clear(DvzDual* dual)
{
    ANN(dual);
    dual->dirty_first = UINT32_MAX;
    dual->dirty_last = 0;
}



void dvz_dual_data(DvzDual* dual, uint32_t first, uint32_t count, void* data)
{
    ANN(dual);
    ANN(dual->array);
    ANN(data);
    ASSERT(count > 0);

    // Copy the data to the array.
    DvzSize item_size = dual->array->item_size;
    ASSERT(item_size > 0);

    dvz_array_data(dual->array, first, count, count, data);

    dvz_dual_dirty(dual, first, count);
}



void dvz_dual_update(DvzDual* dual)
{
    ANN(dual);
    ANN(dual->rqr);
    ANN(dual->array);

    if (dual->dirty_first == UINT32_MAX)
    {
        log_trace("skip dvz_dual_update() on non-dirty dual");
        return;
    }

    // Emit a dat_update command.
    DvzArray* array = dual->array;
    DvzSize item_size = array->item_size;
    DvzSize offset = dual->dirty_first * item_size;
    DvzSize size = (DvzSize)((int64_t)dual->dirty_last - (int64_t)dual->dirty_first) * item_size;
    void* data = dvz_array_item(array, dual->dirty_first);
    dvz_upload_dat(dual->rqr, dual->dat, offset, size, data);

    dvz_dual_clear(dual);
}



void dvz_dual_destroy(DvzDual* dual) { ANN(dual); }
