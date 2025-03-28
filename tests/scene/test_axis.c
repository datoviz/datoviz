/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing axis                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axis.h"
#include "scene/axis.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Axis tests                                                                                   */
/*************************************************************************************************/

int test_axis_1(TstSuite* suite)
{
    ANN(suite);

    DvzRef* ref = dvz_ref(0);

    DvzDim dim = 0;
    double vmin = -1;
    double vmax = +1;
    uint32_t count = 3;



    dvz_ref_destroy(ref);

    return 0;
}
