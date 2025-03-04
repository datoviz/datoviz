/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing mvp                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_mvp.h"
#include "_cglm.h"
#include "scene/mvp.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Mvp tests                                                                                 */
/*************************************************************************************************/

int test_mvp_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    vec4 p = {1, 0, 0, 1};
    vec4 out = {0};

    dvz_mvp_apply(&mvp, p, out);
    AT(glm_vec4_eqv(out, p));

    mvp.view[0][0] = 2;
    mvp.view[1][1] = 2;
    dvz_mvp_apply(&mvp, p, out);
    AT(glm_vec4_eqv(out, (vec4){2, 0, 0, 1}));

    mvp.proj[0][0] = .5;
    mvp.proj[1][1] = .5;
    dvz_mvp_apply(&mvp, p, out);
    AT(glm_vec4_eqv(out, p));

    return 0;
}
