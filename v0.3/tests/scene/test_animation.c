/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing animation                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_animation.h"
#include "app.h"
#include "datoviz.h"
#include "presenter.h"
#include "renderer.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EPSILON 1e-10



/*************************************************************************************************/
/*  Animation tests                                                                              */
/*************************************************************************************************/

int test_animation_1(TstSuite* suite, TstItem* tstitem)
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
