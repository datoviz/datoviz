/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/transform.h"
#include "common.h"
#include "datoviz_protocol.h"
#include "scene/array.h"
#include "scene/dual.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/

DvzTransform* dvz_transform(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzTransform* tr = (DvzTransform*)calloc(1, sizeof(DvzTransform));
    tr->flags = flags;

    // TODO: chaining of transforms, in which case there should really be only one dual.

    // NOTE: the transform holds the DvzMVP dual.
    log_trace("create transform dual");
    tr->dual = dvz_dual_dat(batch, sizeof(DvzMVP), DVZ_DAT_FLAGS_MAPPABLE);
    dvz_batch_desc(batch, "MVP");

    // Initialize the MVP on initialization.
    DvzMVP mvp = dvz_mvp_default();
    dvz_transform_set(tr, &mvp);
    dvz_transform_update(tr);

    return tr;
}



void dvz_transform_set(DvzTransform* tr, DvzMVP* mvp)
{
    // This function sets the DvzMVP structure on the CPU.

    ANN(tr);

    // NOTE: this is safe, &mvp is immediately copied internally in the dual.
    dvz_dual_data(&tr->dual, 0, 1, mvp);
}



void dvz_transform_update(DvzTransform* tr)
{
    // This function emits a dat upload request with the new DvzMVP value.

    ANN(tr);

    // Force the dual update by marking it as dirty.
    dvz_dual_dirty(&tr->dual, 0, 1);

    // Emit the dat upload request.
    dvz_dual_update(&tr->dual);
}



void dvz_transform_next(DvzTransform* tr, DvzTransform* next)
{
    ANN(tr);
    ANN(next);
    tr->next = next;
}



DvzMVP* dvz_transform_mvp(DvzTransform* tr)
{
    ANN(tr);
    ANN(tr->dual.array);

    DvzMVP* mvp = (DvzMVP*)dvz_array_item(tr->dual.array, 0);
    ANN(mvp);

    return mvp;
}



void dvz_transform_destroy(DvzTransform* tr)
{
    ANN(tr);
    log_trace("destroy transform");
    if (tr->dual.array != NULL)
        dvz_dual_destroy(&tr->dual);
    FREE(tr);
}
