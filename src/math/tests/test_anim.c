/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing anim                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_assertions.h"
#include "datoviz/math/anim.h"
#include "test_math.h"
#include "testing.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Anim tests                                                                                   */
/*************************************************************************************************/

int test_anim_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    double t = 0;
    for (int i = 0; i < (int)DVZ_EASING_COUNT; i++)
    {
        AC(dvz_easing((DvzEasing)i, t), 0, DVZ_EPSILON);
        AC(dvz_easing((DvzEasing)i, 1), 1, DVZ_EPSILON);
    }
    AC(dvz_easing(DVZ_EASING_IN_SINE, 0.5), 1.0 - sqrt(0.5), DVZ_EPSILON);
    AC(dvz_easing(DVZ_EASING_OUT_SINE, 0.5), sqrt(0.5), DVZ_EPSILON);
    return 0;
}
