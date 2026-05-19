/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing time utilities                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_time_utils.h"

#include "test_common.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Verify monotonic timestamps are non-decreasing and advance across a short sleep.
 *
 * @param suite test suite context
 * @param tstitem test item context
 * @return zero on success
 */
int test_time_monotonic_ns(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    uint64_t t0 = dvz_time_monotonic_ns();
    AT(t0 > 0);

    uint64_t t1 = t0;
    for (uint32_t i = 0; i < 32; i++)
    {
        uint64_t current = dvz_time_monotonic_ns();
        AT(current >= t1);
        t1 = current;
    }

    dvz_sleep_us(1000);

    uint64_t t2 = dvz_time_monotonic_ns();
    AT(t2 >= t1);
    uint64_t elapsed_ns = t2 - t1;
    AT(elapsed_ns >= 100000ULL);
    AT(elapsed_ns < 1000000000ULL);
    return 0;
}
