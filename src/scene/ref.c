/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Reference                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/ref.h"
#include "_cglm.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Reference                                                                                    */
/*************************************************************************************************/

DvzRef* dvz_ref(int flags)
{
    DvzRef* ref = (DvzRef*)calloc(1, sizeof(DvzRef));
    ANN(ref);

    ref->box = dvz_box(+INFINITY, -INFINITY, +INFINITY, -INFINITY, +INFINITY, -INFINITY);
    ref->flags = flags;

    return ref;
}



void dvz_ref_set(DvzRef* ref, DvzDim dim, double vmin, double vmax)
{
    ANN(ref);
    if (dim == DVZ_DIM_X)
    {
        ref->box.xmin = vmin;
        ref->box.xmax = vmax;
    }
    else if (dim == DVZ_DIM_Y)
    {
        ref->box.ymin = vmin;
        ref->box.ymax = vmax;
    }
    else if (dim == DVZ_DIM_Z)
    {
        ref->box.zmin = vmin;
        ref->box.zmax = vmax;
    }
    else
    {
        log_warn("DvzRef: invalid dimension %d. Use DVZ_DIM_X, DVZ_DIM_Y or DVZ_DIM_Z", dim);
    }
}



void dvz_ref_get(DvzRef* ref, DvzDim dim, double* vmin, double* vmax)
{
    ANN(ref);
    ANN(vmin);
    ANN(vmax);

    if (dim == DVZ_DIM_X)
    {
        *vmin = ref->box.xmin;
        *vmax = ref->box.xmax;
    }
    else if (dim == DVZ_DIM_Y)
    {
        *vmin = ref->box.ymin;
        *vmax = ref->box.ymax;
    }
    else if (dim == DVZ_DIM_Z)
    {
        *vmin = ref->box.zmin;
        *vmax = ref->box.zmax;
    }
    else
    {
        log_warn("DvzRef: invalid dimension %d. Use DVZ_DIM_X, DVZ_DIM_Y or DVZ_DIM_Z", dim);
    }
}



void dvz_ref_expand(DvzRef* ref, DvzDim dim, double vmin, double vmax)
{
    ANN(ref);
    if (dim == DVZ_DIM_X)
    {
        ref->box.xmin = fmin(ref->box.xmin, vmin);
        ref->box.xmax = fmax(ref->box.xmax, vmax);
    }
    else if (dim == DVZ_DIM_Y)
    {
        ref->box.ymin = fmin(ref->box.ymin, vmin);
        ref->box.ymax = fmax(ref->box.ymax, vmax);
    }
    else if (dim == DVZ_DIM_Z)
    {
        ref->box.zmin = fmin(ref->box.zmin, vmin);
        ref->box.zmax = fmax(ref->box.zmax, vmax);
    }
    else
    {
        log_warn("DvzRef: invalid dimension %d. Use DVZ_DIM_X, DVZ_DIM_Y or DVZ_DIM_Z", dim);
    }
}



void dvz_ref_expand2D(DvzRef* ref, uint32_t count, dvec2* pos)
{
    ANN(ref);
    ANN(pos);
    ASSERT(count > 0);

    for (uint32_t i = 0; i < count; i++)
    {
        ref->box.xmin = fmin(ref->box.xmin, pos[i][0]);
        ref->box.xmax = fmax(ref->box.xmax, pos[i][0]);

        ref->box.ymin = fmin(ref->box.ymin, pos[i][1]);
        ref->box.ymax = fmax(ref->box.ymax, pos[i][1]);
    }
}



void dvz_ref_expand3D(DvzRef* ref, uint32_t count, dvec3* pos)
{
    ANN(ref);
    ANN(pos);
    ASSERT(count > 0);

    for (uint32_t i = 0; i < count; i++)
    {
        ref->box.xmin = fmin(ref->box.xmin, pos[i][0]);
        ref->box.xmax = fmax(ref->box.xmax, pos[i][0]);

        ref->box.ymin = fmin(ref->box.ymin, pos[i][1]);
        ref->box.ymax = fmax(ref->box.ymax, pos[i][1]);

        ref->box.zmin = fmin(ref->box.zmin, pos[i][2]);
        ref->box.zmax = fmax(ref->box.zmax, pos[i][2]);
    }
}



void dvz_ref_transform1D(DvzRef* ref, DvzDim dim, uint32_t count, double* pos, vec3* pos_tr)
{
    ANN(ref);
    ANN(pos);
    ANN(pos_tr);
    ASSERT(count > 0);

    dvz_box_normalize_1D(ref->box, DVZ_BOX_NDC, dim, count, pos, pos_tr);
}



void dvz_ref_transform2D(DvzRef* ref, uint32_t count, dvec2* pos, vec3* pos_tr)
{
    ANN(ref);
    ANN(pos);
    ANN(pos_tr);
    ASSERT(count > 0);

    dvz_box_normalize_2D(ref->box, DVZ_BOX_NDC, count, pos, pos_tr);
}



void dvz_ref_transform3D(DvzRef* ref, uint32_t count, dvec3* pos, vec3* pos_tr)
{
    ANN(ref);
    ANN(pos);
    ANN(pos_tr);
    ASSERT(count > 0);

    dvz_box_normalize_3D(ref->box, DVZ_BOX_NDC, count, pos, pos_tr);
}



void dvz_ref_inverse(DvzRef* ref, vec3 pos_tr, dvec3 pos)
{
    ANN(ref);
    dvz_box_inverse(ref->box, DVZ_BOX_NDC, pos_tr, pos);
}



void dvz_ref_destroy(DvzRef* ref)
{
    ANN(ref);
    FREE(ref);
}
