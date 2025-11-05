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
#include <string>
#include <vector>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "testing.h"



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
    printf("%50s", name ? name : "");
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}

static void print_res_begin(int index, const char* name)
{
    log_debug("starting test #%03d %s", index, name ? name : "");
    printf("%50s...", name);
}

static void print_res_end(int index, const char* name, int res)
{
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}



static void print_end(int index, int res)
{
    printf("--------------------------------------------------\n");
    if (index > 0 && res == 0)
        printf("\x1b[32m%d/%d tests PASSED.\x1b[0m\n", index, index);
    else if (index > 0)
        printf("\x1b[31m%d/%d tests FAILED.\x1b[0m\n", res, index);
    else
        printf("\x1b[31mThere were no tests.\x1b[0m\n");
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
            int res = item->test(suite, item);
            print_res_end(index, item->name, res);
            total_res += (res == 0 ? 0 : 1);
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
        int res = item->test(suite, item);
        print_res_end(index, item->name, res);
        total_res += (res == 0 ? 0 : 1);
        ++index;

        if (item->teardown != NULL)
        {
            item->teardown(suite, item);
        }
    }

    // TODO: mark as PASS or FAIL depending on the res
    print_end(index, total_res);
}



void tst_suite_destroy(TstSuite* suite)
{
    log_trace("destroy testing suite");
    ANN(suite);
    ANN(suite->items);
    suite->n_items = 0;
    suite->capacity = 0;
    dvz_free_ptr((void**)&suite->items);
}
