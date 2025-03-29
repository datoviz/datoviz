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
#define REQUESTED_TICK_COUNT 8



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/

static void
_print_ticks(DvzTicksSpec* spec, uint32_t count, double lmin, double lmax, double lstep)
{
    double tick_pos[MAX_TICKS] = {0};
    char* labels[MAX_TICKS];
    for (uint32_t i = 0; i < count; i++)
        labels[i] = calloc(LABEL_LEN, sizeof(char));

    dvz_ticks_linspace(spec, count, lmin, lmax, lstep, labels, tick_pos);

    printf("=== Test : [%g, %g], precision = %d ===\n", lmin, lmax, spec->precision);

    printf("Labels:    ");
    for (uint32_t i = 0; i < count; i++)
        printf("%s  ", labels[i]);
    if (spec->exponent != 0)
        printf("1e%s%d", spec->exponent > 0 ? "+" : "", spec->exponent);
    if (spec->offset != 0)
        printf("%s%g", spec->offset > 0 ? "+" : "", spec->offset);
    printf("\n");

    printf("Positions: ");
    for (uint32_t i = 0; i < count; i++)
        printf("%g  ", tick_pos[i]);
    printf("\n");
    printf("\n\n");

    for (uint32_t i = 0; i < count; i++)
        FREE(labels[i]);
}



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

    dvz_ticks_compute(ticks, 1234, 1234.001, 10);
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



int test_ticks_labels(TstSuite* suite)
{
    ANN(suite);

    DvzTicksSpec spec = {
        .precision = 2,
        .exponent = 0,
        .offset = 0,
    };

    spec.format = DVZ_TICKS_FORMAT_DECIMAL;
    _print_ticks(&spec, 5, 10.0, 50.0, 10.0);

    spec.format = DVZ_TICKS_FORMAT_DECIMAL_FACTORED;
    spec.exponent = 1;
    spec.offset = 1000;
    _print_ticks(&spec, 5, 1010.0, 1050.0, 10.0);

    spec.format = DVZ_TICKS_FORMAT_SCIENTIFIC;
    spec.exponent = 0;
    spec.offset = 0;
    _print_ticks(&spec, 4, 1e-5, 4e-5, 1e-5);

    spec.format = DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED;
    spec.exponent = -5;
    spec.offset = 1e-5;
    _print_ticks(&spec, 4, 1.01e-5, 1.04e-5, 1e-7);

    // _print_ticks(DVZ_TICKS_FORMAT_THOUSANDS, 4, 0, 3000, 1000, 1);
    // _print_ticks(DVZ_TICKS_FORMAT_THOUSANDS_FACTORED, 4, 2000, 5000, 1000, 1);
    // _print_ticks(DVZ_TICKS_FORMAT_MILLIONS, 3, 0, 2e6, 1e6, 1);
    // _print_ticks(DVZ_TICKS_FORMAT_MILLIONS_FACTORED, 3, 3e6, 5e6, 1e6, 2);

    return 0;
}



static void _test_ticks(DvzTicks* ticks, double dmin, double dmax)
{
    ANN(ticks);
    ASSERT(dmin < dmax);

    dvz_ticks_clear(ticks);
    dvz_ticks_compute(ticks, dmin, dmax, REQUESTED_TICK_COUNT);
    uint32_t tick_count = get_tick_count(ticks->lmin, ticks->lmax, ticks->lstep);
    _print_ticks(&ticks->spec, tick_count, ticks->lmin, ticks->lmax, ticks->lstep);
}

int test_ticks_2(TstSuite* suite)
{
    ANN(suite);
    DvzTicks* ticks = dvz_ticks(0);
    dvz_ticks_size(ticks, 500.0, 20.0);

    _test_ticks(ticks, 0, 1);
    _test_ticks(ticks, -1, 1);
    _test_ticks(ticks, 0, 10);
    _test_ticks(ticks, .1, .2);
    _test_ticks(ticks, 1001, 1002);
    for (int32_t i = -9; i <= 9; i++)
    {
        _test_ticks(ticks, -pow((double)10, i), +pow((double)10, i));
    }

    _test_ticks(ticks, 1e3 + .123, 1e3 + .124);
    _test_ticks(ticks, 1.234e8 + .123, 1.234e8 + .1230001);
    _test_ticks(ticks, -2e+07, -1.8e+07);
    _test_ticks(ticks, -2e-07, -1.8e-07);

    dvz_ticks_destroy(ticks);
    return 0;
}
