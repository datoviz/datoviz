/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz scheduler test runner                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_time_utils.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SchedulerFixture
{
    uint32_t marker;
} SchedulerFixture;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a synthetic fixture for scheduler tests.
 *
 * @param suite test suite
 * @param worker_index worker or process index
 * @return fixture pointer
 */
static void* _scheduler_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    (void)suite;

    SchedulerFixture* fixture = (SchedulerFixture*)dvz_calloc(1, sizeof(SchedulerFixture));
    ANN(fixture);
    fixture->marker = 100u + worker_index;
    dvz_sleep_us(1000);
    return fixture;
}



/**
 * Destroy a synthetic fixture for scheduler tests.
 *
 * @param fixture fixture pointer
 */
static void _scheduler_fixture_destroy(void* fixture)
{
    dvz_free(fixture);
}



/**
 * Verify a case can run without fixture state.
 *
 * @param ctx test context
 * @param item test case metadata
 * @return zero on success
 */
static int _scheduler_plain_case(TstContext* ctx, const TstCase* item)
{
    ANN(ctx);
    ANN(item);
    AT(item->name != NULL);
    return 0;
}



/**
 * Verify a case can access its registered scheduler fixture.
 *
 * @param ctx test context
 * @param item test case metadata
 * @return zero on success
 */
static int _scheduler_fixture_case(TstContext* ctx, const TstCase* item)
{
    ANN(ctx);
    ANN(item);

    SchedulerFixture* fixture = (SchedulerFixture*)tst_context_fixture(ctx, item->fixture);
    ANN(fixture);
    AT(fixture->marker >= 100u);
    return 0;
}



/**
 * Add a synthetic test case to the scheduler suite.
 *
 * @param suite test suite
 * @param name case name
 * @param resources resource flags
 * @param isolation isolation mode
 * @param fixture fixture name, or NULL
 * @param fixture_scope fixture scope
 */
static void _scheduler_add_case(
    TstSuite* suite, const char* name, uint64_t resources, TstIsolation isolation,
    const char* fixture, TstFixtureScope fixture_scope)
{
    ANN(suite);

    TstCaseDesc desc =
        tst_case_desc(name, fixture != NULL ? "_scheduler_fixture_case" : "_scheduler_plain_case",
                      fixture != NULL ? _scheduler_fixture_case : _scheduler_plain_case);
    desc.resources = resources;
    desc.isolation = isolation;
    desc.fixture = fixture;
    desc.fixture_scope = fixture_scope;
    tst_suite_add_case(suite, desc);
}



/**
 * Register synthetic cases that exercise runner scheduling classes.
 *
 * @param suite test suite
 */
static void _scheduler_register(TstSuite* suite)
{
    ANN(suite);

    tst_suite_module(suite, "scheduler");
    tst_suite_group(suite, "policy");

    tst_suite_register_fixture(
        suite, "process-fixture", TST_FIXTURE_SCOPE_PROCESS, _scheduler_fixture_create,
        _scheduler_fixture_destroy);
    tst_suite_register_fixture(
        suite, "exclusive-fixture", TST_FIXTURE_SCOPE_EXCLUSIVE, _scheduler_fixture_create,
        _scheduler_fixture_destroy);

    _scheduler_add_case(
        suite, "parallel-cpu-a", TST_RES_CPU, TST_ISOLATION_SERIAL, NULL,
        TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "parallel-cpu-b", TST_RES_CPU, TST_ISOLATION_THREAD_SAFE, NULL,
        TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "parallel-process-fixture", TST_RES_CPU, TST_ISOLATION_SERIAL,
        "process-fixture", TST_FIXTURE_SCOPE_PROCESS);
    _scheduler_add_case(
        suite, "serial-env", TST_RES_CPU | TST_RES_ENV, TST_ISOLATION_SERIAL, NULL,
        TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "serial-log-capture", TST_RES_CPU | TST_RES_LOG_CAPTURE, TST_ISOLATION_SERIAL,
        NULL, TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "serial-exclusive-isolation", TST_RES_CPU, TST_ISOLATION_EXCLUSIVE, NULL,
        TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "serial-exclusive-fixture", TST_RES_CPU, TST_ISOLATION_SERIAL,
        "exclusive-fixture", TST_FIXTURE_SCOPE_EXCLUSIVE);
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run synthetic scheduler tests.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();
    _scheduler_register(&suite);

    int res = tst_suite_run(&suite, argc, argv);
    tst_suite_destroy(&suite);
    return res;
}
