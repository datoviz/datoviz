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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/math/anim.h"
#include "test_math.h"
#include "testing.h"



/*************************************************************************************************/
/*  Anim tests                                                                                   */
/*************************************************************************************************/

int test_anim_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    double t = 0;
    for (int i = 0; i < (int)DVZ_EASING_COUNT; i++)
    {
        AC(dvz_easing((DvzEasing)i, t), 0, EPSILON);
        AC(dvz_easing((DvzEasing)i, 1), 1, EPSILON);
    }
    return 0;
}
