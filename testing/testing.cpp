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

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdlib>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TstOptions
{
    const char* legacy_match;
    const char* module;
    const char* group;
    const char* name;
    const char* tag;
    const char* exclude_tag;
    const char* resource;
    const char* isolation;
    const char* json_path;
    bool list;
    bool list_groups;
    bool fail_fast;
    bool compact;
    bool verbose;
    bool shuffle;
    uint64_t repeat;
    uint64_t seed;
    uint64_t slow_count;
    uint64_t slow_group_count;
    uint64_t default_timeout_ms;
    int color_mode;
} TstOptions;



typedef struct TstAggregate
{
    uint32_t selected_count;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t skipped_count;
    uint64_t summed_case_ns;
    uint64_t first_start_ns;
    uint64_t last_end_ns;
} TstAggregate;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint64_t _tst_now_ns(void)
{
    using Clock = std::chrono::steady_clock;
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}



static bool _tst_streq(const char* a, const char* b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }
    return std::string(a) == std::string(b);
}



static bool _tst_contains(const char* text, const char* needle)
{
    if (needle == NULL || needle[0] == '\0')
    {
        return true;
    }
    if (text == NULL)
    {
        return false;
    }
    return std::string(text).find(needle) != std::string::npos;
}



static std::string _tst_case_id(const TstCase* test)
{
    ANN(test);
    std::string module = test->module != NULL ? test->module : "default";
    std::string group = test->group != NULL ? test->group : "default";
    std::string name = test->name != NULL ? test->name : "unnamed";
    return module + "/" + group + "/" + name;
}



static std::string _tst_duration(uint64_t ns)
{
    char buffer[64] = {0};
    if (ns < 1000)
    {
        dvz_snprintf(buffer, sizeof(buffer), "%" PRIu64 " ns", ns);
    }
    else if (ns < 1000000)
    {
        dvz_snprintf(buffer, sizeof(buffer), "%.2f us", (double)ns / 1000.0);
    }
    else if (ns < 1000000000)
    {
        dvz_snprintf(buffer, sizeof(buffer), "%.2f ms", (double)ns / 1000000.0);
    }
    else
    {
        dvz_snprintf(buffer, sizeof(buffer), "%.2f s", (double)ns / 1000000000.0);
    }
    return std::string(buffer);
}



static bool _tst_stdout_is_tty(void)
{
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}



static bool _tst_use_color(const TstOptions* options)
{
    ANN(options);
    if (options->color_mode == 2)
    {
        return true;
    }
    if (options->color_mode == 1)
    {
        return false;
    }
    if (std::getenv("NO_COLOR") != NULL)
    {
        return false;
    }
    return _tst_stdout_is_tty();
}



static const char* _tst_status_name(TstStatus status)
{
    switch (status)
    {
    case TST_STATUS_PASS:
        return "PASS";
    case TST_STATUS_FAIL:
        return "FAIL";
    case TST_STATUS_SKIP:
        return "SKIP";
    case TST_STATUS_NOT_RUN:
    default:
        return "NONE";
    }
}



static const char* _tst_isolation_name(TstIsolation isolation)
{
    switch (isolation)
    {
    case TST_ISOLATION_PROCESS:
        return "process";
    case TST_ISOLATION_THREAD_SAFE:
        return "thread-safe";
    case TST_ISOLATION_EXCLUSIVE:
        return "exclusive";
    case TST_ISOLATION_SERIAL:
    default:
        return "serial";
    }
}



static bool _tst_resource_matches(uint64_t resources, const char* name)
{
    if (name == NULL)
    {
        return true;
    }
    if (_tst_streq(name, "none"))
    {
        return resources == TST_RES_NONE;
    }
    if (_tst_streq(name, "cpu"))
        return (resources & TST_RES_CPU) != 0;
    if (_tst_streq(name, "gpu"))
        return (resources & TST_RES_GPU) != 0;
    if (_tst_streq(name, "vulkan"))
        return (resources & TST_RES_VULKAN) != 0;
    if (_tst_streq(name, "glfw"))
        return (resources & TST_RES_GLFW) != 0;
    if (_tst_streq(name, "filesystem"))
        return (resources & TST_RES_FILESYSTEM) != 0;
    if (_tst_streq(name, "env"))
        return (resources & TST_RES_ENV) != 0;
    if (_tst_streq(name, "video"))
        return (resources & TST_RES_VIDEO) != 0;
    if (_tst_streq(name, "log-capture"))
        return (resources & TST_RES_LOG_CAPTURE) != 0;
    if (_tst_streq(name, "global-state"))
        return (resources & TST_RES_GLOBAL_STATE) != 0;
    return false;
}



static std::string _tst_resources_string(uint64_t resources)
{
    if (resources == TST_RES_NONE)
    {
        return "none";
    }

    std::string out;
    struct ResourceName
    {
        uint64_t flag;
        const char* name;
    };
    const ResourceName names[] = {
        {TST_RES_CPU, "cpu"},
        {TST_RES_GPU, "gpu"},
        {TST_RES_VULKAN, "vulkan"},
        {TST_RES_GLFW, "glfw"},
        {TST_RES_FILESYSTEM, "filesystem"},
        {TST_RES_ENV, "env"},
        {TST_RES_VIDEO, "video"},
        {TST_RES_LOG_CAPTURE, "log-capture"},
        {TST_RES_GLOBAL_STATE, "global-state"},
    };
    for (const ResourceName& rn : names)
    {
        if ((resources & rn.flag) == 0)
        {
            continue;
        }
        if (!out.empty())
        {
            out += ",";
        }
        out += rn.name;
    }
    return out;
}



static bool _tst_isolation_matches(TstIsolation isolation, const char* name)
{
    if (name == NULL)
    {
        return true;
    }
    return _tst_streq(_tst_isolation_name(isolation), name);
}



static std::string _tst_json_escape(const char* value)
{
    std::string out;
    if (value == NULL)
    {
        return out;
    }
    for (const char* p = value; *p != '\0'; ++p)
    {
        switch (*p)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += *p;
            break;
        }
    }
    return out;
}



static TstOptions _tst_options_default(void)
{
    TstOptions options = {};
    options.repeat = 1;
    options.seed = 1;
    options.slow_count = 0;
    options.slow_group_count = 0;
    options.default_timeout_ms = 0;
    options.color_mode = 0;
    options.compact = true;
    return options;
}



static uint64_t _tst_parse_u64(const char* value, uint64_t fallback)
{
    if (value == NULL)
    {
        return fallback;
    }
    char* end = NULL;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end == value)
    {
        return fallback;
    }
    return (uint64_t)parsed;
}



static void _tst_print_usage(void)
{
    dvz_fprintf(
        stdout,
        "Usage: dvztest [filter] [--module name] [--group name] [--case name]\n"
        "               [--tag tag] [--exclude-tag tag] [--resource name]\n"
        "               [--isolation mode] [--list] [--list-groups]\n"
        "               [--json path] [--fail-fast] [--repeat count]\n"
        "               [--shuffle --seed seed] [--slow count]\n"
        "               [--slow-groups count] [--timeout ms]\n");
}



static int _tst_parse_options(int argc, char** argv, TstOptions* options)
{
    ANN(options);
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (arg == NULL)
        {
            continue;
        }
        if (_tst_streq(arg, "--help") || _tst_streq(arg, "-h"))
        {
            _tst_print_usage();
            return 1;
        }
        if (_tst_streq(arg, "--module") && i + 1 < argc)
            options->module = argv[++i];
        else if (_tst_streq(arg, "--group") && i + 1 < argc)
            options->group = argv[++i];
        else if (_tst_streq(arg, "--case") && i + 1 < argc)
            options->name = argv[++i];
        else if (_tst_streq(arg, "--tag") && i + 1 < argc)
            options->tag = argv[++i];
        else if (_tst_streq(arg, "--exclude-tag") && i + 1 < argc)
            options->exclude_tag = argv[++i];
        else if (_tst_streq(arg, "--resource") && i + 1 < argc)
            options->resource = argv[++i];
        else if (_tst_streq(arg, "--isolation") && i + 1 < argc)
            options->isolation = argv[++i];
        else if (_tst_streq(arg, "--json") && i + 1 < argc)
            options->json_path = argv[++i];
        else if (_tst_streq(arg, "--repeat") && i + 1 < argc)
            options->repeat = std::max<uint64_t>(1, _tst_parse_u64(argv[++i], 1));
        else if (_tst_streq(arg, "--seed") && i + 1 < argc)
            options->seed = _tst_parse_u64(argv[++i], 1);
        else if (_tst_streq(arg, "--slow") && i + 1 < argc)
            options->slow_count = _tst_parse_u64(argv[++i], 0);
        else if (_tst_streq(arg, "--slow-groups") && i + 1 < argc)
            options->slow_group_count = _tst_parse_u64(argv[++i], 0);
        else if (_tst_streq(arg, "--timeout") && i + 1 < argc)
            options->default_timeout_ms = _tst_parse_u64(argv[++i], 0);
        else if (_tst_streq(arg, "--color") && i + 1 < argc)
        {
            const char* mode = argv[++i];
            options->color_mode = _tst_streq(mode, "always") ? 2 : (_tst_streq(mode, "never") ? 1 : 0);
        }
        else if (_tst_streq(arg, "--list"))
            options->list = true;
        else if (_tst_streq(arg, "--list-groups"))
            options->list_groups = true;
        else if (_tst_streq(arg, "--fail-fast"))
            options->fail_fast = true;
        else if (_tst_streq(arg, "--compact"))
            options->compact = true;
        else if (_tst_streq(arg, "--verbose"))
            options->verbose = true;
        else if (_tst_streq(arg, "--shuffle"))
            options->shuffle = true;
        else if (arg[0] != '-' && options->legacy_match == NULL)
            options->legacy_match = arg;
        else
        {
            dvz_fprintf(stderr, "unrecognized test option: %s\n", arg);
            return -1;
        }
    }
    return 0;
}



static bool _tst_case_matches(const TstCase* test, const TstOptions* options)
{
    ANN(test);
    ANN(options);

    if (options->module != NULL && !_tst_streq(test->module, options->module))
        return false;
    if (options->group != NULL && !_tst_streq(test->group, options->group))
        return false;
    if (options->name != NULL && !_tst_streq(test->name, options->name) &&
        !_tst_streq(test->function_name, options->name))
        return false;
    if (options->tag != NULL && !_tst_contains(test->tags, options->tag))
        return false;
    if (options->exclude_tag != NULL && _tst_contains(test->tags, options->exclude_tag))
        return false;
    if (!_tst_resource_matches(test->resources, options->resource))
        return false;
    if (!_tst_isolation_matches(test->isolation, options->isolation))
        return false;

    if (options->legacy_match != NULL)
    {
        std::string id = _tst_case_id(test);
        if (!_tst_contains(id.c_str(), options->legacy_match) &&
            !_tst_contains(test->function_name, options->legacy_match) &&
            !_tst_contains(test->tags, options->legacy_match))
        {
            return false;
        }
    }
    return true;
}



static void _tst_context_destroy(TstContext* ctx)
{
    ANN(ctx);
    ctx->captured_log_count = 0;
    ctx->captured_log_capacity = 0;
    dvz_free_ptr((void**)&ctx->captured_logs);
}



static void _tst_log_capture_reset(TstContext* ctx)
{
    ANN(ctx);
    ctx->captured_log_count = 0;
}



static void _tst_log_capture_append(
    TstContext* ctx, int level, const char* file, int line, const char* message)
{
    ANN(ctx);
    if (!ctx->capture_logs)
    {
        return;
    }

    if (ctx->captured_log_capacity == 0)
    {
        ctx->captured_logs =
            (TstLogRecord*)dvz_calloc(TST_LOG_CAPTURE_DEFAULT_CAPACITY, sizeof(TstLogRecord));
        ANN(ctx->captured_logs);
        ctx->captured_log_capacity = TST_LOG_CAPTURE_DEFAULT_CAPACITY;
    }
    else if (ctx->captured_log_count == ctx->captured_log_capacity)
    {
        uint32_t new_capacity = 2 * ctx->captured_log_capacity;
        ctx->captured_logs = (TstLogRecord*)dvz_realloc(
            ctx->captured_logs, (size_t)(new_capacity * sizeof(TstLogRecord)));
        ANN(ctx->captured_logs);
        ctx->captured_log_capacity = new_capacity;
    }

    ASSERT(ctx->captured_log_count < ctx->captured_log_capacity);
    TstLogRecord* rec = &ctx->captured_logs[ctx->captured_log_count++];
    ANN(rec);
    dvz_memset(rec, sizeof(*rec), 0, sizeof(*rec));
    rec->level = level;
    rec->line = line;
    dvz_snprintf(rec->file, sizeof(rec->file), "%s", file != NULL ? file : "");
    dvz_snprintf(rec->message, sizeof(rec->message), "%s", message != NULL ? message : "");
}



static int _tst_item_finalize(TstContext* ctx, int res)
{
    ANN(ctx);
    if (ctx->expect_error_active)
    {
        dvz_fprintf(stderr, "expected-error scope left open at end of test\n");
        ctx->expect_error_active = false;
        ctx->failure_message = "expected-error scope left open";
        res = 1;
    }
    if (ctx->strict_unexpected_errors && ctx->unexpected_error_seen)
    {
        dvz_fprintf(stderr, "unexpected error log emitted during test\n");
        ctx->failure_message = "unexpected error log emitted during test";
        res = 1;
    }
    return res;
}



static void _tst_print_case(const TstCase* result, const TstOptions* options)
{
    ANN(result);
    ANN(options);
    const bool color = _tst_use_color(options);
    const char* status = _tst_status_name(result->status);
    const char* code = "";
    const char* reset = "";
    if (color)
    {
        if (result->status == TST_STATUS_PASS)
            code = "\x1b[32m";
        else if (result->status == TST_STATUS_FAIL)
            code = "\x1b[31m";
        else if (result->status == TST_STATUS_SKIP)
            code = "\x1b[33m";
        reset = "\x1b[0m";
    }

    const std::string id = _tst_case_id(result);
    dvz_fprintf(
        stdout, "%s%-4s%s  %-68s %10s\n", code, status, reset, id.c_str(),
        _tst_duration(result->elapsed_ns).c_str());

    if (options->verbose || result->status == TST_STATUS_FAIL)
    {
        dvz_fprintf(stdout, "  function   %s\n", result->function_name);
        dvz_fprintf(stdout, "  resources  %s\n", _tst_resources_string(result->resources).c_str());
        dvz_fprintf(stdout, "  isolation  %s\n", _tst_isolation_name(result->isolation));
        dvz_fprintf(stdout, "  elapsed    %s\n", _tst_duration(result->elapsed_ns).c_str());
        if (result->timeout_ms > 0)
            dvz_fprintf(stdout, "  timeout    %" PRIu64 " ms\n", result->timeout_ms);
        if (result->skip_reason != NULL)
            dvz_fprintf(stdout, "  reason     %s\n", result->skip_reason);
    }
}



static void _tst_update_aggregate(TstAggregate* agg, const TstCase* result)
{
    ANN(agg);
    ANN(result);
    agg->selected_count++;
    if (result->status == TST_STATUS_PASS)
        agg->passed_count++;
    else if (result->status == TST_STATUS_FAIL)
        agg->failed_count++;
    else if (result->status == TST_STATUS_SKIP)
        agg->skipped_count++;
    agg->summed_case_ns += result->elapsed_ns;
    if (agg->first_start_ns == 0 || result->start_ns < agg->first_start_ns)
        agg->first_start_ns = result->start_ns;
    if (result->end_ns > agg->last_end_ns)
        agg->last_end_ns = result->end_ns;
}



static void _tst_print_summary(
    const TstRunSummary* summary, const std::vector<TstCase>& results,
    const std::map<std::string, TstAggregate>& modules,
    const std::map<std::string, TstAggregate>& groups)
{
    ANN(summary);
    dvz_fprintf(stdout, "--------------------------------------------------\n");
    dvz_fprintf(
        stdout, "%u/%u tests passed, %u failed, %u skipped\n", summary->passed_count,
        summary->selected_count, summary->failed_count, summary->skipped_count);
    dvz_fprintf(
        stdout, "case time: %s, runner time: %s\n",
        _tst_duration(summary->summed_case_ns).c_str(),
        _tst_duration(summary->runner_elapsed_ns).c_str());

    if (!modules.empty())
    {
        dvz_fprintf(stdout, "\nModules:\n");
        for (const auto& it : modules)
        {
            const TstAggregate& agg = it.second;
            dvz_fprintf(
                stdout, "  %-16s %3u selected, %3u failed, %10s summed\n", it.first.c_str(),
                agg.selected_count, agg.failed_count, _tst_duration(agg.summed_case_ns).c_str());
        }
    }

    if (!groups.empty() && results.size() < 64)
    {
        dvz_fprintf(stdout, "\nGroups:\n");
        for (const auto& it : groups)
        {
            const TstAggregate& agg = it.second;
            dvz_fprintf(
                stdout, "  %-32s %3u selected, %3u failed, %10s summed\n", it.first.c_str(),
                agg.selected_count, agg.failed_count, _tst_duration(agg.summed_case_ns).c_str());
        }
    }

    if (summary->failed_count > 0)
    {
        dvz_fprintf(stdout, "\nFailed tests:\n");
        for (const TstCase& result : results)
        {
            if (result.status == TST_STATUS_FAIL)
            {
                dvz_fprintf(stdout, "  - %s\n", _tst_case_id(&result).c_str());
            }
        }
    }
}



static void _tst_print_slow_cases(const std::vector<TstCase>& results, uint64_t count)
{
    if (count == 0 || results.empty())
    {
        return;
    }
    std::vector<TstCase> sorted = results;
    std::sort(sorted.begin(), sorted.end(), [](const TstCase& a, const TstCase& b) {
        return a.elapsed_ns > b.elapsed_ns;
    });
    count = std::min<uint64_t>(count, sorted.size());
    dvz_fprintf(stdout, "\nSlowest tests:\n");
    for (uint64_t i = 0; i < count; ++i)
    {
        dvz_fprintf(
            stdout, "  %-68s %10s\n", _tst_case_id(&sorted[(size_t)i]).c_str(),
            _tst_duration(sorted[(size_t)i].elapsed_ns).c_str());
    }
}



static void
_tst_print_slow_groups(const std::map<std::string, TstAggregate>& groups, uint64_t count)
{
    if (count == 0 || groups.empty())
    {
        return;
    }
    std::vector<std::pair<std::string, TstAggregate>> sorted(groups.begin(), groups.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second.summed_case_ns > b.second.summed_case_ns;
    });
    count = std::min<uint64_t>(count, sorted.size());
    dvz_fprintf(stdout, "\nSlowest groups:\n");
    for (uint64_t i = 0; i < count; ++i)
    {
        dvz_fprintf(
            stdout, "  %-32s %10s\n", sorted[(size_t)i].first.c_str(),
            _tst_duration(sorted[(size_t)i].second.summed_case_ns).c_str());
    }
}



static int _tst_write_json(
    const char* path, const TstRunSummary* summary, const std::vector<TstCase>& results)
{
    if (path == NULL)
    {
        return 0;
    }
    std::ofstream out(path);
    if (!out)
    {
        dvz_fprintf(stderr, "could not write JSON test results to %s\n", path);
        return 1;
    }

    out << "{\n";
    out << "  \"summary\": {\n";
    out << "    \"selected\": " << summary->selected_count << ",\n";
    out << "    \"passed\": " << summary->passed_count << ",\n";
    out << "    \"failed\": " << summary->failed_count << ",\n";
    out << "    \"skipped\": " << summary->skipped_count << ",\n";
    out << "    \"summed_case_ns\": " << summary->summed_case_ns << ",\n";
    out << "    \"runner_elapsed_ns\": " << summary->runner_elapsed_ns << "\n";
    out << "  },\n";
    out << "  \"cases\": [\n";
    for (size_t i = 0; i < results.size(); ++i)
    {
        const TstCase& r = results[i];
        out << "    {\n";
        out << "      \"module\": \"" << _tst_json_escape(r.module) << "\",\n";
        out << "      \"group\": \"" << _tst_json_escape(r.group) << "\",\n";
        out << "      \"name\": \"" << _tst_json_escape(r.name) << "\",\n";
        out << "      \"function\": \"" << _tst_json_escape(r.function_name) << "\",\n";
        out << "      \"status\": \"" << _tst_status_name(r.status) << "\",\n";
        out << "      \"resources\": \"" << _tst_resources_string(r.resources) << "\",\n";
        out << "      \"isolation\": \"" << _tst_isolation_name(r.isolation) << "\",\n";
        out << "      \"repeat_index\": " << r.repeat_index << ",\n";
        out << "      \"timeout_ms\": " << r.timeout_ms << ",\n";
        out << "      \"start_ns\": " << r.start_ns << ",\n";
        out << "      \"end_ns\": " << r.end_ns << ",\n";
        out << "      \"elapsed_ns\": " << r.elapsed_ns << "\n";
        out << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return 0;
}



static void _tst_print_list(TstSuite* suite, const TstOptions* options)
{
    ANN(suite);
    ANN(options);
    for (uint32_t i = 0; i < suite->n_cases; ++i)
    {
        const TstCase* test = &suite->cases[i];
        if (!_tst_case_matches(test, options))
        {
            continue;
        }
        dvz_fprintf(
            stdout, "%s  function=%s resources=%s isolation=%s\n", _tst_case_id(test).c_str(),
            test->function_name != NULL ? test->function_name : "",
            _tst_resources_string(test->resources).c_str(),
            _tst_isolation_name(test->isolation));
    }
}



static void _tst_print_groups(TstSuite* suite, const TstOptions* options)
{
    ANN(suite);
    ANN(options);
    std::set<std::string> groups;
    for (uint32_t i = 0; i < suite->n_cases; ++i)
    {
        const TstCase* test = &suite->cases[i];
        if (!_tst_case_matches(test, options))
        {
            continue;
        }
        groups.insert(
            std::string(test->module != NULL ? test->module : "default") + "/" +
            std::string(test->group != NULL ? test->group : "default"));
    }
    for (const std::string& group : groups)
    {
        dvz_fprintf(stdout, "%s\n", group.c_str());
    }
}



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

TstSuite tst_suite(void)
{
    TstSuite suite = {};
    suite.cases = (TstCase*)dvz_calloc(TST_DEFAULT_CAPACITY, sizeof(TstCase));
    ANN(suite.cases);
    suite.capacity = TST_DEFAULT_CAPACITY;
    suite.n_cases = 0;
    suite.current_module = NULL;
    suite.current_group = "default";
    suite.strict_unexpected_errors = false;
    suite.log_adapter.error_level = 3;
    return suite;
}



TstCaseDesc tst_case_desc(const char* name, const char* function_name, TstFunction test)
{
    TstCaseDesc desc = {};
    desc.name = name;
    desc.function_name = function_name;
    desc.tags = NULL;
    desc.resources = TST_RES_CPU;
    desc.isolation = TST_ISOLATION_SERIAL;
    desc.timeout_ms = 0;
    desc.test = test;
    return desc;
}



void tst_suite_module(TstSuite* suite, const char* module)
{
    ANN(suite);
    suite->current_module = module != NULL ? module : "default";
    suite->current_group = "default";
}



void tst_suite_group(TstSuite* suite, const char* group)
{
    ANN(suite);
    suite->current_group = group != NULL ? group : "default";
}



void tst_suite_add_case(TstSuite* suite, TstCaseDesc desc)
{
    ANN(suite);
    ANN(desc.test);
    if (suite->capacity == suite->n_cases)
    {
        ANN(suite->cases);
        ASSERT(suite->n_cases > 0);
        suite->cases =
            (TstCase*)dvz_realloc(suite->cases, (size_t)(2 * suite->n_cases * sizeof(TstCase)));
        ANN(suite->cases);
        suite->capacity *= 2;
    }
    ASSERT(suite->n_cases < suite->capacity);

    TstCase* test = &suite->cases[suite->n_cases++];
    ANN(test);
    dvz_memset(test, sizeof(*test), 0, sizeof(*test));
    test->module = suite->current_module != NULL ? suite->current_module : desc.tags;
    if (test->module == NULL)
    {
        test->module = "default";
    }
    test->group = suite->current_group != NULL ? suite->current_group : "default";
    test->name = desc.name != NULL ? desc.name : desc.function_name;
    test->function_name = desc.function_name != NULL ? desc.function_name : desc.name;
    test->tags = desc.tags;
    test->resources = desc.resources;
    test->isolation = desc.isolation;
    test->timeout_ms = desc.timeout_ms;
    test->test = desc.test;
    test->setup = desc.setup;
    test->teardown = desc.teardown;
    test->user_data = desc.user_data;
    test->status = TST_STATUS_NOT_RUN;
}



void tst_suite_set_log_adapter(TstSuite* suite, const TstLogAdapter* adapter)
{
    ANN(suite);
    if (adapter == NULL)
    {
        dvz_memset(&suite->log_adapter, sizeof(suite->log_adapter), 0, sizeof(suite->log_adapter));
        suite->log_adapter.error_level = 3;
        return;
    }
    suite->log_adapter = *adapter;
}



int tst_suite_run(TstSuite* suite, int argc, char** argv)
{
    ANN(suite);
    ANN(suite->cases);

    TstOptions options = _tst_options_default();
    int parsed = _tst_parse_options(argc, argv, &options);
    if (parsed != 0)
    {
        return parsed > 0 ? 0 : 1;
    }

    if (options.list)
    {
        _tst_print_list(suite, &options);
        return 0;
    }
    if (options.list_groups)
    {
        _tst_print_groups(suite, &options);
        return 0;
    }

    std::vector<TstCase*> selected;
    for (uint32_t i = 0; i < suite->n_cases; ++i)
    {
        TstCase* test = &suite->cases[i];
        if (_tst_case_matches(test, &options))
        {
            selected.push_back(test);
        }
    }

    if (options.shuffle && selected.size() > 1)
    {
        std::mt19937_64 rng(options.seed);
        std::shuffle(selected.begin(), selected.end(), rng);
    }

    TstRunSummary summary = {};
    std::vector<TstCase> results;
    std::map<std::string, TstAggregate> modules;
    std::map<std::string, TstAggregate> groups;
    const uint64_t runner_start_ns = _tst_now_ns();

    for (uint64_t repeat = 0; repeat < options.repeat; ++repeat)
    {
        for (TstCase* test : selected)
        {
            TstCase result = *test;
            result.repeat_index = repeat;
            if (result.timeout_ms == 0)
            {
                result.timeout_ms = options.default_timeout_ms;
            }

            TstContext ctx = {};
            ctx.suite = suite;
            ctx.test = &result;
            ctx.user_data = result.user_data;
            ctx.suppress_expected_error_output = true;
            ctx.strict_unexpected_errors = suite->strict_unexpected_errors;

            if (suite->log_adapter.install != NULL)
            {
                suite->log_adapter.install(&ctx, suite->log_adapter.user_data);
            }

            result.start_ns = _tst_now_ns();
            int res = 0;
            if (result.setup != NULL)
            {
                res = result.setup(&ctx, &result);
            }
            if (res == 0)
            {
                res = result.test(&ctx, &result);
            }
            if (result.teardown != NULL)
            {
                int teardown_res = result.teardown(&ctx, &result);
                if (res == 0)
                {
                    res = teardown_res;
                }
            }
            res = _tst_item_finalize(&ctx, res);
            result.end_ns = _tst_now_ns();
            result.elapsed_ns = result.end_ns - result.start_ns;
            if (result.timeout_ms > 0 && result.elapsed_ns > result.timeout_ms * 1000000ull)
            {
                result.skip_reason = "timeout exceeded";
                res = 1;
            }
            result.result = res;
            result.status = res == 0 ? TST_STATUS_PASS : TST_STATUS_FAIL;

            if (suite->log_adapter.uninstall != NULL)
            {
                suite->log_adapter.uninstall(suite->log_adapter.user_data);
            }

            _tst_print_case(&result, &options);
            _tst_update_aggregate(&modules[result.module != NULL ? result.module : "default"], &result);
            _tst_update_aggregate(&groups[_tst_case_id(&result).substr(
                                      0, _tst_case_id(&result).rfind('/'))],
                                  &result);

            summary.selected_count++;
            if (result.status == TST_STATUS_PASS)
                summary.passed_count++;
            else if (result.status == TST_STATUS_FAIL)
                summary.failed_count++;
            else if (result.status == TST_STATUS_SKIP)
                summary.skipped_count++;
            summary.summed_case_ns += result.elapsed_ns;

            results.push_back(result);
            _tst_context_destroy(&ctx);

            if (options.fail_fast && result.status == TST_STATUS_FAIL)
            {
                break;
            }
        }
        if (options.fail_fast && summary.failed_count > 0)
        {
            break;
        }
    }

    summary.runner_elapsed_ns = _tst_now_ns() - runner_start_ns;
    suite->last_summary = summary;

    _tst_print_summary(&summary, results, modules, groups);
    _tst_print_slow_cases(results, options.slow_count);
    _tst_print_slow_groups(groups, options.slow_group_count);

    if (_tst_write_json(options.json_path, &summary, results) != 0)
    {
        return 1;
    }
    return summary.failed_count == 0 ? 0 : 1;
}



void tst_log_capture_begin(TstContext* ctx)
{
    ANN(ctx);
    ctx->capture_logs = true;
    _tst_log_capture_reset(ctx);
}



void tst_log_capture_end(TstContext* ctx)
{
    ANN(ctx);
    ctx->capture_logs = false;
}



uint32_t tst_log_capture_count(const TstContext* ctx)
{
    ANN(ctx);
    return ctx->captured_log_count;
}



const TstLogRecord* tst_log_capture_get(const TstContext* ctx, uint32_t index)
{
    ANN(ctx);
    if (index >= ctx->captured_log_count)
    {
        return NULL;
    }
    return &ctx->captured_logs[index];
}



void tst_expect_error_begin(TstContext* ctx)
{
    ANN(ctx);
    ctx->expect_error_active = true;
    ctx->expect_error_seen = false;
}



int tst_expect_error_end(TstContext* ctx)
{
    ANN(ctx);
    if (!ctx->expect_error_active)
    {
        return 1;
    }
    ctx->expect_error_active = false;
    return ctx->expect_error_seen ? 0 : 1;
}



void tst_set_strict_unexpected_errors(TstSuite* suite, bool enabled)
{
    ANN(suite);
    suite->strict_unexpected_errors = enabled;
}



int tst_context_log(TstContext* ctx, int level, const char* file, int line, const char* message)
{
    if (ctx == NULL)
    {
        return 0;
    }
    _tst_log_capture_append(ctx, level, file, line, message);
    const int error_level = ctx->suite != NULL ? ctx->suite->log_adapter.error_level : 3;
    if (level >= error_level)
    {
        if (ctx->expect_error_active)
        {
            ctx->expect_error_seen = true;
            return ctx->suppress_expected_error_output ? 1 : 0;
        }
        if (ctx->strict_unexpected_errors)
        {
            ctx->unexpected_error_seen = true;
        }
    }
    return 0;
}



void tst_assert_fail(TstContext* ctx, const char* file, int line, const char* expr)
{
    char message[512] = {0};
    dvz_snprintf(message, sizeof(message), "assertion '%s' failed", expr != NULL ? expr : "");
    dvz_fprintf(stderr, "%s:%d: %s\n", file != NULL ? file : "", line, message);
    (void)tst_context_log(ctx, ctx != NULL && ctx->suite != NULL ? ctx->suite->log_adapter.error_level : 3,
                          file, line, message);
}



void tst_suite_destroy(TstSuite* suite)
{
    ANN(suite);
    suite->n_cases = 0;
    suite->capacity = 0;
    dvz_free_ptr((void**)&suite->cases);
}
