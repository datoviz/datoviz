/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Params                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/params.h"
#include "_map.h"
#include "request.h"
#include "scene/array.h"
#include "scene/dual.h"



/*************************************************************************************************/
/*  Params functions                                                                             */
/*************************************************************************************************/

DvzParams* dvz_params(DvzBatch* batch, DvzSize struct_size, bool is_shared)
{
    ANN(batch);
    ASSERT(struct_size > 0);

    DvzParams* params = (DvzParams*)calloc(1, sizeof(DvzParams));
    params->batch = batch;
    params->is_shared = is_shared;

    // Create the dual for the uniform with the params.
    params->dual = dvz_dual_dat(batch, struct_size, DVZ_DAT_FLAGS_MAPPABLE);
    dvz_batch_desc(batch, "params");

    return params;
}


void dvz_params_attr(DvzParams* params, uint32_t idx, DvzSize offset, DvzSize item_size)
{
    ANN(params);
    ASSERT(idx < DVZ_PARAMS_MAX_ATTRS);

    params->attrs[idx].attr_idx = idx;
    params->attrs[idx].offset = offset;
    params->attrs[idx].item_size = item_size;
}



void dvz_params_data(DvzParams* params, void* data)
{
    ANN(params);

    dvz_dual_data(&params->dual, 0, 1, data);
    dvz_dual_update(&params->dual);
}



void dvz_params_set(DvzParams* params, uint32_t idx, void* item)
{
    ANN(params);
    ASSERT(idx < DVZ_PARAMS_MAX_ATTRS);

    DvzSize offset = params->attrs[idx].offset;
    DvzSize item_size = params->attrs[idx].item_size;

    dvz_dual_column(&params->dual, offset, item_size, 0, 1, 1, item);
    // NOTE: call it manually?
    //  dvz_dual_update(&params->dual);
}



void* dvz_params_get(DvzParams* params, uint32_t idx)
{
    ANN(params);
    ASSERT(idx < DVZ_PARAMS_MAX_ATTRS);

    DvzSize offset = params->attrs[idx].offset;
    DvzSize item_size = params->attrs[idx].item_size;

    return (void*)((uint64_t)params->dual.array->data + (uint64_t)offset);
}



void dvz_params_bind(DvzParams* params, DvzId graphics_id, uint32_t slot_idx)
{
    ANN(params);
    ASSERT(params->dual.dat != DVZ_ID_NONE);

    dvz_bind_dat(params->batch, graphics_id, slot_idx, params->dual.dat, 0);
}



void dvz_params_update(DvzParams* params)
{
    ANN(params);
    dvz_dual_update(&params->dual);
}



void dvz_params_destroy(DvzParams* params)
{
    ANN(params);
    // TODO: destroy the dat
    FREE(params);
}
