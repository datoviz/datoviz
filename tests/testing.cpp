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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
    printf("- Running test #%03d %28s\n", index, name);
}



static void print_res(int index, const char* name, int res)
{
    printf("%50s", name);
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}

static void print_res_begin(int index, const char* name) { printf("%50s...", name); }

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
/*  Util functions                                                                               */
/*************************************************************************************************/

static inline bool test_name_matches(TstTest* test, const char* str)
{
    ANN(test);
    ANN(str);
    return strstr(test->name, str) != NULL;
}



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

TstSuite tst_suite(void)
{
    TstSuite suite = {};
    suite.items = (TstItem*)calloc(TST_DEFAULT_CAPACITY, sizeof(TstItem));
    suite.capacity = TST_DEFAULT_CAPACITY;
    suite.n_items = 0;
    return suite;
}



TstItem* _append(TstSuite* suite, TstItemType type, TstFunction function, void* user_data)
{
    ANN(suite);
    // log_trace(
    //     "append one test item to suite with %d items, capacity %d", //
    //     suite->n_items, suite->capacity);
    // Resize the array if needed.
    if (suite->capacity == suite->n_items)
    {
        log_trace("reallocate memory for test suite items");
        ANN(suite->items);
        ASSERT(suite->n_items > 0);
        REALLOC(TstItem*, suite->items, (size_t)(2 * suite->n_items * sizeof(TstItem)));
        suite->capacity *= 2;
    }
    ASSERT(suite->n_items < suite->capacity);
    TstItem* item = &suite->items[suite->n_items++];
    item->type = type;
    item->active = false;
    switch (type)
    {
    case TST_ITEM_SETUP:
    case TST_ITEM_TEARDOWN:
        item->u.f.function = function;
        item->u.f.user_data = user_data;
        break;

    case TST_ITEM_TEST:
        item->u.t.function = function;
        item->u.t.user_data = user_data;
        break;

    default:
        break;
    }
    return item;
}



void tst_suite_setup(TstSuite* suite, TstFunction setup, void* user_data)
{
    ANN(suite);
    _append(suite, TST_ITEM_SETUP, setup, user_data);
}



void tst_suite_add(TstSuite* suite, const char* name, TstFunction test, void* user_data)
{
    ANN(suite);
    TstItem* item = _append(suite, TST_ITEM_TEST, test, user_data);
    item->u.t.name = name;
}



void tst_suite_teardown(TstSuite* suite, TstFunction teardown, void* user_data)
{
    ANN(suite);
    _append(suite, TST_ITEM_TEARDOWN, teardown, user_data);
}



void tst_suite_run(TstSuite* suite, const char* match)
{
    log_trace("running testing suite");
    ANN(suite);
    TstItem* item = NULL;
    TstItem* current_fixture = NULL;
    print_start();

    // First pass: mark selected tests and mark fixtures with at least 1 test.
    for (uint32_t i = 0; i < suite->n_items; i++)
    {
        item = &suite->items[i];
        switch (item->type)
        {
        case TST_ITEM_SETUP:
            current_fixture = item;
            break;
        case TST_ITEM_TEARDOWN:
            if (current_fixture != NULL)
            {
                // The teardown is active iff the associated setup is.
                item->active = current_fixture->active;
            }
            current_fixture = NULL;
            break;
        case TST_ITEM_TEST:
            if (match == NULL || test_name_matches(&item->u.t, match))
            {
                item->active = true;
                if (current_fixture != NULL)
                    current_fixture->active = true;
            }
            break;
        default:
            break;
        }
    }

    // Second pass: run selected tests.
    int index = 0;
    int res = 0, cur_res = 0;
    for (uint32_t i = 0; i < suite->n_items; i++)
    {
        item = &suite->items[i];
        switch (item->type)
        {
        case TST_ITEM_SETUP:
        case TST_ITEM_TEARDOWN:
            if (item->active)
                item->u.f.function(suite);

            break;

        case TST_ITEM_TEST:
            if (item->active)
            {
                item->u.t.res = item->u.t.function(suite);
                cur_res = item->u.t.res;
                print_res_begin(index, item->u.t.name);
                print_res_end(index, item->u.t.name, cur_res);
                res += cur_res == 0 ? 0 : 1;
                index++;
            }
            break;

        default:
            break;
        }
    }

    // Third pass: reset active to true for all items.
    for (uint32_t i = 0; i < suite->n_items; i++)
    {
        item = &suite->items[i];
        item->active = true;
    }

    // TODO: mark as PASS or FAIL depending on the res
    print_end(index, res);
}



void tst_suite_destroy(TstSuite* suite)
{
    log_trace("destroy testing suite");
    ANN(suite);
    ANN(suite->items);
    suite->n_items = 0;
    suite->capacity = 0;
    FREE(suite->items);
}
