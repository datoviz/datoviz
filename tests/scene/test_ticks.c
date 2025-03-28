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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAX_TICKS            32
#define LABEL_LEN            64
#define REQUESTED_TICK_COUNT 10



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



static void _test_ticks_case(
    DvzTicksFormat format, uint32_t count, double lmin, double lmax, double lstep,
    uint32_t precision)
{
    DvzTicksSpec spec = {
        .format = format,
        .precision = precision,
        .exponent = 0,
        .offset = 0,
    };

    double tick_pos[MAX_TICKS] = {0};
    char* labels[MAX_TICKS];
    for (uint32_t i = 0; i < count; i++)
        labels[i] = calloc(LABEL_LEN, sizeof(char));

    int32_t exponent = 0;
    double offset = 0;

    dvz_ticks_linspace(&spec, count, lmin, lmax, lstep, labels, tick_pos, &exponent, &offset);

    printf("=== Test : [%.3f, %.3f], precision = %d ===\n", lmin, lmax, precision);
    printf("Offset: %.6f, Exponent: %d\n", offset, exponent);
    for (uint32_t i = 0; i < count; i++)
        printf("  Tick %2u: pos=%g, label='%s'\n", i, tick_pos[i], labels[i]);
    printf("\n");

    for (uint32_t i = 0; i < count; i++)
        FREE(labels[i]);
}

int test_ticks_labels(TstSuite* suite)
{
    ANN(suite);

    _test_ticks_case(DVZ_TICKS_FORMAT_DECIMAL, 5, 10.0, 50.0, 10.0, 2);
    _test_ticks_case(DVZ_TICKS_FORMAT_DECIMAL_FACTORED, 5, 1010.0, 1050.0, 10.0, 2);
    _test_ticks_case(DVZ_TICKS_FORMAT_SCIENTIFIC, 4, 1e-5, 4e-5, 1e-5, 2);
    _test_ticks_case(DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED, 4, 1.01e-5, 1.04e-5, 1e-7, 2);

    // _test_ticks_case(DVZ_TICKS_FORMAT_THOUSANDS, 4, 0, 3000, 1000, 1);
    // _test_ticks_case(DVZ_TICKS_FORMAT_THOUSANDS_FACTORED, 4, 2000, 5000, 1000, 1);
    // _test_ticks_case(DVZ_TICKS_FORMAT_MILLIONS, 3, 0, 2e6, 1e6, 1);
    // _test_ticks_case(DVZ_TICKS_FORMAT_MILLIONS_FACTORED, 3, 3e6, 5e6, 1e6, 2);

    return 0;
}



static void _test_ticks(DvzTicks* ticks, double dmin, double dmax)
{
    ANN(ticks);
    ASSERT(dmin < dmax);

    dvz_ticks_compute(ticks, dmin, dmax, REQUESTED_TICK_COUNT);

    uint32_t tick_count = get_tick_count(ticks->lmin, ticks->lmax, ticks->lstep);

    _test_ticks_case(
        ticks->format, tick_count, ticks->lmin, ticks->lmax, ticks->lstep, ticks->precision);
}

int test_ticks_2(TstSuite* suite)
{
    ANN(suite);
    DvzTicks* ticks = dvz_ticks(0);
    dvz_ticks_size(ticks, 500.0, 20.0);

    _test_ticks(ticks, 0, 1);

    dvz_ticks_destroy(ticks);
    return 0;
}
