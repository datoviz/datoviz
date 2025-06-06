/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Dual                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/dual.h"
#include "_map.h"
#include "datoviz_protocol.h"
#include "scene/array.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzDual dvz_dual(DvzBatch* batch, DvzArray* array, DvzId dat)
{
    ANN(batch);
    ANN(array);
    ASSERT(dat != DVZ_ID_NONE);

    DvzDual dual = {0};
    dual.batch = batch;
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
    // NOTE: the passed data buffer is immediately copied to the array.

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



void dvz_dual_column(
    DvzDual* dual, DvzSize offset, DvzSize col_size, uint32_t first, uint32_t count,
    uint32_t repeats, void* data)
{
    ANN(dual);
    ANN(dual->array);
    ASSERT(col_size > 0);
    ASSERT(count > 0);
    ASSERT(repeats >= 1);
    ANN(data);

    DvzArray* array = dual->array;

    dvz_array_column(
        array, offset, col_size, first, repeats * count, count, data, //
        DVZ_DTYPE_CUSTOM, DVZ_DTYPE_CUSTOM, DVZ_ARRAY_COPY_REPEAT, repeats);

    dvz_dual_dirty(dual, first, repeats * count);
}



void dvz_dual_resize(DvzDual* dual, uint32_t count)
{
    ANN(dual);
    ANN(dual->array);
    ASSERT(count > 0);

    // NOTE: for now, we do not make use of count
    dvz_dual_clear(dual);

    // Send a dat update command.
    dvz_resize_dat(dual->batch, dual->dat, count * dual->array->item_size);
}



void dvz_dual_update(DvzDual* dual)
{
    ANN(dual);
    ANN(dual->batch);
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
    dvz_upload_dat(dual->batch, dual->dat, offset, size, data, 0);

    dvz_dual_clear(dual);
}



void dvz_dual_destroy(DvzDual* dual)
{
    ANN(dual);
    if (dual->need_destroy)
    {
        log_trace("automatically destroying dual's array");
        dvz_array_destroy(dual->array);
        // TODO: destruction of associated dat?
    }
}



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

DvzDual dvz_dual_vertex(DvzBatch* batch, uint32_t vertex_count, DvzSize vertex_size, int flags)
{
    ANN(batch);
    ASSERT(vertex_count > 0);
    ASSERT(vertex_size > 0);

    DvzRequest req =
        dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, vertex_count * vertex_size, flags);
    dvz_batch_desc(batch, "vertex");
    DvzId dat_id = req.id;
    DvzArray* array = dvz_array_struct(vertex_count, vertex_size);

    DvzDual dual = dvz_dual(batch, array, dat_id);
    // dual.need_destroy = true;
    return dual;
}



DvzDual dvz_dual_index(DvzBatch* batch, uint32_t index_count, int flags)
{
    ANN(batch);
    ASSERT(index_count > 0);

    DvzSize index_size = sizeof(DvzIndex);
    DvzRequest req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_INDEX, index_count * index_size, flags);
    dvz_batch_desc(batch, "index");
    DvzId dat_id = req.id;
    DvzArray* array = dvz_array_struct(index_count, index_size);

    DvzDual dual = dvz_dual(batch, array, dat_id);
    // dual.need_destroy = true;
    return dual;
}



DvzDual dvz_dual_indirect(DvzBatch* batch, bool indexed)
{
    ANN(batch);

    DvzSize size =
        indexed ? sizeof(DvzDrawIndexedIndirectCommand) : sizeof(DvzDrawIndirectCommand);
    DvzRequest req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_INDIRECT, size, 0);
    dvz_batch_desc(batch, "indirect");
    DvzId dat_id = req.id;
    DvzArray* array = dvz_array_struct(1, size);

    DvzDual dual = dvz_dual(batch, array, dat_id);
    // dual.need_destroy = true;
    return dual;
}



DvzDual dvz_dual_dat(DvzBatch* batch, DvzSize vertex_size, int flags)
{
    ANN(batch);
    ASSERT(vertex_size > 0);

    // NOTE: typically, we create a dat for a uniform buffer with just a single item of a given
    // struct.
    const uint32_t n_items = 1;

    DvzRequest req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, n_items * vertex_size, flags);
    dvz_batch_desc(batch, "uniform");
    DvzId dat_id = req.id;
    DvzArray* array = dvz_array_struct(n_items, vertex_size);

    DvzDual dual = dvz_dual(batch, array, dat_id);
    // dual.need_destroy = true;
    return dual;
}
