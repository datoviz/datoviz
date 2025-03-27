/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing ticks                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_ticks.h"
#include "scene/ticks.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Ticks test utils                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Ticks tests                                                                                  */
/*************************************************************************************************/

int test_ticks_1(TstSuite* suite)
{
    ANN(suite);
    DvzTicks* ticks = dvz_ticks(0);
    dvz_ticks_size(ticks, 500, 10);

    dvz_ticks_compute(ticks, 0.123, 0.456, 10);
    dvz_ticks_print(ticks);

    dvz_ticks_compute(ticks, 1.23e6, 1.24e6, 10);
    dvz_ticks_print(ticks);

    dvz_ticks_compute(ticks, -12, -10.987, 5);
    dvz_ticks_print(ticks);

    dvz_ticks_compute(ticks, -12, -10.987, 5);
    dvz_ticks_print(ticks);

    dvz_ticks_clear(ticks);
    AT(dvz_ticks_compute(ticks, 0, 1, 10));
    AT(!dvz_ticks_compute(ticks, 0, 1.01, 10));
    AT(!dvz_ticks_compute(ticks, -0.01, 1.01, 10));
    AT(dvz_ticks_compute(ticks, 0, 1.1, 10));
    AT(!dvz_ticks_compute(ticks, 0.01, 1.1, 10));
    AT(dvz_ticks_compute(ticks, 0.05, 1.1, 10));

    dvz_ticks_destroy(ticks);
    return 0;
}
