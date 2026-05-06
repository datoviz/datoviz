/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic testing framework                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define TST_LOG_CAPTURE_DEFAULT_CAPACITY 16
#define TST_RESULT_NAME_WIDTH           68

static void _tst_log_capture_reset(TstSuite* suite)
{
    ANN(suite);
    suite->captured_log_count = 0;
}



static void _tst_log_capture_append(
    TstSuite* suite, int level, const char* file, int line, const char* message)
{
    ANN(suite);
    if (!suite->capture_logs)
    {
        return;
    }

    if (suite->captured_log_capacity == 0)
    {
        suite->captured_logs =
            (TstLogRecord*)dvz_calloc(TST_LOG_CAPTURE_DEFAULT_CAPACITY, sizeof(TstLogRecord));
        ANN(suite->captured_logs);
        suite->captured_log_capacity = TST_LOG_CAPTURE_DEFAULT_CAPACITY;
    }
    else if (suite->captured_log_count == suite->captured_log_capacity)
    {
        uint32_t new_capacity = 2 * suite->captured_log_capacity;
        suite->captured_logs = (TstLogRecord*)dvz_realloc(
            suite->captured_logs, (size_t)(new_capacity * sizeof(TstLogRecord)));
        ANN(suite->captured_logs);
        suite->captured_log_capacity = new_capacity;
    }

    ASSERT(suite->captured_log_count < suite->captured_log_capacity);
    TstLogRecord* rec = &suite->captured_logs[suite->captured_log_count++];
    ANN(rec);
    dvz_memset(rec, sizeof(*rec), 0, sizeof(*rec));
    rec->level = level;
    rec->line = line;
    dvz_snprintf(rec->file, sizeof(rec->file), "%s", file ? file : "");
    dvz_snprintf(rec->message, sizeof(rec->message), "%s", message ? message : "");
}



static int _tst_log_intercept(
    void* udata, int level, const char* file, int line, const char* message)
{
    TstSuite* suite = (TstSuite*)udata;
    if (suite == NULL)
    {
        return 0;
    }

    _tst_log_capture_append(suite, level, file, line, message);

    if (level >= LOG_ERROR)
    {
        if (suite->expect_error_active)
        {
            suite->expect_error_seen = true;
            return suite->suppress_expected_error_output ? 1 : 0;
        }
        if (suite->strict_unexpected_errors)
        {
            suite->unexpected_error_seen = true;
        }
    }
    return 0;
}



static void _tst_item_begin(TstSuite* suite)
{
    ANN(suite);
    suite->expect_error_active = false;
    suite->expect_error_seen = false;
    suite->unexpected_error_seen = false;
    if (suite->capture_logs)
    {
        _tst_log_capture_reset(suite);
    }
}



static int _tst_item_finalize(TstSuite* suite, int res)
{
    ANN(suite);
    if (suite->expect_error_active)
    {
        fprintf(stderr, "expected-error scope left open at end of test\n");
        suite->expect_error_active = false;
        res = 1;
    }
    if (suite->strict_unexpected_errors && suite->unexpected_error_seen)
    {
        fprintf(stderr, "unexpected error log emitted during test\n");
        res = 1;
    }
    return res;
}



/*************************************************************************************************/
/*  Test printing                                                                                */
/*************************************************************************************************/

static void print_start(void)
{
    printf("--- Starting tests -------------------------------\n"); //
}



static void print_test(int index, const char* name)
{
    printf("- Running test #%03d %28s\n", index, name ? name : "");
}



static void print_res(int index, const char* name, int res)
{
    printf("%*s", TST_RESULT_NAME_WIDTH, name ? name : "");
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}

static void print_res_begin(int index, const char* name)
{
    log_debug("starting test #%03d %s", index, name ? name : "");
    printf("%*s...", TST_RESULT_NAME_WIDTH, name);
}

static void print_res_end(int index, const char* name, int res)
{
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}



static void print_end(int index, int res, const char** failed_tests, uint32_t failed_count)
{
    printf("--------------------------------------------------\n");
    if (index > 0 && res == 0)
        printf("\x1b[32m%d/%d tests PASSED.\x1b[0m\n", index, index);
    else if (index > 0)
        printf("\x1b[31m%d/%d tests FAILED.\x1b[0m\n", res, index);
    else
        printf("\x1b[31mThere were no tests.\x1b[0m\n");

    if (res > 0 && failed_tests != NULL && failed_count > 0)
    {
        printf("\x1b[31mFailed tests:\x1b[0m\n");
        for (uint32_t i = 0; i < failed_count; ++i)
        {
            const char* name = failed_tests[i];
            printf("  - %s\n", name != NULL ? name : "(unnamed)");
        }
    }
}



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

TstSuite tst_suite(void)
{
    TstSuite suite = {};
    suite.items = (TstItem*)dvz_calloc(TST_DEFAULT_CAPACITY, sizeof(TstItem));
    ANN(suite.items);
    suite.capacity = TST_DEFAULT_CAPACITY;
    suite.n_items = 0;
    suite.capture_logs = false;
    suite.expect_error_active = false;
    suite.expect_error_seen = false;
    suite.unexpected_error_seen = false;
    suite.suppress_expected_error_output = true;
    suite.strict_unexpected_errors = false;
    suite.captured_log_count = 0;
    suite.captured_log_capacity = 0;
    suite.captured_logs = NULL;
    return suite;
}



void tst_suite_add(
    TstSuite* suite, const char* name, const char* tags, //
    TstFunction test, TstFunction setup, TstFunction teardown, void* user_data, int flags)
{
    ANN(suite);
    ANN(name);
    // log_trace(
    //     "append one test item to suite with %d items, capacity %d", //
    //     suite->n_items, suite->capacity);
    // Resize the array if needed.
    if (suite->capacity == suite->n_items)
    {
        log_trace("reallocate memory for test suite items");
        ANN(suite->items);
        ASSERT(suite->n_items > 0);
        suite->items =
            (TstItem*)dvz_realloc(suite->items, (size_t)(2 * suite->n_items * sizeof(TstItem)));
        ANN(suite->items);
        suite->capacity *= 2;
    }
    ASSERT(suite->n_items < suite->capacity);
    TstItem* item = &suite->items[suite->n_items++];
    ANN(item);

    item->name = name;
    item->tags = tags;
    item->test = test;
    item->setup = setup;
    item->teardown = teardown;
    item->flags = flags;
    item->user_data = user_data;
}



void tst_suite_run(TstSuite* suite, const char* match)
{
    log_trace("running testing suite");
    ANN(suite);
    ANN(suite->items);

    log_set_intercept(_tst_log_intercept, suite);
    print_start();

    struct TstGroupedItems
    {
        TstFunction setup;
        TstFunction teardown;
        int flags;
        std::vector<TstItem*> items;
    };

    std::vector<TstGroupedItems> grouped_tests;
    std::vector<TstItem*> standalone_tests;

    // First step: Collect matching test items and group them
    for (uint32_t i = 0; i < suite->n_items; ++i)
    {
        TstItem* item = &suite->items[i];
        if (!match || (item->name && std::string(item->name).find(match) != std::string::npos) ||
            (item->tags && std::string(item->tags).find(match) != std::string::npos))
        {
            if (item->flags & TST_ITEM_FLAGS_STANDALONE)
            {
                standalone_tests.push_back(item);
            }
            else
            {
                TstGroupedItems* bucket = NULL;
                for (auto& group : grouped_tests)
                {
                    if (group.setup == item->setup && group.teardown == item->teardown &&
                        group.flags == item->flags)
                    {
                        bucket = &group;
                        break;
                    }
                }
                if (bucket == NULL)
                {
                    grouped_tests.push_back(
                        {item->setup, item->teardown, item->flags, std::vector<TstItem*>()});
                    bucket = &grouped_tests.back();
                }
                bucket->items.push_back(item);
            }
        }
    }

    int total_res = 0;
    int index = 0;
    std::vector<const char*> failed_tests;

    // Second step: Execute grouped tests
    for (auto& group : grouped_tests)
    {
        TstFunction setup = group.setup;
        TstFunction teardown = group.teardown;

        // Setup.
        if (setup != NULL)
        {
            setup(suite, group.items.front());
        }

        // All shared tests for that setup.
        for (TstItem* item : group.items)
        {
            print_res_begin(index, item->name);
            _tst_item_begin(suite);
            int res = item->test(suite, item);
            res = _tst_item_finalize(suite, res);
            print_res_end(index, item->name, res);
            total_res += (res == 0 ? 0 : 1);
            if (res != 0)
            {
                failed_tests.push_back(item->name);
            }
            ++index;
        }

        // Teardown
        if (teardown != NULL)
        {
            teardown(suite, group.items.front());
        }
    }

    // Third step: Execute standalone tests individually
    for (TstItem* item : standalone_tests)
    {
        if (item->setup != NULL)
        {
            item->setup(suite, item);
        }

        print_res_begin(index, item->name);
        _tst_item_begin(suite);
        int res = item->test(suite, item);
        res = _tst_item_finalize(suite, res);
        print_res_end(index, item->name, res);
        total_res += (res == 0 ? 0 : 1);
        if (res != 0)
        {
            failed_tests.push_back(item->name);
        }
        ++index;

        if (item->teardown != NULL)
        {
            item->teardown(suite, item);
        }
    }

    // TODO: mark as PASS or FAIL depending on the res
    print_end(index, total_res, failed_tests.data(), (uint32_t)failed_tests.size());
    log_set_intercept(NULL, NULL);
}



/**
 * Enable log capture for the current suite.
 *
 * @param suite test suite
 * @return void this function does not return a value
 */
void tst_log_capture_begin(TstSuite* suite)
{
    ANN(suite);
    suite->capture_logs = true;
    _tst_log_capture_reset(suite);
}



/**
 * Disable log capture for the current suite.
 *
 * @param suite test suite
 * @return void this function does not return a value
 */
void tst_log_capture_end(TstSuite* suite)
{
    ANN(suite);
    suite->capture_logs = false;
}



/**
 * Return the number of captured log records.
 *
 * @param suite test suite
 * @return number of captured logs
 */
uint32_t tst_log_capture_count(const TstSuite* suite)
{
    ANN(suite);
    return suite->captured_log_count;
}



/**
 * Return a captured log record by index.
 *
 * @param suite test suite
 * @param index zero-based captured log index
 * @return captured log record pointer, or NULL when out of range
 */
const TstLogRecord* tst_log_capture_get(const TstSuite* suite, uint32_t index)
{
    ANN(suite);
    if (index >= suite->captured_log_count)
    {
        return NULL;
    }
    return &suite->captured_logs[index];
}



/**
 * Start an expected-error scope for the current test.
 *
 * @param suite test suite
 * @return void this function does not return a value
 */
void tst_expect_error_begin(TstSuite* suite)
{
    ANN(suite);
    suite->expect_error_active = true;
    suite->expect_error_seen = false;
}



/**
 * End an expected-error scope and validate that an error was observed.
 *
 * @param suite test suite
 * @return 0 when an expected error was seen, 1 otherwise
 */
int tst_expect_error_end(TstSuite* suite)
{
    ANN(suite);
    if (!suite->expect_error_active)
    {
        return 1;
    }
    suite->expect_error_active = false;
    return suite->expect_error_seen ? 0 : 1;
}



/**
 * Enable or disable strict failure on unexpected error logs.
 *
 * @param suite test suite
 * @param enabled whether strict mode is enabled
 * @return void this function does not return a value
 */
void tst_set_strict_unexpected_errors(TstSuite* suite, bool enabled)
{
    ANN(suite);
    suite->strict_unexpected_errors = enabled;
}



void tst_suite_destroy(TstSuite* suite)
{
    log_trace("destroy testing suite");
    ANN(suite);
    ANN(suite->items);
    suite->n_items = 0;
    suite->capacity = 0;
    suite->capture_logs = false;
    suite->expect_error_active = false;
    suite->expect_error_seen = false;
    suite->unexpected_error_seen = false;
    suite->captured_log_count = 0;
    suite->captured_log_capacity = 0;
    dvz_free_ptr((void**)&suite->items);
    dvz_free_ptr((void**)&suite->captured_logs);
}
