/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing ref                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_ref.h"
#include "scene/ref.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Ref tests                                                                                   */
/*************************************************************************************************/

int test_ref_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzRef* ref = dvz_ref(0);

    DvzDim dim = 0;
    double vmin = -1;
    double vmax = +1;
    uint32_t count = 3;

    dvec2* pos2D = (dvec2*)calloc(count, sizeof(dvec2));
    dvec3* pos3D = (dvec3*)calloc(count, sizeof(dvec3));
    vec3* pos_tr = (vec3*)calloc(count, sizeof(vec3));

    dvz_ref_set(ref, dim, vmin, vmax);
    dvz_ref_expand(ref, dim, 0, 2);

    dvz_ref_get(ref, dim, &vmin, &vmax);
    AT(vmin == -1);
    AT(vmax == +2);


    // Test expand2D.
    pos2D[0][0] = -2;
    pos2D[2][0] = +4;

    dvz_ref_expand_2D(ref, count, pos2D);
    dvz_ref_get(ref, dim, &vmin, &vmax);
    AT(vmin == -2);
    AT(vmax == +4);

    // [-2, +4] into [-1, +1]
    dvz_ref_normalize_2D(ref, count, pos2D, pos_tr);
    AC(pos_tr[0][dim], -1, EPS);
    AC(pos_tr[2][dim], +1, EPS);


    // Test expand3D.
    dim = DVZ_DIM_Z;
    pos3D[0][dim] = -20;
    pos3D[1][dim] = +10;
    pos3D[2][dim] = +40;

    dvz_ref_expand_3D(ref, count, pos3D);
    dvz_ref_get(ref, dim, &vmin, &vmax);
    AT(vmin == -20);
    AT(vmax == +40);

    // [-20, +40] into [-1, +1]
    dvz_ref_normalize_3D(ref, count, pos3D, pos_tr);
    AC(pos_tr[0][dim], -1, EPS);
    AC(pos_tr[1][dim], 0, EPS);
    AC(pos_tr[2][dim], +1, EPS);


    dvz_ref_destroy(ref);

    FREE(pos2D);
    FREE(pos3D);
    FREE(pos_tr);

    return 0;
}
