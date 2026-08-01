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
#include "_compat.h"
#include "_time_utils.h"
#include "testing.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SchedulerFixture
{
    uint32_t marker;
} SchedulerFixture;



typedef struct SchedulerRunState
{
    char token[64];
    uint32_t selected_count;
    bool child_process;
    bool child_metadata_mismatch;
    bool prepared;
} SchedulerRunState;



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static uint64_t _scheduler_pid(void);
static bool _scheduler_has_run_state(const TstContext* ctx);
static bool _scheduler_suite_has_run_state(const TstSuite* suite);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the current process id as an integer.
 *
 * @return process id
 */
static uint64_t _scheduler_pid(void)
{
#if defined(_WIN32)
    return (uint64_t)_getpid();
#else
    return (uint64_t)getpid();
#endif
}



/**
 * Store the root scheduler process id for child-process checks.
 */
static void _scheduler_init_root_pid(void)
{
    if (getenv("DVZTEST_SCHEDULER_ROOT_PID") != NULL)
    {
        return;
    }

    char value[32] = {0};
    dvz_snprintf(value, sizeof(value), "%" PRIu64, _scheduler_pid());
#if defined(_WIN32)
    _putenv_s("DVZTEST_SCHEDULER_ROOT_PID", value);
#else
    setenv("DVZTEST_SCHEDULER_ROOT_PID", value, 1);
#endif
}



/**
 * Verify that a test context observes the immutable adapter state.
 *
 * @param ctx test context
 * @return whether the context aliases prepared suite run state
 */
static bool _scheduler_has_run_state(const TstContext* ctx)
{
    ANN(ctx);

    const SchedulerRunState* state = (const SchedulerRunState*)tst_context_run_state(ctx);
    ANN(state);
    return state->prepared && state->token[0] != '\0' && state == tst_suite_run_state(ctx->suite);
}



/**
 * Verify that a fixture constructor observes the immutable adapter state.
 *
 * @param suite test suite
 * @return whether the suite has prepared run state
 */
static bool _scheduler_suite_has_run_state(const TstSuite* suite)
{
    ANN(suite);

    const SchedulerRunState* state = (const SchedulerRunState*)tst_suite_run_state(suite);
    ANN(state);
    return state->prepared && state->token[0] != '\0';
}



/**
 * Parse synthetic scheduler run-adapter options.
 *
 * @param state adapter state
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @param index current option index
 * @return positive when handled, zero when unhandled, negative on error
 */
static int _scheduler_parse_option(void* state, int argc, char** argv, int* index)
{
    ANN(state);
    ANN(argv);
    ANN(index);

    SchedulerRunState* run = (SchedulerRunState*)state;
    const char* arg = argv[*index];
    if (arg != NULL && strcmp(arg, "--scheduler-token") == 0)
    {
        if (*index + 1 >= argc || argv[*index + 1] == NULL)
        {
            dvz_fprintf(stderr, "--scheduler-token requires a value\n");
            return -1;
        }
        dvz_snprintf(run->token, sizeof(run->token), "%s", argv[++*index]);
        return 1;
    }
    if (arg != NULL && strcmp(arg, "--scheduler-child-metadata-mismatch") == 0)
    {
        run->child_metadata_mismatch = true;
        return 1;
    }
    return 0;
}



/**
 * Record child-process state for synthetic metadata checks.
 *
 * @param state adapter state
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @param list whether list mode was requested
 * @param list_groups whether group-list mode was requested
 * @param child_process whether this process writes a child report
 * @return zero on success
 */
static int _scheduler_configure_run(
    void* state, int argc, char** argv, bool list, bool list_groups, bool child_process)
{
    (void)argc;
    (void)argv;
    (void)list;
    (void)list_groups;

    SchedulerRunState* run = (SchedulerRunState*)state;
    ANN(run);
    run->child_process = child_process;
    return 0;
}



/**
 * Mark the synthetic run state ready after runner case selection.
 *
 * @param state adapter state
 * @param case_count selected case count
 * @param cases selected cases
 * @param child_process whether this process writes a child report
 * @return zero on success
 */
static int _scheduler_prepare_run(
    void* state, uint32_t case_count, const TstCase* const* cases, bool child_process)
{
    (void)cases;

    SchedulerRunState* run = (SchedulerRunState*)state;
    ANN(run);
    AT(run->child_process == child_process);
    run->selected_count = case_count;
    run->prepared = true;
    return 0;
}



/**
 * Write synthetic run metadata for adapter aggregation checks.
 *
 * @param state adapter state
 * @param json output JSON buffer
 * @param size output JSON buffer size
 * @return zero on success
 */
static int _scheduler_write_run_json(const void* state, char* json, size_t size)
{
    ANN(state);
    ANN(json);

    const SchedulerRunState* run = (const SchedulerRunState*)state;
    const char* suffix = run->child_process && run->child_metadata_mismatch ? "-child" : "";
    int written = dvz_snprintf(
        json, size, "{\"scheduler_token\":\"%s%s\",\"selected_count\":%u}", run->token,
        suffix, run->selected_count);
    return written < 0 || (size_t)written >= size ? 1 : 0;
}

/**
 * Create a synthetic fixture for scheduler tests.
 *
 * @param suite test suite
 * @param worker_index worker or process index
 * @return fixture pointer
 */
static void* _scheduler_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    ASSERT(_scheduler_suite_has_run_state(suite));

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
    AT(_scheduler_has_run_state(ctx));
    AT(item->name != NULL);
    return 0;
}



/**
 * Verify child-process cases execute outside the root scheduler process when requested.
 *
 * @param ctx test context
 * @param item test case metadata
 * @return zero on success
 */
static int _scheduler_process_case(TstContext* ctx, const TstCase* item)
{
    ANN(ctx);
    ANN(item);
    AT(_scheduler_has_run_state(ctx));

    const char* require_child = getenv("DVZTEST_SCHEDULER_REQUIRE_CHILD");
    if (require_child == NULL || require_child[0] == '\0' || require_child[0] == '0')
    {
        return 0;
    }

    const char* root_pid_text = getenv("DVZTEST_SCHEDULER_ROOT_PID");
    ANN(root_pid_text);
    const uint64_t root_pid = (uint64_t)strtoull(root_pid_text, NULL, 10);
    AT(root_pid != 0);
    AT(_scheduler_pid() != root_pid);
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
    TstCaseDesc process_desc =
        tst_case_desc("process-isolated-child", "_scheduler_process_case", _scheduler_process_case);
    process_desc.resources = TST_RES_CPU;
    process_desc.isolation = TST_ISOLATION_PROCESS;
    tst_suite_add_case(suite, process_desc);
    _scheduler_add_case(
        suite, "serial-env", TST_RES_CPU | TST_RES_ENV, TST_ISOLATION_SERIAL, NULL,
        TST_FIXTURE_SCOPE_NONE);
    _scheduler_add_case(
        suite, "serial-log-capture", TST_RES_CPU | TST_RES_LOG_CAPTURE, TST_ISOLATION_SERIAL,
        NULL, TST_FIXTURE_SCOPE_NONE);
    TstCaseDesc exclusive_desc =
        tst_case_desc("serial-exclusive-isolation", "_scheduler_process_case", _scheduler_process_case);
    exclusive_desc.resources = TST_RES_CPU;
    exclusive_desc.isolation = TST_ISOLATION_EXCLUSIVE;
    tst_suite_add_case(suite, exclusive_desc);
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
    _scheduler_init_root_pid();

    TstSuite suite = tst_suite();
    SchedulerRunState run_state = {0};
    dvz_snprintf(run_state.token, sizeof(run_state.token), "default");
    TstRunAdapter adapter = {0};
    adapter.parse_option = _scheduler_parse_option;
    adapter.configure = _scheduler_configure_run;
    adapter.prepare = _scheduler_prepare_run;
    adapter.write_json = _scheduler_write_run_json;
    adapter.state = &run_state;
    tst_suite_set_run_adapter(&suite, &adapter);
    _scheduler_register(&suite);

    int res = tst_suite_run(&suite, argc, argv);
    tst_suite_destroy(&suite);
    return res;
}
