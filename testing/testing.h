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
#include <stddef.h>

#include "_log.h"
#include "datoviz/common/macros.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_DEFAULT_CAPACITY 32



/*************************************************************************************************/
/*  Test assertions */
/*************************************************************************************************/

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



/*************************************************************************************************/
/*  Profiling                                                                                    */
/*************************************************************************************************/

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
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define TEST(test, tags, setup, teardown, flags)                                                  \
    tst_suite_add(suite, #test, tags, test, setup, teardown, NULL, flags);

#define TEST_SIMPLE(test) TEST(test, tags, NULL, NULL, TST_ITEM_FLAGS_NONE)

#define AT_EXPECTED_ERROR(suite, x)                                                               \
    do                                                                                            \
    {                                                                                             \
        tst_expect_error_begin((suite));                                                          \
        AT((x));                                                                                  \
        (void)tst_expect_error_end((suite));                                                      \
    } while (0)

#define AT_EXPECTED_ERROR_STRICT(suite, x)                                                        \
    do                                                                                            \
    {                                                                                             \
        tst_expect_error_begin((suite));                                                          \
        AT((x));                                                                                  \
        AT(tst_expect_error_end((suite)) == 0);                                                   \
    } while (0)



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
typedef struct TstLogRecord TstLogRecord;

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



struct TstLogRecord
{
    int level;
    int line;
    char file[64];
    char message[512];
};



struct TstSuite
{
    uint32_t n_items;                     // number of items
    uint32_t capacity;                    // size of the allocated array TstSuite.items
    TstItem* items;                       // array of items
    void* context;                        // user-specified custom context
    bool capture_logs;                    // whether log capture is enabled
    bool expect_error_active;             // whether an expected-error scope is currently active
    bool expect_error_seen;               // whether an expected error was observed in the current scope
    bool unexpected_error_seen;           // whether an unexpected error was observed in the current test
    bool suppress_expected_error_output;  // whether expected errors are hidden from console output
    bool strict_unexpected_errors;        // whether unexpected error logs should fail tests
    uint32_t captured_log_count;          // number of captured log entries
    uint32_t captured_log_capacity;       // allocated size of captured logs
    TstLogRecord* captured_logs;          // captured log entries
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

void tst_log_capture_begin(TstSuite* suite);

void tst_log_capture_end(TstSuite* suite);

uint32_t tst_log_capture_count(const TstSuite* suite);

const TstLogRecord* tst_log_capture_get(const TstSuite* suite, uint32_t index);

void tst_expect_error_begin(TstSuite* suite);

int tst_expect_error_end(TstSuite* suite);

void tst_set_strict_unexpected_errors(TstSuite* suite, bool enabled);



EXTERN_C_OFF

#endif
