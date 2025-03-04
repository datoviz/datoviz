/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing PRNG                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_prng.h"
#include "_prng.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_prng_1(TstSuite* suite, TstItem* tstitem)
{
    DvzPrng* prng = dvz_prng();
    uint64_t uuid = dvz_prng_uuid(prng);
    log_info("random uuid is %" PRIu64, uuid);
    dvz_prng_destroy(prng);
    return 0;
}
