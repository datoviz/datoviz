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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/common/macros.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TST_DEFAULT_CAPACITY 32
#define TST_LOG_CAPTURE_DEFAULT_CAPACITY 16



/*************************************************************************************************/
/*  Test assertions                                                                              */
/*************************************************************************************************/

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        tst_assert_fail(NULL, __FILE__, __LINE__, #x);                                            \
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
/*  Platform helpers                                                                             */
/*************************************************************************************************/

static inline int tst_setenv(const char* name, const char* value)
{
#if defined(_WIN32)
    return _putenv_s(name, value);
#else
    return setenv(name, value, 1);
#endif
}

static inline int tst_unsetenv(const char* name)
{
#if defined(_WIN32)
    return _putenv_s(name, "");
#else
    return unsetenv(name);
#endif
}



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define TST_MODULE(suite, module_name) tst_suite_module((suite), (module_name))

#define TST_GROUP(group_name) tst_suite_group((suite), (group_name))

#define TST_CASE(test)                                                                            \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_CASE_EX(test, desc_expr)                                                              \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = (desc_expr);                                                      \
        if (_tst_desc.name == NULL)                                                               \
            _tst_desc.name = #test;                                                               \
        if (_tst_desc.function_name == NULL)                                                      \
            _tst_desc.function_name = #test;                                                      \
        _tst_desc.test = (test);                                                                  \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

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

#define AT_EXPECTED_LOG_STRICT(suite, level, x)                                                   \
    do                                                                                            \
    {                                                                                             \
        tst_expect_log_begin((suite), (level));                                                    \
        AT((x));                                                                                  \
        AT(tst_expect_error_end((suite)) == 0);                                                   \
    } while (0)



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TST_RES_NONE = 0,
    TST_RES_CPU = 1u << 0,
    TST_RES_GPU = 1u << 1,
    TST_RES_VULKAN = 1u << 2,
    TST_RES_GLFW = 1u << 3,
    TST_RES_FILESYSTEM = 1u << 4,
    TST_RES_ENV = 1u << 5,
    TST_RES_VIDEO = 1u << 6,
    TST_RES_LOG_CAPTURE = 1u << 7,
    TST_RES_GLOBAL_STATE = 1u << 8,
} TstResourceFlags;



typedef enum
{
    TST_ISOLATION_SERIAL = 0,
    TST_ISOLATION_PROCESS = 1,
    TST_ISOLATION_THREAD_SAFE = 2,
    TST_ISOLATION_EXCLUSIVE = 3,
} TstIsolation;



typedef enum
{
    TST_STATUS_NOT_RUN = 0,
    TST_STATUS_PASS = 1,
    TST_STATUS_FAIL = 2,
    TST_STATUS_SKIP = 3,
} TstStatus;



typedef enum
{
    TST_FIXTURE_SCOPE_NONE = 0,
    TST_FIXTURE_SCOPE_CASE = 1,
    TST_FIXTURE_SCOPE_WORKER = 2,
    TST_FIXTURE_SCOPE_PROCESS = 3,
    TST_FIXTURE_SCOPE_EXCLUSIVE = 4,
} TstFixtureScope;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TstCase TstCase;
typedef struct TstCaseDesc TstCaseDesc;
typedef struct TstContext TstContext;
typedef struct TstLogAdapter TstLogAdapter;
typedef struct TstLogRecord TstLogRecord;
typedef struct TstRunAdapter TstRunAdapter;
typedef struct TstRunSummary TstRunSummary;
typedef struct TstSuite TstSuite;

typedef int (*TstFunction)(TstContext* suite, const TstCase* item);
typedef const char* (*TstSkipFunction)(TstContext* suite, const TstCase* item);
typedef void (*TstLogInstall)(TstContext* ctx, void* user_data);
typedef void (*TstLogUninstall)(void* user_data);
typedef void* (*TstFixtureCreate)(TstSuite* suite, uint32_t worker_index);
typedef void (*TstFixtureDestroy)(void* fixture);
typedef int (*TstRunParseOption)(void* state, int argc, char** argv, int* index);
typedef int (*TstRunConfigure)(
    void* state, int argc, char** argv, bool list, bool list_groups, bool child_process);
typedef int (*TstRunEarlyAction)(void* state);
typedef int (*TstRunPrepare)(
    void* state, uint32_t case_count, const TstCase* const* cases, bool child_process);
typedef int (*TstRunWriteJson)(const void* state, char* json, size_t size);
typedef void (*TstRunReport)(const void* state);
typedef void (*TstRunPrintUsage)(const void* state);
typedef void (*TstRunDestroy)(void* state);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TstLogRecord
{
    int level;
    int line;
    char file[64];
    char message[512];
};



struct TstCaseDesc
{
    const char* name;
    const char* function_name;
    const char* tags;
    uint64_t resources;
    TstIsolation isolation;
    uint64_t timeout_ms;
    TstFunction test;
    TstFunction setup;
    TstFunction teardown;
    TstSkipFunction skip;
    const char* fixture;
    TstFixtureScope fixture_scope;
    void* user_data;
};



struct TstCase
{
    const char* module;
    const char* group;
    const char* name;
    const char* function_name;
    const char* tags;
    uint64_t resources;
    TstIsolation isolation;
    uint64_t timeout_ms;
    TstFunction test;
    TstFunction setup;
    TstFunction teardown;
    TstSkipFunction skip;
    const char* fixture;
    TstFixtureScope fixture_scope;
    void* user_data;

    TstStatus status;
    int result;
    uint64_t repeat_index;
    uint64_t order_index;
    uint32_t shard_index;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t elapsed_ns;
    uint64_t fixture_setup_ns;
    const char* skip_reason;
};



struct TstLogAdapter
{
    TstLogInstall install;
    TstLogUninstall uninstall;
    void* user_data;
    int error_level;
};



struct TstRunAdapter
{
    TstRunParseOption parse_option;
    TstRunConfigure configure;
    TstRunEarlyAction early_action;
    TstRunPrepare prepare;
    TstRunWriteJson write_json;
    TstRunReport report;
    TstRunPrintUsage print_usage;
    TstRunDestroy destroy;
    void* state;
};



struct TstContext
{
    TstSuite* suite;
    const TstCase* test;
    void* user_data;
    bool capture_logs;
    bool expect_error_active;
    bool expect_error_seen;
    int expect_log_level;
    bool unexpected_error_seen;
    bool suppress_expected_error_output;
    bool strict_unexpected_errors;
    uint32_t captured_log_count;
    uint32_t captured_log_capacity;
    TstLogRecord* captured_logs;
    const char* skip_reason;
    const char* failure_message;
    uint32_t worker_index;
    uint64_t fixture_setup_ns;
    void* fixture_state;
    const void* run_state;
};



struct TstRunSummary
{
    uint32_t selected_count;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t skipped_count;
    uint64_t summed_case_ns;
    uint64_t runner_elapsed_ns;
};



struct TstSuite
{
    uint32_t n_cases;
    uint32_t capacity;
    TstCase* cases;
    void* context;
    const char* current_module;
    const char* current_group;
    bool strict_unexpected_errors;
    TstLogAdapter log_adapter;
    TstRunAdapter run_adapter;
    TstRunSummary last_summary;
    void* fixture_registry;
};



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

TstSuite tst_suite(void);

TstCaseDesc tst_case_desc(const char* name, const char* function_name, TstFunction test);

void tst_suite_module(TstSuite* suite, const char* module);

void tst_suite_group(TstSuite* suite, const char* group);

void tst_suite_add_case(TstSuite* suite, TstCaseDesc desc);

void tst_suite_set_log_adapter(TstSuite* suite, const TstLogAdapter* adapter);

void tst_suite_set_run_adapter(TstSuite* suite, const TstRunAdapter* adapter);

const void* tst_suite_run_state(const TstSuite* suite);

const void* tst_context_run_state(const TstContext* ctx);

void tst_suite_register_fixture(
    TstSuite* suite, const char* name, TstFixtureScope scope, TstFixtureCreate create,
    TstFixtureDestroy destroy);

int tst_suite_run(TstSuite* suite, int argc, char** argv);

void tst_suite_destroy(TstSuite* suite);

void tst_log_capture_begin(TstContext* ctx);

void tst_log_capture_end(TstContext* ctx);

uint32_t tst_log_capture_count(const TstContext* ctx);

const TstLogRecord* tst_log_capture_get(const TstContext* ctx, uint32_t index);

void tst_expect_error_begin(TstContext* ctx);

void tst_expect_log_begin(TstContext* ctx, int level);

int tst_expect_error_end(TstContext* ctx);

void tst_skip(TstContext* ctx, const char* reason);

void tst_set_strict_unexpected_errors(TstSuite* suite, bool enabled);

void* tst_context_fixture(TstContext* ctx, const char* name);

int tst_context_log(TstContext* ctx, int level, const char* file, int line, const char* message);

void tst_assert_fail(TstContext* ctx, const char* file, int line, const char* expr);



EXTERN_C_OFF

#endif
