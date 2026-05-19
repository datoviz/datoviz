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
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "testing.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TST_RESULT_NAME_WIDTH 80
#define TST_RESULT_TIME_WIDTH 10
#define TST_RESULT_SEPARATOR_WIDTH                                                               \
    (4 + 2 + TST_RESULT_NAME_WIDTH + 1 + TST_RESULT_TIME_WIDTH)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TstOptions
{
    const char* positional_filter;
    const char* module;
    const char* group;
    const char* name;
    const char* tag;
    const char* exclude_tag;
    const char* resource;
    const char* isolation;
    const char* json_path;
    const char* child_json_path;
    const char* parent_json_path;
    const char* shard_policy;
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
    uint32_t jobs;
    uint32_t shard_index;
    uint32_t shard_count;
    uint64_t selected_order_index;
    uint64_t repeat_index;
    int color_mode;
    bool process_child;
} TstOptions;



typedef enum TstShardPolicy
{
    TST_SHARD_POLICY_ALL = 0,
    TST_SHARD_POLICY_PARALLEL_SAFE = 1,
    TST_SHARD_POLICY_SERIAL_ONLY = 2,
} TstShardPolicy;



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



typedef struct TstFixtureDef
{
    std::string name;
    TstFixtureScope scope;
    TstFixtureCreate create;
    TstFixtureDestroy destroy;
    void* worker_instance;
    bool worker_created;
} TstFixtureDef;



typedef struct TstFixtureRunState
{
    TstSuite* suite;
    std::vector<TstFixtureDef>* registry;
    uint32_t worker_index;
    std::vector<std::pair<size_t, void*>> case_instances;
} TstFixtureRunState;



typedef struct TstSelectedCase
{
    TstCase* test;
    uint64_t order_index;
} TstSelectedCase;



typedef struct TstOwnedResult
{
    TstCase test;
    std::string module;
    std::string group;
    std::string name;
    std::string function_name;
    std::string tags;
    std::string fixture;
    std::string skip_reason;
} TstOwnedResult;



typedef struct TstChildProc
{
    uint32_t shard_index;
    uint32_t shard_count;
    TstShardPolicy policy;
    std::string json_path;
#if defined(_WIN32)
    intptr_t pid;
#else
    pid_t pid;
#endif
} TstChildProc;



typedef struct TstShardProgress
{
    uint64_t total_cases;
    uint64_t completed_cases;
    uint64_t failed_cases;
    uint64_t skipped_cases;
    uint32_t total_shards;
    uint32_t completed_shards;
} TstShardProgress;



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



static const char* _tst_nonnull(const std::string& value)
{
    return value.empty() ? NULL : value.c_str();
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



/**
 * Remove a fixed prefix from a display label when it is present.
 *
 * @param label display label to trim.
 * @param prefix prefix to remove.
 */
static void _tst_drop_display_prefix(std::string* label, const std::string& prefix)
{
    ANN(label);
    if (prefix.empty())
    {
        return;
    }
    if (label->rfind(prefix, 0) == 0)
    {
        label->erase(0, prefix.size());
    }
}



/**
 * Split a metadata field into the token form used by fixture names.
 *
 * @param value metadata string.
 * @return Normalized tokens.
 */
static std::vector<std::string> _tst_display_tokens(const char* value)
{
    std::vector<std::string> tokens;
    if (value == NULL || value[0] == '\0')
    {
        return tokens;
    }

    std::string token;
    for (const char* c = value; *c != '\0'; c++)
    {
        const bool lower = *c >= 'a' && *c <= 'z';
        const bool upper = *c >= 'A' && *c <= 'Z';
        const bool digit = *c >= '0' && *c <= '9';
        if (lower || upper || digit)
        {
            token.push_back(*c);
        }
        else if (!token.empty())
        {
            tokens.push_back(token);
            token.clear();
        }
    }
    if (!token.empty())
    {
        tokens.push_back(token);
    }
    return tokens;
}



/**
 * Return whether a leading display token should be removed.
 *
 * @param token leading fixture-name token.
 * @param drop_tokens metadata tokens that are already shown in the fixture id.
 * @return true if the leading token is redundant.
 */
static bool
_tst_display_should_drop_token(const std::string& token, const std::vector<std::string>& drop_tokens)
{
    for (const std::string& drop_token : drop_tokens)
    {
        if (token == drop_token)
        {
            return true;
        }
        if (drop_token == "validation" && token == "validate")
        {
            return true;
        }
        if (drop_token == "emit" && token == "emitter")
        {
            return true;
        }
    }
    return false;
}



/**
 * Remove leading fixture-name tokens that are already present in module/group metadata.
 *
 * @param label display label to trim.
 * @param drop_tokens metadata tokens that are already shown in the fixture id.
 */
static void
_tst_drop_display_token_prefixes(std::string* label, const std::vector<std::string>& drop_tokens)
{
    ANN(label);
    while (!label->empty())
    {
        size_t separator = label->find('_');
        if (separator == std::string::npos)
        {
            break;
        }
        const std::string token = label->substr(0, separator);
        if (!_tst_display_should_drop_token(token, drop_tokens))
        {
            break;
        }
        label->erase(0, separator + 1);
    }
}



/**
 * Remove group-specific phrase prefixes that duplicate fixture metadata or local conventions.
 *
 * @param test test case.
 * @param label display label to trim.
 */
static void _tst_drop_display_phrase_prefixes(const TstCase* test, std::string* label)
{
    ANN(test);
    ANN(label);
    const std::string module = test->module != NULL ? test->module : "";
    const std::string group = test->group != NULL ? test->group : "";

    if (module == "scene" && group == "frame-plan")
    {
        _tst_drop_display_prefix(label, "graph_");
    }
    if (module == "scene" && group == "scene-graph")
    {
        _tst_drop_display_prefix(label, "visual_");
    }
    if (module == "scene" && group == "pick-probe")
    {
        _tst_drop_display_prefix(label, "process_requests_");
        _tst_drop_display_prefix(label, "process_pick_probe_");
        _tst_drop_display_prefix(label, "poll_pick_probe_");
        _tst_drop_display_prefix(label, "pick_request_");
        _tst_drop_display_prefix(label, "probe_request_");
        _tst_drop_display_prefix(label, "request_");
        _tst_drop_display_prefix(label, "image_probe_");
        _tst_drop_display_prefix(label, "point_pick_");
        _tst_drop_display_prefix(label, "pixel_pick_");
        _tst_drop_display_prefix(label, "marker_pick_");
    }
    if (module == "scene" && group == "fields")
    {
        _tst_drop_display_prefix(label, "image_field_");
        _tst_drop_display_prefix(label, "image_visual_");
        _tst_drop_display_prefix(label, "sampled_field_");
        _tst_drop_display_prefix(label, "visual_buffer_");
        _tst_drop_display_prefix(label, "visual_field_");
        _tst_drop_display_prefix(label, "visual_scale_");
        _tst_drop_display_prefix(label, "volume_field_");
        _tst_drop_display_prefix(label, "volume_visual_");
    }
    if (module == "scene" && group == "panzoom")
    {
        _tst_drop_display_prefix(label, "panel_panzoom_");
    }
    if (module == "scene" && group == "arcball")
    {
        _tst_drop_display_prefix(label, "camera_arcball_");
    }
    if (module == "canvas" && group == "default")
    {
        _tst_drop_display_prefix(label, "glfw_wrap_surface_");
    }
    if (module == "drp2" && group == "runtime-validation")
    {
        _tst_drop_display_prefix(label, "rejects_");
    }
    if (module == "drp2" && group == "vklite-runtime")
    {
        _tst_drop_display_prefix(label, "refreshes_bind_group_after_");
    }
    if (module == "drp2" && group == "render-pass")
    {
        _tst_drop_display_prefix(label, "begin_render_pass_");
    }
}



/**
 * Build the compact case label used by normal console output.
 *
 * @param test test case.
 * @return Display name with redundant C symbol prefixes removed.
 */
static std::string _tst_case_display_name(const TstCase* test)
{
    ANN(test);
    std::string name = test->name != NULL ? test->name : "unnamed";
    _tst_drop_display_prefix(&name, "test_");
    std::vector<std::string> drop_tokens = _tst_display_tokens(test->module);
    if (test->group != NULL)
    {
        std::vector<std::string> group_tokens = _tst_display_tokens(test->group);
        drop_tokens.insert(drop_tokens.end(), group_tokens.begin(), group_tokens.end());
    }
    _tst_drop_display_token_prefixes(&name, drop_tokens);
    _tst_drop_display_phrase_prefixes(test, &name);
    _tst_drop_display_token_prefixes(&name, drop_tokens);
    return name.empty() ? "unnamed" : name;
}



/**
 * Build the compact case identifier used by normal console output.
 *
 * @param test test case.
 * @return Display identifier in module/group/name form.
 */
static std::string _tst_case_display_id(const TstCase* test)
{
    ANN(test);
    std::string module = test->module != NULL ? test->module : "default";
    std::string group = test->group != NULL ? test->group : "default";
    if (group == "default")
    {
        return module + "/" + _tst_case_display_name(test);
    }
    return module + "/" + group + "/" + _tst_case_display_name(test);
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
        const double us = (double)ns / 1000.0;
        dvz_snprintf(buffer, sizeof(buffer), us < 10.0 ? "%.1f us" : "%.0f us", us);
    }
    else if (ns < 1000000000)
    {
        const double ms = (double)ns / 1000000.0;
        dvz_snprintf(buffer, sizeof(buffer), ms < 10.0 ? "%.1f ms" : "%.0f ms", ms);
    }
    else
    {
        const double s = (double)ns / 1000000000.0;
        dvz_snprintf(buffer, sizeof(buffer), s < 10.0 ? "%.1f s" : "%.0f s", s);
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



static const char* _tst_duration_color(uint64_t ns)
{
    if (ns >= 1000000000)
    {
        return "\x1b[1;36m";
    }
    if (ns >= 100000000)
    {
        return "\x1b[36m";
    }
    return "\x1b[90m";
}



static void _tst_print_duration(FILE* stream, const TstOptions* options, uint64_t ns, int width)
{
    ANN(stream);
    ANN(options);
    const std::string duration = _tst_duration(ns);
    if (_tst_use_color(options))
    {
        dvz_fprintf(stream, "%s%*s\x1b[0m", _tst_duration_color(ns), width, duration.c_str());
    }
    else
    {
        dvz_fprintf(stream, "%*s", width, duration.c_str());
    }
}



static void _tst_print_separator(FILE* stream)
{
    ANN(stream);
    for (int i = 0; i < TST_RESULT_SEPARATOR_WIDTH; i++)
    {
        dvz_fprintf(stream, "-");
    }
    dvz_fprintf(stream, "\n");
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



static TstStatus _tst_status_from_name(const char* name)
{
    if (_tst_streq(name, "PASS"))
        return TST_STATUS_PASS;
    if (_tst_streq(name, "FAIL"))
        return TST_STATUS_FAIL;
    if (_tst_streq(name, "SKIP"))
        return TST_STATUS_SKIP;
    return TST_STATUS_NOT_RUN;
}



/**
 * Return the ANSI color escape used for a test status.
 *
 * @param status test status value
 * @return ANSI color sequence, or an empty string for uncolored statuses
 */
static const char* _tst_status_color(TstStatus status)
{
    switch (status)
    {
    case TST_STATUS_PASS:
        return "\x1b[32m";
    case TST_STATUS_FAIL:
        return "\x1b[31m";
    case TST_STATUS_SKIP:
        return "\x1b[33m";
    case TST_STATUS_NOT_RUN:
    default:
        return "";
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



static TstIsolation _tst_isolation_from_name(const char* name)
{
    if (_tst_streq(name, "process"))
        return TST_ISOLATION_PROCESS;
    if (_tst_streq(name, "thread-safe"))
        return TST_ISOLATION_THREAD_SAFE;
    if (_tst_streq(name, "exclusive"))
        return TST_ISOLATION_EXCLUSIVE;
    return TST_ISOLATION_SERIAL;
}



static const char* _tst_fixture_scope_name(TstFixtureScope scope)
{
    switch (scope)
    {
    case TST_FIXTURE_SCOPE_CASE:
        return "case";
    case TST_FIXTURE_SCOPE_WORKER:
        return "worker";
    case TST_FIXTURE_SCOPE_PROCESS:
        return "process";
    case TST_FIXTURE_SCOPE_EXCLUSIVE:
        return "exclusive";
    case TST_FIXTURE_SCOPE_NONE:
    default:
        return "none";
    }
}



static TstFixtureScope _tst_fixture_scope_from_name(const char* name)
{
    if (_tst_streq(name, "case"))
        return TST_FIXTURE_SCOPE_CASE;
    if (_tst_streq(name, "worker"))
        return TST_FIXTURE_SCOPE_WORKER;
    if (_tst_streq(name, "process"))
        return TST_FIXTURE_SCOPE_PROCESS;
    if (_tst_streq(name, "exclusive"))
        return TST_FIXTURE_SCOPE_EXCLUSIVE;
    return TST_FIXTURE_SCOPE_NONE;
}



static std::vector<TstFixtureDef>* _tst_fixture_registry(TstSuite* suite, bool create)
{
    ANN(suite);
    if (suite->fixture_registry == NULL && create)
    {
        suite->fixture_registry = new std::vector<TstFixtureDef>();
        ANN(suite->fixture_registry);
    }
    return (std::vector<TstFixtureDef>*)suite->fixture_registry;
}



static size_t _tst_fixture_find(const std::vector<TstFixtureDef>* registry, const char* name)
{
    if (registry == NULL || name == NULL)
    {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < registry->size(); i++)
    {
        if ((*registry)[i].name == name)
        {
            return i;
        }
    }
    return SIZE_MAX;
}



static TstFixtureScope _tst_fixture_case_scope(const TstSuite* suite, const TstCase* test)
{
    ANN(test);
    if (test->fixture_scope != TST_FIXTURE_SCOPE_NONE)
    {
        return test->fixture_scope;
    }
    if (test->fixture == NULL)
    {
        return TST_FIXTURE_SCOPE_NONE;
    }

    const std::vector<TstFixtureDef>* registry =
        suite != NULL ? (const std::vector<TstFixtureDef>*)suite->fixture_registry : NULL;
    size_t idx = _tst_fixture_find(registry, test->fixture);
    if (idx == SIZE_MAX)
    {
        return TST_FIXTURE_SCOPE_NONE;
    }
    return (*registry)[idx].scope;
}



static void _tst_fixture_destroy_case(TstFixtureRunState* state)
{
    if (state == NULL || state->registry == NULL)
    {
        return;
    }
    for (const auto& item : state->case_instances)
    {
        const size_t idx = item.first;
        void* instance = item.second;
        if (idx < state->registry->size() && instance != NULL)
        {
            TstFixtureDef& def = (*(state->registry))[idx];
            if (def.destroy != NULL)
            {
                def.destroy(instance);
            }
        }
    }
    state->case_instances.clear();
}



static void _tst_fixture_destroy_all(TstSuite* suite)
{
    if (suite == NULL || suite->fixture_registry == NULL)
    {
        return;
    }

    std::vector<TstFixtureDef>* registry = (std::vector<TstFixtureDef>*)suite->fixture_registry;
    for (TstFixtureDef& def : *registry)
    {
        if (def.worker_created && def.worker_instance != NULL && def.destroy != NULL)
        {
            def.destroy(def.worker_instance);
        }
        def.worker_instance = NULL;
        def.worker_created = false;
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



static uint64_t _tst_resource_flag(const std::string& name)
{
    if (name == "cpu")
        return TST_RES_CPU;
    if (name == "gpu")
        return TST_RES_GPU;
    if (name == "vulkan")
        return TST_RES_VULKAN;
    if (name == "glfw")
        return TST_RES_GLFW;
    if (name == "filesystem")
        return TST_RES_FILESYSTEM;
    if (name == "env")
        return TST_RES_ENV;
    if (name == "video")
        return TST_RES_VIDEO;
    if (name == "log-capture")
        return TST_RES_LOG_CAPTURE;
    if (name == "global-state")
        return TST_RES_GLOBAL_STATE;
    return TST_RES_NONE;
}



static uint64_t _tst_resources_from_string(const std::string& value)
{
    if (value.empty() || value == "none")
    {
        return TST_RES_NONE;
    }
    uint64_t resources = TST_RES_NONE;
    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        resources |= _tst_resource_flag(token);
    }
    return resources;
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



static TstShardPolicy _tst_shard_policy_from_name(const char* name)
{
    if (_tst_streq(name, "parallel-safe"))
    {
        return TST_SHARD_POLICY_PARALLEL_SAFE;
    }
    if (_tst_streq(name, "serial-only"))
    {
        return TST_SHARD_POLICY_SERIAL_ONLY;
    }
    return TST_SHARD_POLICY_ALL;
}



static const char* _tst_shard_policy_name(TstShardPolicy policy)
{
    switch (policy)
    {
    case TST_SHARD_POLICY_PARALLEL_SAFE:
        return "parallel-safe";
    case TST_SHARD_POLICY_SERIAL_ONLY:
        return "serial-only";
    case TST_SHARD_POLICY_ALL:
    default:
        return "all";
    }
}



static bool _tst_case_parallel_safe(const TstSuite* suite, const TstCase* test)
{
    ANN(test);
    if (test->isolation == TST_ISOLATION_EXCLUSIVE)
    {
        return false;
    }

    const uint64_t serial_resources =
        TST_RES_GLFW | TST_RES_ENV | TST_RES_VIDEO | TST_RES_LOG_CAPTURE | TST_RES_GLOBAL_STATE;
    if ((test->resources & serial_resources) != 0)
    {
        return false;
    }

    const TstFixtureScope fixture_scope = _tst_fixture_case_scope(suite, test);
    if (fixture_scope == TST_FIXTURE_SCOPE_EXCLUSIVE)
    {
        return false;
    }

    return true;
}



static bool
_tst_shard_policy_matches(const TstSuite* suite, const TstCase* test, TstShardPolicy policy)
{
    const bool parallel_safe = _tst_case_parallel_safe(suite, test);
    switch (policy)
    {
    case TST_SHARD_POLICY_PARALLEL_SAFE:
        return parallel_safe;
    case TST_SHARD_POLICY_SERIAL_ONLY:
        return !parallel_safe;
    case TST_SHARD_POLICY_ALL:
    default:
        return true;
    }
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
    options.jobs = 1;
    options.shard_index = UINT32_MAX;
    options.shard_count = 0;
    options.selected_order_index = UINT64_MAX;
    options.repeat_index = UINT64_MAX;
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



static bool _tst_is_option_with_value(const char* arg)
{
    if (arg == NULL)
    {
        return false;
    }
    return _tst_streq(arg, "--module") || _tst_streq(arg, "--group") ||
           _tst_streq(arg, "--case") || _tst_streq(arg, "--tag") ||
           _tst_streq(arg, "--exclude-tag") || _tst_streq(arg, "--resource") ||
           _tst_streq(arg, "--isolation") || _tst_streq(arg, "--json") ||
           _tst_streq(arg, "--child-json") || _tst_streq(arg, "--parent-json") ||
           _tst_streq(arg, "--repeat") || _tst_streq(arg, "--seed") ||
           _tst_streq(arg, "--slow") || _tst_streq(arg, "--slow-groups") ||
           _tst_streq(arg, "--timeout") || _tst_streq(arg, "--jobs") ||
           _tst_streq(arg, "--shard-index") || _tst_streq(arg, "--shard-count") ||
           _tst_streq(arg, "--shard-policy") || _tst_streq(arg, "--color") ||
           _tst_streq(arg, "--selected-order-index") || _tst_streq(arg, "--repeat-index");
}



static bool _tst_parent_only_option(const char* arg)
{
    if (arg == NULL)
    {
        return false;
    }
    return _tst_streq(arg, "--jobs") || _tst_streq(arg, "--shard-index") ||
           _tst_streq(arg, "--shard-count") || _tst_streq(arg, "--child-json") ||
           _tst_streq(arg, "--parent-json") || _tst_streq(arg, "--json") ||
           _tst_streq(arg, "--shard-policy") || _tst_streq(arg, "--selected-order-index") ||
           _tst_streq(arg, "--repeat-index");
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
        "               [--slow-groups count] [--timeout ms]\n"
        "               [--jobs count] [--shard-index index --shard-count count]\n"
        "               [--child-json path] [--parent-json path]\n");
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
        else if (_tst_streq(arg, "--child-json") && i + 1 < argc)
            options->child_json_path = argv[++i];
        else if (_tst_streq(arg, "--parent-json") && i + 1 < argc)
            options->parent_json_path = argv[++i];
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
        else if (_tst_streq(arg, "--jobs") && i + 1 < argc)
            options->jobs = (uint32_t)std::max<uint64_t>(1, _tst_parse_u64(argv[++i], 1));
        else if (_tst_streq(arg, "--shard-index") && i + 1 < argc)
            options->shard_index = (uint32_t)_tst_parse_u64(argv[++i], UINT32_MAX);
        else if (_tst_streq(arg, "--shard-count") && i + 1 < argc)
            options->shard_count = (uint32_t)_tst_parse_u64(argv[++i], 0);
        else if (_tst_streq(arg, "--shard-policy") && i + 1 < argc)
            options->shard_policy = argv[++i];
        else if (_tst_streq(arg, "--selected-order-index") && i + 1 < argc)
            options->selected_order_index = _tst_parse_u64(argv[++i], UINT64_MAX);
        else if (_tst_streq(arg, "--repeat-index") && i + 1 < argc)
            options->repeat_index = _tst_parse_u64(argv[++i], UINT64_MAX);
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
        else if (_tst_streq(arg, "--process-child"))
            options->process_child = true;
        else if (arg[0] != '-' && options->positional_filter == NULL)
            options->positional_filter = arg;
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

    if (options->positional_filter != NULL)
    {
        std::string id = _tst_case_id(test);
        std::string display_id = _tst_case_display_id(test);
        if (!_tst_contains(id.c_str(), options->positional_filter) &&
            !_tst_contains(display_id.c_str(), options->positional_filter) &&
            !_tst_contains(test->function_name, options->positional_filter) &&
            !_tst_contains(test->tags, options->positional_filter))
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
        code = _tst_status_color(result->status);
        reset = "\x1b[0m";
    }

    const std::string id = _tst_case_display_id(result);
    dvz_fprintf(stdout, "%s%-4s%s  %-*s ", code, status, reset, TST_RESULT_NAME_WIDTH, id.c_str());
    _tst_print_duration(stdout, options, result->elapsed_ns, TST_RESULT_TIME_WIDTH);
    if (result->status == TST_STATUS_SKIP && result->skip_reason != NULL)
    {
        dvz_fprintf(stdout, "  %s", result->skip_reason);
    }
    dvz_fprintf(stdout, "\n");

    if (options->verbose || result->status == TST_STATUS_FAIL)
    {
        dvz_fprintf(stdout, "  function   %s\n", result->function_name);
        dvz_fprintf(stdout, "  resources  %s\n", _tst_resources_string(result->resources).c_str());
        dvz_fprintf(stdout, "  isolation  %s\n", _tst_isolation_name(result->isolation));
        if (result->fixture != NULL)
        {
            dvz_fprintf(stdout, "  fixture    %s (%s)\n", result->fixture,
                        _tst_fixture_scope_name(result->fixture_scope));
            if (result->fixture_setup_ns > 0)
            {
                dvz_fprintf(stdout, "  fixture setup ");
                _tst_print_duration(stdout, options, result->fixture_setup_ns, 0);
                dvz_fprintf(stdout, "\n");
            }
        }
        dvz_fprintf(stdout, "  elapsed    ");
        _tst_print_duration(stdout, options, result->elapsed_ns, 0);
        dvz_fprintf(stdout, "\n");
        if (result->timeout_ms > 0)
            dvz_fprintf(stdout, "  timeout    %" PRIu64 " ms\n", result->timeout_ms);
        if (result->skip_reason != NULL)
            dvz_fprintf(stdout, "  reason     %s\n", result->skip_reason);
    }
}



/**
 * Print one colored summary count when terminal colors are enabled.
 *
 * @param stream output stream
 * @param options runner options controlling color output
 * @param status status whose color should be used
 * @param value numeric count to print
 */
static void
_tst_print_summary_count(FILE* stream, const TstOptions* options, TstStatus status, uint32_t value)
{
    ANN(stream);
    ANN(options);
    if (_tst_use_color(options))
    {
        dvz_fprintf(stream, "%s%u\x1b[0m", _tst_status_color(status), value);
    }
    else
    {
        dvz_fprintf(stream, "%u", value);
    }
}



/**
 * Print a summary status phrase, coloring the whole phrase only for nonzero counts.
 *
 * @param stream output stream
 * @param options runner options controlling color output
 * @param status status whose color should be used
 * @param value numeric count to print
 * @param label status label to print after the count
 */
static void _tst_print_summary_status_phrase(
    FILE* stream, const TstOptions* options, TstStatus status, uint32_t value, const char* label)
{
    ANN(stream);
    ANN(options);
    ANN(label);
    if (value > 0 && _tst_use_color(options))
    {
        dvz_fprintf(stream, "%s%u %s\x1b[0m", _tst_status_color(status), value, label);
    }
    else
    {
        dvz_fprintf(stream, "%u %s", value, label);
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
    const std::map<std::string, TstAggregate>& modules, const TstOptions* options,
    const std::map<std::string, TstAggregate>& groups)
{
    ANN(summary);
    ANN(options);
    _tst_print_separator(stdout);
    _tst_print_summary_count(stdout, options, TST_STATUS_PASS, summary->passed_count);
    dvz_fprintf(stdout, "/%u tests passed, ", summary->selected_count);
    _tst_print_summary_status_phrase(
        stdout, options, TST_STATUS_FAIL, summary->failed_count, "failed");
    dvz_fprintf(stdout, ", ");
    _tst_print_summary_status_phrase(
        stdout, options, TST_STATUS_SKIP, summary->skipped_count, "skipped");
    dvz_fprintf(stdout, "\n");
    dvz_fprintf(stdout, "case time: ");
    _tst_print_duration(stdout, options, summary->summed_case_ns, 0);
    dvz_fprintf(stdout, ", runner time: ");
    _tst_print_duration(stdout, options, summary->runner_elapsed_ns, 0);
    dvz_fprintf(stdout, "\n");

    if (!modules.empty())
    {
        dvz_fprintf(stdout, "\nModules:\n");
        for (const auto& it : modules)
        {
            const TstAggregate& agg = it.second;
            dvz_fprintf(
                stdout, "  %-16s %3u selected, %3u failed, %3u skipped, ",
                it.first.c_str(), agg.selected_count, agg.failed_count, agg.skipped_count);
            _tst_print_duration(stdout, options, agg.summed_case_ns, TST_RESULT_TIME_WIDTH);
            dvz_fprintf(stdout, " summed\n");
        }
    }

    if (!groups.empty() && results.size() < 64)
    {
        dvz_fprintf(stdout, "\nGroups:\n");
        for (const auto& it : groups)
        {
            const TstAggregate& agg = it.second;
            dvz_fprintf(
                stdout, "  %-32s %3u selected, %3u failed, %3u skipped, ",
                it.first.c_str(), agg.selected_count, agg.failed_count, agg.skipped_count);
            _tst_print_duration(stdout, options, agg.summed_case_ns, TST_RESULT_TIME_WIDTH);
            dvz_fprintf(stdout, " summed\n");
        }
    }

    if (summary->failed_count > 0)
    {
        dvz_fprintf(stdout, "\nFailed tests:\n");
        for (const TstCase& result : results)
        {
            if (result.status == TST_STATUS_FAIL)
            {
                dvz_fprintf(stdout, "  - %s\n", _tst_case_display_id(&result).c_str());
            }
        }
    }
}



static void
_tst_print_slow_cases(const std::vector<TstCase>& results, const TstOptions* options, uint64_t count)
{
    ANN(options);
    if (count == 0 || results.empty())
    {
        return;
    }
    std::vector<TstCase> sorted = results;
    sorted.erase(
        std::remove_if(
            sorted.begin(), sorted.end(),
            [](const TstCase& result) { return result.status == TST_STATUS_SKIP; }),
        sorted.end());
    if (sorted.empty())
    {
        return;
    }
    std::sort(sorted.begin(), sorted.end(), [](const TstCase& a, const TstCase& b) {
        return a.elapsed_ns > b.elapsed_ns;
    });
    count = std::min<uint64_t>(count, sorted.size());
    dvz_fprintf(stdout, "\nSlowest tests:\n");
    for (uint64_t i = 0; i < count; ++i)
    {
        dvz_fprintf(
            stdout, "  %-*s ", TST_RESULT_NAME_WIDTH,
            _tst_case_display_id(&sorted[(size_t)i]).c_str());
        _tst_print_duration(stdout, options, sorted[(size_t)i].elapsed_ns, TST_RESULT_TIME_WIDTH);
        dvz_fprintf(stdout, "\n");
    }
}



static void
_tst_print_slow_groups(
    const std::map<std::string, TstAggregate>& groups, const TstOptions* options, uint64_t count)
{
    ANN(options);
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
        dvz_fprintf(stdout, "  %-32s ", sorted[(size_t)i].first.c_str());
        _tst_print_duration(
            stdout, options, sorted[(size_t)i].second.summed_case_ns, TST_RESULT_TIME_WIDTH);
        dvz_fprintf(stdout, "\n");
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
    out << "  \"schema_version\": 2,\n";
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
        out << "      \"case_id\": \"" << _tst_json_escape(_tst_case_id(&r).c_str()) << "\",\n";
        out << "      \"module\": \"" << _tst_json_escape(r.module) << "\",\n";
        out << "      \"group\": \"" << _tst_json_escape(r.group) << "\",\n";
        out << "      \"name\": \"" << _tst_json_escape(r.name) << "\",\n";
        out << "      \"function\": \"" << _tst_json_escape(r.function_name) << "\",\n";
        out << "      \"status\": \"" << _tst_status_name(r.status) << "\",\n";
        out << "      \"resources\": \"" << _tst_resources_string(r.resources) << "\",\n";
        out << "      \"isolation\": \"" << _tst_isolation_name(r.isolation) << "\",\n";
        if (r.fixture != NULL)
            out << "      \"fixture\": \"" << _tst_json_escape(r.fixture) << "\",\n";
        else
            out << "      \"fixture\": null,\n";
        out << "      \"fixture_scope\": \"" << _tst_fixture_scope_name(r.fixture_scope) << "\",\n";
        if (r.skip_reason != NULL)
            out << "      \"skip_reason\": \"" << _tst_json_escape(r.skip_reason) << "\",\n";
        else
            out << "      \"skip_reason\": null,\n";
        out << "      \"repeat_index\": " << r.repeat_index << ",\n";
        out << "      \"order_index\": " << r.order_index << ",\n";
        out << "      \"shard_index\": " << r.shard_index << ",\n";
        out << "      \"timeout_ms\": " << r.timeout_ms << ",\n";
        out << "      \"start_ns\": " << r.start_ns << ",\n";
        out << "      \"end_ns\": " << r.end_ns << ",\n";
        out << "      \"elapsed_ns\": " << r.elapsed_ns << ",\n";
        out << "      \"fixture_setup_ns\": " << r.fixture_setup_ns << "\n";
        out << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return 0;
}



static std::string _tst_json_line_string(const std::string& line)
{
    size_t colon = line.find(':');
    if (colon == std::string::npos)
    {
        return "";
    }
    size_t start = line.find('"', colon + 1);
    if (start == std::string::npos)
    {
        return "";
    }
    std::string out;
    bool escape = false;
    for (size_t i = start + 1; i < line.size(); i++)
    {
        const char c = line[i];
        if (escape)
        {
            switch (c)
            {
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                out.push_back(c);
                break;
            }
            escape = false;
            continue;
        }
        if (c == '\\')
        {
            escape = true;
            continue;
        }
        if (c == '"')
        {
            break;
        }
        out.push_back(c);
    }
    return out;
}



static uint64_t _tst_json_line_u64(const std::string& line)
{
    size_t colon = line.find(':');
    if (colon == std::string::npos)
    {
        return 0;
    }
    return _tst_parse_u64(line.c_str() + colon + 1, 0);
}



static void _tst_owned_result_refresh(TstOwnedResult* owned)
{
    ANN(owned);
    owned->test.module = _tst_nonnull(owned->module);
    owned->test.group = _tst_nonnull(owned->group);
    owned->test.name = _tst_nonnull(owned->name);
    owned->test.function_name = _tst_nonnull(owned->function_name);
    owned->test.tags = _tst_nonnull(owned->tags);
    owned->test.fixture = _tst_nonnull(owned->fixture);
    owned->test.skip_reason = _tst_nonnull(owned->skip_reason);
}



static int _tst_read_json_results(const char* path, std::vector<TstOwnedResult>* out)
{
    ANN(path);
    ANN(out);
    std::ifstream in(path);
    if (!in)
    {
        dvz_fprintf(stderr, "could not read JSON test results from %s\n", path);
        return 1;
    }

    bool in_cases = false;
    bool in_case = false;
    TstOwnedResult current = {};
    std::string line;
    while (std::getline(in, line))
    {
        if (!in_cases)
        {
            if (line.find("\"cases\"") != std::string::npos)
            {
                in_cases = true;
            }
            continue;
        }
        if (!in_case)
        {
            if (line.find('{') != std::string::npos)
            {
                current = {};
                in_case = true;
            }
            continue;
        }
        if (line.find('}') != std::string::npos)
        {
            _tst_owned_result_refresh(&current);
            out->push_back(current);
            in_case = false;
            continue;
        }

        if (line.find("\"module\"") != std::string::npos)
            current.module = _tst_json_line_string(line);
        else if (line.find("\"group\"") != std::string::npos)
            current.group = _tst_json_line_string(line);
        else if (line.find("\"name\"") != std::string::npos)
            current.name = _tst_json_line_string(line);
        else if (line.find("\"function\"") != std::string::npos)
            current.function_name = _tst_json_line_string(line);
        else if (line.find("\"status\"") != std::string::npos)
            current.test.status = _tst_status_from_name(_tst_json_line_string(line).c_str());
        else if (line.find("\"resources\"") != std::string::npos)
            current.test.resources = _tst_resources_from_string(_tst_json_line_string(line));
        else if (line.find("\"isolation\"") != std::string::npos)
            current.test.isolation = _tst_isolation_from_name(_tst_json_line_string(line).c_str());
        else if (line.find("\"fixture_scope\"") != std::string::npos)
            current.test.fixture_scope =
                _tst_fixture_scope_from_name(_tst_json_line_string(line).c_str());
        else if (line.find("\"fixture\"") != std::string::npos &&
                 line.find("null") == std::string::npos)
            current.fixture = _tst_json_line_string(line);
        else if (line.find("\"skip_reason\"") != std::string::npos &&
                 line.find("null") == std::string::npos)
            current.skip_reason = _tst_json_line_string(line);
        else if (line.find("\"repeat_index\"") != std::string::npos)
            current.test.repeat_index = _tst_json_line_u64(line);
        else if (line.find("\"order_index\"") != std::string::npos)
            current.test.order_index = _tst_json_line_u64(line);
        else if (line.find("\"shard_index\"") != std::string::npos)
            current.test.shard_index = (uint32_t)_tst_json_line_u64(line);
        else if (line.find("\"timeout_ms\"") != std::string::npos)
            current.test.timeout_ms = _tst_json_line_u64(line);
        else if (line.find("\"start_ns\"") != std::string::npos)
            current.test.start_ns = _tst_json_line_u64(line);
        else if (line.find("\"end_ns\"") != std::string::npos)
            current.test.end_ns = _tst_json_line_u64(line);
        else if (line.find("\"elapsed_ns\"") != std::string::npos)
            current.test.elapsed_ns = _tst_json_line_u64(line);
        else if (line.find("\"fixture_setup_ns\"") != std::string::npos)
            current.test.fixture_setup_ns = _tst_json_line_u64(line);
    }
    return 0;
}



static int _tst_read_json_summary(const char* path, TstRunSummary* summary)
{
    ANN(path);
    ANN(summary);
    std::ifstream in(path);
    if (!in)
    {
        dvz_fprintf(stderr, "could not read JSON test summary from %s\n", path);
        return 1;
    }

    *summary = {};
    bool in_summary = false;
    std::string line;
    while (std::getline(in, line))
    {
        if (!in_summary)
        {
            if (line.find("\"summary\"") != std::string::npos)
            {
                in_summary = true;
            }
            continue;
        }
        if (line.find('}') != std::string::npos)
        {
            return 0;
        }

        if (line.find("\"selected\"") != std::string::npos)
            summary->selected_count = (uint32_t)_tst_json_line_u64(line);
        else if (line.find("\"passed\"") != std::string::npos)
            summary->passed_count = (uint32_t)_tst_json_line_u64(line);
        else if (line.find("\"failed\"") != std::string::npos)
            summary->failed_count = (uint32_t)_tst_json_line_u64(line);
        else if (line.find("\"skipped\"") != std::string::npos)
            summary->skipped_count = (uint32_t)_tst_json_line_u64(line);
        else if (line.find("\"summed_case_ns\"") != std::string::npos)
            summary->summed_case_ns = _tst_json_line_u64(line);
        else if (line.find("\"runner_elapsed_ns\"") != std::string::npos)
            summary->runner_elapsed_ns = _tst_json_line_u64(line);
    }
    return 1;
}



static void _tst_rebuild_summary(
    const std::vector<TstCase>& results, uint64_t runner_elapsed_ns, TstRunSummary* summary,
    std::map<std::string, TstAggregate>* modules, std::map<std::string, TstAggregate>* groups)
{
    ANN(summary);
    ANN(modules);
    ANN(groups);
    *summary = {};
    modules->clear();
    groups->clear();
    for (const TstCase& result : results)
    {
        _tst_update_aggregate(&(*modules)[result.module != NULL ? result.module : "default"], &result);
        _tst_update_aggregate(
            &(*groups)[_tst_case_id(&result).substr(0, _tst_case_id(&result).rfind('/'))], &result);
        summary->selected_count++;
        if (result.status == TST_STATUS_PASS)
            summary->passed_count++;
        else if (result.status == TST_STATUS_FAIL)
            summary->failed_count++;
        else if (result.status == TST_STATUS_SKIP)
            summary->skipped_count++;
        summary->summed_case_ns += result.elapsed_ns;
    }
    summary->runner_elapsed_ns = runner_elapsed_ns;
}



static std::string _tst_child_json_path(uint32_t shard_index)
{
    const char* tmp = std::getenv("TMPDIR");
    if (tmp == NULL || tmp[0] == '\0')
    {
        tmp = "/tmp";
    }
    std::ostringstream ss;
    ss << tmp << "/dvztest-shard-";
#if defined(_WIN32)
    ss << _getpid();
#else
    ss << getpid();
#endif
    ss << "-" << _tst_now_ns() << "-" << shard_index << ".json";
    return ss.str();
}



static std::vector<std::string> _tst_child_args(
    int argc, char** argv, uint32_t shard_index, uint32_t shard_count, const std::string& json_path,
    TstShardPolicy policy)
{
    std::vector<std::string> args;
    args.push_back(argc > 0 && argv[0] != NULL ? argv[0] : "");
    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if (_tst_parent_only_option(arg))
        {
            if (_tst_is_option_with_value(arg) && i + 1 < argc)
            {
                i++;
            }
            continue;
        }
        args.push_back(arg != NULL ? arg : "");
    }
    args.push_back("--shard-index");
    args.push_back(std::to_string(shard_index));
    args.push_back("--shard-count");
    args.push_back(std::to_string(shard_count));
    args.push_back("--child-json");
    args.push_back(json_path);
    args.push_back("--shard-policy");
    args.push_back(_tst_shard_policy_name(policy));
    args.push_back("--color");
    args.push_back("never");
    return args;
}



static bool _tst_process_child_skip_option(const char* arg)
{
    return _tst_parent_only_option(arg) || _tst_streq(arg, "--repeat");
}



static std::vector<std::string> _tst_process_child_args(
    int argc, char** argv, uint64_t order_index, uint64_t repeat_index,
    const std::string& json_path)
{
    std::vector<std::string> args;
    args.push_back(argc > 0 && argv[0] != NULL ? argv[0] : "");
    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if (_tst_process_child_skip_option(arg))
        {
            if (_tst_is_option_with_value(arg) && i + 1 < argc)
            {
                i++;
            }
            continue;
        }
        args.push_back(arg != NULL ? arg : "");
    }
    args.push_back("--process-child");
    args.push_back("--selected-order-index");
    args.push_back(std::to_string(order_index));
    args.push_back("--repeat");
    args.push_back("1");
    args.push_back("--repeat-index");
    args.push_back(std::to_string(repeat_index));
    args.push_back("--child-json");
    args.push_back(json_path);
    args.push_back("--color");
    args.push_back("never");
    return args;
}



static int _tst_spawn_child(const std::vector<std::string>& args, TstChildProc* child)
{
    ANN(child);
    std::vector<char*> cargs;
    cargs.reserve(args.size() + 1);
    for (const std::string& arg : args)
    {
        cargs.push_back(const_cast<char*>(arg.c_str()));
    }
    cargs.push_back(NULL);

#if defined(_WIN32)
    intptr_t pid = _spawnv(_P_NOWAIT, args[0].c_str(), cargs.data());
    if (pid == -1)
    {
        return 1;
    }
    child->pid = pid;
#else
    pid_t pid = fork();
    if (pid < 0)
    {
        return 1;
    }
    if (pid == 0)
    {
        execv(args[0].c_str(), cargs.data());
        _exit(127);
    }
    child->pid = pid;
#endif
    return 0;
}



static int _tst_wait_child(TstChildProc* child)
{
    ANN(child);
#if defined(_WIN32)
    int status = 0;
    if (_cwait(&status, child->pid, 0) == -1)
    {
        return 1;
    }
    return status;
#else
    int status = 0;
    if (waitpid(child->pid, &status, 0) < 0)
    {
        return 1;
    }
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    return 1;
#endif
}



static int _tst_run_process_child_case(
    int argc, char** argv, uint64_t order_index, uint64_t repeat_index, TstOwnedResult* result)
{
    ANN(result);

    const std::string json_path = _tst_child_json_path((uint32_t)(order_index & UINT32_MAX));
    std::vector<std::string> args =
        _tst_process_child_args(argc, argv, order_index, repeat_index, json_path);

    TstChildProc child = {};
    child.json_path = json_path;
    if (_tst_spawn_child(args, &child) != 0)
    {
        dvz_fprintf(stderr, "could not spawn process-isolated case %" PRIu64 "\n", order_index);
        return 1;
    }

    int status = _tst_wait_child(&child);
    if (status != 0)
    {
        dvz_fprintf(
            stderr, "process-isolated case %" PRIu64 " failed with exit code %d\n", order_index,
            status);
    }

    std::vector<TstOwnedResult> owned;
    int read_res = _tst_read_json_results(json_path.c_str(), &owned);
    std::remove(json_path.c_str());
    if (read_res != 0)
    {
        return 1;
    }
    if (owned.size() != 1)
    {
        dvz_fprintf(
            stderr, "process-isolated case %" PRIu64 " produced %zu JSON results\n", order_index,
            owned.size());
        return 1;
    }

    *result = owned[0];
    _tst_owned_result_refresh(result);
    return status == 0 ? 0 : 1;
}



static uint32_t
_tst_count_policy_cases(TstSuite* suite, const TstOptions* options, TstShardPolicy policy)
{
    ANN(suite);
    ANN(options);
    uint32_t count = 0;
    for (uint32_t i = 0; i < suite->n_cases; i++)
    {
        TstCase* test = &suite->cases[i];
        if (_tst_case_matches(test, options) && _tst_shard_policy_matches(suite, test, policy))
        {
            count++;
        }
    }
    return count;
}



static void _tst_shard_progress_update(
    TstShardProgress* progress, const TstRunSummary* child_summary)
{
    ANN(progress);
    ANN(child_summary);
    progress->completed_shards++;
    progress->completed_cases += child_summary->selected_count;
    progress->failed_cases += child_summary->failed_count;
    progress->skipped_cases += child_summary->skipped_count;
}



static void _tst_print_shard_progress(
    const TstShardProgress* progress, const TstChildProc* child, bool final)
{
    ANN(progress);
    ANN(child);

    const double fail_pct =
        progress->completed_cases > 0
            ? (100.0 * (double)progress->failed_cases / (double)progress->completed_cases)
            : 0.0;
    const uint32_t bar_width = 24;
    const uint32_t filled =
        progress->total_cases > 0
            ? (uint32_t)((progress->completed_cases * bar_width) / progress->total_cases)
            : bar_width;

    dvz_fprintf(stderr, "\r%s shard %u/%u [", _tst_shard_policy_name(child->policy),
                child->shard_index + 1, child->shard_count);
    for (uint32_t i = 0; i < bar_width; i++)
    {
        dvz_fprintf(stderr, "%c", i < filled ? '#' : '-');
    }
    dvz_fprintf(
        stderr,
        "] shards %u/%u | cases %" PRIu64 "/%" PRIu64 " | fail %" PRIu64 "/%" PRIu64
        " (%.1f%%) | skipped %" PRIu64 "          ",
        progress->completed_shards, progress->total_shards, progress->completed_cases,
        progress->total_cases, progress->failed_cases, progress->completed_cases, fail_pct,
        progress->skipped_cases);
    if (final)
    {
        dvz_fprintf(stderr, "\n");
    }
    fflush(stderr);
}



static int _tst_run_parent_shards(
    TstSuite* suite, int argc, char** argv, const TstOptions* options, uint64_t runner_start_ns)
{
    ANN(suite);
    ANN(options);
    std::vector<TstChildProc> running;
    std::vector<TstOwnedResult> owned;
    TstShardProgress progress = {};
    int child_failure = 0;

    auto run_phase = [&](TstShardPolicy policy, uint32_t shard_count, uint32_t max_jobs) {
        uint32_t next_shard = 0;
        while (next_shard < shard_count || !running.empty())
        {
            while (next_shard < shard_count && running.size() < max_jobs)
            {
                const std::string json_path = _tst_child_json_path(next_shard);
                std::vector<std::string> args =
                    _tst_child_args(argc, argv, next_shard, shard_count, json_path, policy);
                TstChildProc child = {};
                child.shard_index = next_shard;
                child.shard_count = shard_count;
                child.policy = policy;
                child.json_path = json_path;
                if (_tst_spawn_child(args, &child) != 0)
                {
                    dvz_fprintf(stderr, "could not spawn shard %u\n", next_shard);
                    child_failure = 1;
                    next_shard = shard_count;
                    break;
                }
                running.push_back(child);
                next_shard++;
            }

            if (running.empty())
            {
                break;
            }

            TstChildProc child = running.front();
            running.erase(running.begin());
            int status = _tst_wait_child(&child);
            if (status != 0)
            {
                dvz_fprintf(stderr, "shard %u failed with exit code %d\n", child.shard_index, status);
                child_failure = 1;
            }

            TstRunSummary child_summary = {};
            if (_tst_read_json_summary(child.json_path.c_str(), &child_summary) != 0)
            {
                child_failure = 1;
            }
            if (_tst_read_json_results(child.json_path.c_str(), &owned) != 0)
            {
                child_failure = 1;
            }
            std::remove(child.json_path.c_str());
            _tst_shard_progress_update(&progress, &child_summary);
            _tst_print_shard_progress(
                &progress, &child, progress.completed_shards == progress.total_shards);
        }
    };

    const uint32_t parallel_count =
        _tst_count_policy_cases(suite, options, TST_SHARD_POLICY_PARALLEL_SAFE);
    const uint32_t serial_count =
        _tst_count_policy_cases(suite, options, TST_SHARD_POLICY_SERIAL_ONLY);
    progress.total_cases = (uint64_t)(parallel_count + serial_count) * options->repeat;
    progress.total_shards = (parallel_count > 0 ? options->jobs : 0) + (serial_count > 0 ? 1u : 0u);

    if (parallel_count > 0)
        run_phase(TST_SHARD_POLICY_PARALLEL_SAFE, options->jobs, std::max<uint32_t>(1, options->jobs));
    if (serial_count > 0)
        run_phase(TST_SHARD_POLICY_SERIAL_ONLY, 1, 1);

    for (TstChildProc& child : running)
    {
        int status = _tst_wait_child(&child);
        if (status != 0)
        {
            dvz_fprintf(stderr, "shard %u failed with exit code %d\n", child.shard_index, status);
            child_failure = 1;
        }

        TstRunSummary child_summary = {};
        if (_tst_read_json_summary(child.json_path.c_str(), &child_summary) != 0)
        {
            child_failure = 1;
        }
        if (_tst_read_json_results(child.json_path.c_str(), &owned) != 0)
        {
            child_failure = 1;
        }
        std::remove(child.json_path.c_str());
        _tst_shard_progress_update(&progress, &child_summary);
        _tst_print_shard_progress(
            &progress, &child, progress.completed_shards == progress.total_shards);
    }
    std::sort(owned.begin(), owned.end(), [](const TstOwnedResult& a, const TstOwnedResult& b) {
        if (a.test.repeat_index != b.test.repeat_index)
            return a.test.repeat_index < b.test.repeat_index;
        return a.test.order_index < b.test.order_index;
    });
    for (TstOwnedResult& item : owned)
    {
        _tst_owned_result_refresh(&item);
    }

    std::vector<TstCase> results;
    results.reserve(owned.size());
    for (const TstOwnedResult& item : owned)
    {
        results.push_back(item.test);
    }

    TstRunSummary summary = {};
    std::map<std::string, TstAggregate> modules;
    std::map<std::string, TstAggregate> groups;
    _tst_rebuild_summary(
        results, _tst_now_ns() - runner_start_ns, &summary, &modules, &groups);
    suite->last_summary = summary;

    _tst_print_summary(&summary, results, modules, options, groups);
    _tst_print_slow_cases(results, options, options->slow_count);
    _tst_print_slow_groups(groups, options, options->slow_group_count);

    const char* json_path = options->parent_json_path != NULL ? options->parent_json_path
                                                              : options->json_path;
    if (_tst_write_json(json_path, &summary, results) != 0)
    {
        return 1;
    }
    return child_failure == 0 && summary.failed_count == 0 ? 0 : 1;
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
            stdout, "%s  function=%s resources=%s isolation=%s fixture=%s fixture_scope=%s\n",
            _tst_case_display_id(test).c_str(), test->function_name != NULL ? test->function_name : "",
            _tst_resources_string(test->resources).c_str(), _tst_isolation_name(test->isolation),
            test->fixture != NULL ? test->fixture : "",
            _tst_fixture_scope_name(_tst_fixture_case_scope(suite, test)));
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
    test->skip = desc.skip;
    test->fixture = desc.fixture;
    test->fixture_scope = desc.fixture_scope;
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



void tst_suite_register_fixture(
    TstSuite* suite, const char* name, TstFixtureScope scope, TstFixtureCreate create,
    TstFixtureDestroy destroy)
{
    ANN(suite);
    ANN(name);
    ANN(create);
    std::vector<TstFixtureDef>* registry = _tst_fixture_registry(suite, true);
    ANN(registry);

    size_t idx = _tst_fixture_find(registry, name);
    if (idx == SIZE_MAX)
    {
        TstFixtureDef def = {};
        def.name = name;
        def.scope = scope;
        def.create = create;
        def.destroy = destroy;
        registry->push_back(def);
        return;
    }

    TstFixtureDef& def = (*registry)[idx];
    ASSERT(!def.worker_created);
    def.scope = scope;
    def.create = create;
    def.destroy = destroy;
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

    const bool child_mode = options.shard_count > 0;
    const TstShardPolicy shard_policy = _tst_shard_policy_from_name(options.shard_policy);
    if (options.jobs > 1 && !child_mode)
    {
        return _tst_run_parent_shards(suite, argc, argv, &options, _tst_now_ns());
    }
    if (child_mode && options.shard_index >= options.shard_count)
    {
        dvz_fprintf(stderr, "invalid shard index %u for shard count %u\n", options.shard_index,
                    options.shard_count);
        return 1;
    }

    std::vector<TstSelectedCase> selected;
    for (uint32_t i = 0; i < suite->n_cases; ++i)
    {
        TstCase* test = &suite->cases[i];
        if (_tst_case_matches(test, &options))
        {
            TstSelectedCase item = {};
            item.test = test;
            item.order_index = selected.size();
            selected.push_back(item);
        }
    }

    if (options.shuffle && selected.size() > 1)
    {
        std::mt19937_64 rng(options.seed);
        std::shuffle(selected.begin(), selected.end(), rng);
    }
    for (size_t i = 0; i < selected.size(); i++)
    {
        selected[i].order_index = i;
    }
    if (child_mode)
    {
        selected.erase(
            std::remove_if(
                selected.begin(), selected.end(), [&options](const TstSelectedCase& item) {
                    return item.order_index % options.shard_count != options.shard_index;
                }),
            selected.end());
    }
    if (shard_policy != TST_SHARD_POLICY_ALL)
    {
        selected.erase(
            std::remove_if(
                selected.begin(), selected.end(),
                [suite, shard_policy](const TstSelectedCase& item) {
                    return !_tst_shard_policy_matches(suite, item.test, shard_policy);
                }),
            selected.end());
    }
    if (options.process_child)
    {
        if (options.selected_order_index == UINT64_MAX)
        {
            dvz_fprintf(stderr, "--process-child requires --selected-order-index\n");
            return 1;
        }
        selected.erase(
            std::remove_if(
                selected.begin(), selected.end(), [&options](const TstSelectedCase& item) {
                    return item.order_index != options.selected_order_index;
                }),
            selected.end());
    }

    TstRunSummary summary = {};
    std::vector<TstCase> results;
    std::deque<TstOwnedResult> process_owned;
    std::map<std::string, TstAggregate> modules;
    std::map<std::string, TstAggregate> groups;
    TstFixtureRunState fixture_state = {};
    fixture_state.suite = suite;
    fixture_state.registry = _tst_fixture_registry(suite, false);
    fixture_state.worker_index = 0;
    const bool suppress_output = options.child_json_path != NULL;
    const uint64_t runner_start_ns = _tst_now_ns();
    auto record_result = [&](const TstCase& result) {
        if (!suppress_output)
        {
            _tst_print_case(&result, &options);
        }
        _tst_update_aggregate(&modules[result.module != NULL ? result.module : "default"], &result);
        _tst_update_aggregate(
            &groups[_tst_case_id(&result).substr(0, _tst_case_id(&result).rfind('/'))], &result);

        summary.selected_count++;
        if (result.status == TST_STATUS_PASS)
            summary.passed_count++;
        else if (result.status == TST_STATUS_FAIL)
            summary.failed_count++;
        else if (result.status == TST_STATUS_SKIP)
            summary.skipped_count++;
        summary.summed_case_ns += result.elapsed_ns;

        results.push_back(result);
    };

    for (uint64_t repeat = 0; repeat < options.repeat; ++repeat)
    {
        for (const TstSelectedCase& selected_case : selected)
        {
            TstCase* test = selected_case.test;
            TstCase result = *test;
            result.repeat_index = options.repeat_index != UINT64_MAX ? options.repeat_index : repeat;
            result.order_index = selected_case.order_index;
            result.shard_index = child_mode ? options.shard_index : 0;
            result.fixture_scope = _tst_fixture_case_scope(suite, &result);
            if (result.timeout_ms == 0)
            {
                result.timeout_ms = options.default_timeout_ms;
            }

            if (!child_mode && !options.process_child && result.isolation == TST_ISOLATION_PROCESS)
            {
                TstOwnedResult child_result = {};
                const uint64_t start_ns = _tst_now_ns();
                int child_res = _tst_run_process_child_case(
                    argc, argv, selected_case.order_index, result.repeat_index, &child_result);
                if (child_result.test.name != NULL)
                {
                    process_owned.push_back(child_result);
                    _tst_owned_result_refresh(&process_owned.back());
                    result = process_owned.back().test;
                }
                else
                {
                    result.start_ns = start_ns;
                    result.end_ns = _tst_now_ns();
                    result.elapsed_ns = result.end_ns - result.start_ns;
                    result.result = 1;
                    result.status = TST_STATUS_FAIL;
                }
                if (child_res != 0 && result.status == TST_STATUS_PASS)
                {
                    result.result = 1;
                    result.status = TST_STATUS_FAIL;
                }

                record_result(result);
                if (options.fail_fast && result.status == TST_STATUS_FAIL)
                {
                    break;
                }
                continue;
            }

            TstContext ctx = {};
            ctx.suite = suite;
            ctx.test = &result;
            ctx.user_data = result.user_data;
            ctx.suppress_expected_error_output = true;
            ctx.strict_unexpected_errors = suite->strict_unexpected_errors;
            ctx.worker_index = fixture_state.worker_index;
            ctx.fixture_state = &fixture_state;

            result.start_ns = _tst_now_ns();

            const char* skip_reason = NULL;
            if (result.skip != NULL)
            {
                skip_reason = result.skip(&ctx, &result);
            }

            if (skip_reason == NULL && suite->log_adapter.install != NULL)
            {
                suite->log_adapter.install(&ctx, suite->log_adapter.user_data);
            }

            int res = 0;
            if (skip_reason != NULL)
            {
                result.skip_reason = skip_reason;
            }
            else if (result.setup != NULL)
            {
                res = result.setup(&ctx, &result);
                if (ctx.skip_reason != NULL)
                {
                    result.skip_reason = ctx.skip_reason;
                }
            }
            if (skip_reason == NULL && result.skip_reason == NULL && res == 0)
            {
                res = result.test(&ctx, &result);
                if (ctx.skip_reason != NULL)
                {
                    result.skip_reason = ctx.skip_reason;
                }
            }
            if (skip_reason == NULL && result.skip_reason == NULL && result.teardown != NULL)
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
            result.fixture_setup_ns = ctx.fixture_setup_ns;
            if (result.timeout_ms > 0 && result.elapsed_ns > result.timeout_ms * 1000000ull)
            {
                res = 1;
            }
            result.result = res;
            result.status = result.skip_reason != NULL ? TST_STATUS_SKIP
                                                       : (res == 0 ? TST_STATUS_PASS : TST_STATUS_FAIL);

            if (skip_reason == NULL && suite->log_adapter.uninstall != NULL)
            {
                suite->log_adapter.uninstall(suite->log_adapter.user_data);
            }

            record_result(result);
            _tst_fixture_destroy_case(&fixture_state);
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
    _tst_fixture_destroy_all(suite);

    if (!suppress_output)
    {
        _tst_print_summary(&summary, results, modules, &options, groups);
        _tst_print_slow_cases(results, &options, options.slow_count);
        _tst_print_slow_groups(groups, &options, options.slow_group_count);
    }

    const char* json_path = options.child_json_path != NULL ? options.child_json_path : options.json_path;
    if (_tst_write_json(json_path, &summary, results) != 0)
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
    ctx->expect_log_level = ctx->suite != NULL ? ctx->suite->log_adapter.error_level : 3;
}



void tst_expect_log_begin(TstContext* ctx, int level)
{
    ANN(ctx);
    ctx->expect_error_active = true;
    ctx->expect_error_seen = false;
    ctx->expect_log_level = level;
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



void tst_skip(TstContext* ctx, const char* reason)
{
    ANN(ctx);
    ctx->skip_reason = reason != NULL ? reason : "skipped";
}



void tst_set_strict_unexpected_errors(TstSuite* suite, bool enabled)
{
    ANN(suite);
    suite->strict_unexpected_errors = enabled;
}



void* tst_context_fixture(TstContext* ctx, const char* name)
{
    ANN(ctx);
    ANN(name);

    TstFixtureRunState* state = (TstFixtureRunState*)ctx->fixture_state;
    if (state == NULL || state->registry == NULL)
    {
        return NULL;
    }

    const size_t idx = _tst_fixture_find(state->registry, name);
    if (idx == SIZE_MAX)
    {
        return NULL;
    }

    TstFixtureDef& def = (*(state->registry))[idx];
    if (def.scope == TST_FIXTURE_SCOPE_CASE)
    {
        for (const auto& item : state->case_instances)
        {
            if (item.first == idx)
            {
                return item.second;
            }
        }
        const uint64_t start_ns = _tst_now_ns();
        void* instance = def.create != NULL ? def.create(state->suite, state->worker_index) : NULL;
        ctx->fixture_setup_ns += _tst_now_ns() - start_ns;
        state->case_instances.push_back(std::make_pair(idx, instance));
        return instance;
    }

    if (def.scope == TST_FIXTURE_SCOPE_WORKER || def.scope == TST_FIXTURE_SCOPE_PROCESS ||
        def.scope == TST_FIXTURE_SCOPE_EXCLUSIVE)
    {
        if (!def.worker_created)
        {
            const uint64_t start_ns = _tst_now_ns();
            def.worker_instance =
                def.create != NULL ? def.create(state->suite, state->worker_index) : NULL;
            ctx->fixture_setup_ns += _tst_now_ns() - start_ns;
            def.worker_created = true;
        }
        return def.worker_instance;
    }

    return NULL;
}



int tst_context_log(TstContext* ctx, int level, const char* file, int line, const char* message)
{
    if (ctx == NULL)
    {
        return 0;
    }
    _tst_log_capture_append(ctx, level, file, line, message);
    const int error_level = ctx->suite != NULL ? ctx->suite->log_adapter.error_level : 3;
    if (ctx->expect_error_active && level >= ctx->expect_log_level)
    {
        ctx->expect_error_seen = true;
        return ctx->suppress_expected_error_output ? 1 : 0;
    }
    if (level >= error_level)
    {
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
    _tst_fixture_destroy_all(suite);
    if (suite->fixture_registry != NULL)
    {
        std::vector<TstFixtureDef>* registry = (std::vector<TstFixtureDef>*)suite->fixture_registry;
        delete registry;
        suite->fixture_registry = NULL;
    }
    suite->n_cases = 0;
    suite->capacity = 0;
    dvz_free_ptr((void**)&suite->cases);
}
