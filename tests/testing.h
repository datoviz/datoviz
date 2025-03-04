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
    TST_ITEM_NONE,
    TST_ITEM_TEST,
    TST_ITEM_SETUP,
    TST_ITEM_TEARDOWN,
} TstItemType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TstSuite TstSuite;
typedef struct TstTest TstTest;
typedef struct TstFixture TstFixture;
typedef struct TstItem TstItem; // either a test or a fixture
typedef union TstItemUnion TstItemUnion;

typedef int (*TstFunction)(TstSuite* suite);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TstTest
{
    const char* name;
    TstFunction function;
    int res;
    void* user_data;
};



struct TstFixture
{
    TstFunction function;
    void* user_data;
};



union TstItemUnion
{
    TstTest t;
    TstFixture f;
};



struct TstItem
{
    // remove these, and replace by:
    // name
    // tags
    // flags
    // function
    // setup
    // teardown
    // res
    // user_data
    TstItemType type;
    TstItemUnion u;
    bool active;
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

TstItem* _append(TstSuite* suite, TstItemType type, TstFunction function, void* user_data);

void tst_suite_setup(TstSuite* suite, TstFunction setup, void* user_data);

void tst_suite_add(TstSuite* suite, const char* name, TstFunction test, void* user_data);

void tst_suite_teardown(TstSuite* suite, TstFunction teardown, void* user_data);

void tst_suite_run(TstSuite* suite, const char* match);

void tst_suite_destroy(TstSuite* suite);



EXTERN_C_OFF

#endif
