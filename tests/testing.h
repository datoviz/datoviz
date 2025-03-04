/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic testing framework                                                                    */
/*************************************************************************************************/

#ifndef TST_HEADER
#define TST_HEADER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_log.h"
#include "datoviz_macros.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  MACROS                                                                                       */
/*************************************************************************************************/

#define TST_DEFAULT_CAPACITY 32

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        log_error("assertion '%s' failed", #x);                                                   \
        return 1;                                                                                 \
    }

#define AEn(n, x, y)                                                                              \
    {                                                                                             \
        for (uint32_t k = 0; k < (n); k++)                                                        \
            AT((x)[k] == (y)[k]);                                                                 \
    }

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN(((x) - (y)), -(eps), +(eps))

#define ACn(n, x, y, eps)                                                                         \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AC((x)[i], (y)[i], (eps));

#define EPS 1e-6

#define PROF_START(num)                                                                           \
    {                                                                                             \
        uint32_t N = num;                                                                         \
        DvzClock clock = dvz_clock();                                                             \
        for (uint32_t i = 0; i < N; i++)                                                          \
        {

#define PROF_END                                                                                  \
    }                                                                                             \
    double elapsed = dvz_clock_get(&clock);                                                       \
    log_info("profiling: %.6f ms per run", 1000 * elapsed / N);                                   \
    }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TST_ITEM_FLAGS_NONE = 0x0000,
    TST_ITEM_FLAGS_STANDALONE = 0xF000,
} TstItemFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TstItem TstItem;
typedef struct TstSuite TstSuite;

typedef int (*TstFunction)(TstSuite* suite, TstItem* item);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TstItem
{
    const char* name;
    const char* tags;
    TstFunction test;
    TstFunction setup;
    TstFunction teardown;
    int flags;
    void* user_data;

    int res;
};



struct TstSuite
{
    uint32_t n_items;  // number of items
    uint32_t capacity; // size of the allocated array TstSuite.items
    TstItem* items;    // array of items
    void* context;     // user-specified custom context
};



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

TstSuite tst_suite(void);

void tst_suite_add(
    TstSuite* suite, const char* name, const char* tags, //
    TstFunction test, TstFunction setup, TstFunction teardown, void* user_data, int flags);

void tst_suite_run(TstSuite* suite, const char* match);

void tst_suite_destroy(TstSuite* suite);



EXTERN_C_OFF

#endif
