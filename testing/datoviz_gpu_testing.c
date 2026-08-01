/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz GPU testing adapter                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz_gpu_selection.h"
#include "datoviz_testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzTestingGpuState
{
    uint32_t requested_index;
    uint32_t available_count;
    DvzTestingGpuSource source;
    DvzGpuInfo info;
    bool cli_seen;
    bool explicit_selection;
    bool list_gpus;
    bool list_mode;
    bool child_process;
    bool resolved;
} DvzTestingGpuState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a portable name for a Vulkan physical-device type.
 *
 * @param type Vulkan physical-device type
 * @return static device-type name
 */
static const char* _dvz_testing_gpu_type_name(VkPhysicalDeviceType type)
{
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        return "other";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "cpu";
    default:
        return "unknown";
    }
}



/**
 * Print every physical device visible through a Datoviz Vulkan instance.
 *
 * @param instance Datoviz instance
 * @param count enumerated GPU count
 */
static void _dvz_testing_gpu_print_available(DvzInstance* instance, uint32_t count)
{
    ANN(instance);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzGpuInfo info = {0};
        if (!dvz_instance_gpu_info(instance, i, &info))
        {
            continue;
        }
        dvz_fprintf(
            stdout,
            "[%u] name=\"%s\" type=%s(%d) vendor_id=0x%04x device_id=0x%04x "
            "api=%u.%u.%u api_version_raw=%u driver_version_raw=%u\n",
            info.index, info.name, _dvz_testing_gpu_type_name(info.device_type),
            (int)info.device_type, info.vendor_id, info.device_id,
            VK_API_VERSION_MAJOR(info.api_version), VK_API_VERSION_MINOR(info.api_version),
            VK_API_VERSION_PATCH(info.api_version), info.api_version, info.driver_version);
    }
}



/**
 * Enumerate and optionally resolve the requested physical device.
 *
 * @param state adapter state
 * @param discovery whether to print discovery output only
 * @return zero on success
 */
static int _dvz_testing_gpu_resolve(DvzTestingGpuState* state, bool discovery)
{
    ANN(state);
    DvzInstanceConfig config = dvz_instance_config();
    DvzInstance* instance = dvz_instance_create(&config);
    if (instance == NULL)
    {
        if (discovery || state->explicit_selection)
        {
            dvz_fprintf(stderr, "unable to create a Vulkan instance for GPU selection\n");
            return 1;
        }
        return 0;
    }

    uint32_t count = dvz_instance_gpu_count(instance);
    state->available_count = count;
    if (discovery)
    {
        if (count == 0)
        {
            dvz_fprintf(stderr, "no Vulkan physical devices were found\n");
            dvz_instance_destroy(instance);
            return 1;
        }
        _dvz_testing_gpu_print_available(instance, count);
        dvz_instance_destroy(instance);
        return 0;
    }

    if (count == 0)
    {
        if (state->explicit_selection)
        {
            dvz_fprintf(stderr, "no Vulkan physical devices are available for --gpu\n");
            dvz_instance_destroy(instance);
            return 1;
        }
        dvz_instance_destroy(instance);
        return 0;
    }
    DvzTestingGpuSelection selection = {
        .requested_index = state->requested_index,
        .source = state->source,
        .explicit_selection = state->explicit_selection,
    };
    if (!dvz_testing_gpu_selection_resolve(instance, &selection, &state->info, &count))
    {
        dvz_fprintf(
            stderr, "GPU index %u is unavailable (available count=%u)\n",
            state->requested_index, count);
        _dvz_testing_gpu_print_available(instance, count);
        dvz_instance_destroy(instance);
        return 1;
    }
    state->resolved = true;
    dvz_instance_destroy(instance);
    return 0;
}



/**
 * Return the configured selection-source name.
 *
 * @param source selection source
 * @return static source name
 */
/**
 * Append one JSON-escaped string into a bounded output buffer.
 *
 * @param input source string
 * @param output output buffer
 * @param size output buffer size
 * @return true when the complete escaped string fit
 */
static bool _dvz_testing_json_escape(const char* input, char* output, size_t size)
{
    ANN(input);
    ANN(output);
    if (size == 0)
    {
        return false;
    }

    size_t j = 0;
    for (const unsigned char* p = (const unsigned char*)input; *p != '\0'; p++)
    {
        char escaped[7] = {0};
        const char* text = NULL;
        if (*p == '"')
            text = "\\\"";
        else if (*p == '\\')
            text = "\\\\";
        else if (*p == '\n')
            text = "\\n";
        else if (*p == '\r')
            text = "\\r";
        else if (*p == '\t')
            text = "\\t";
        else if (*p < 0x20)
        {
            dvz_snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned int)*p);
            text = escaped;
        }

        if (text != NULL)
        {
            const size_t length = strlen(text);
            if (length >= size - j)
                return false;
            dvz_memcpy(output + j, size - j, text, length);
            j += length;
        }
        else
        {
            if (j + 1 >= size)
                return false;
            output[j++] = (char)*p;
        }
    }
    output[j] = '\0';
    return true;
}



/**
 * Return typed GPU adapter state from an opaque run state pointer.
 *
 * @param state opaque run state
 * @return typed state, or NULL
 */
static const DvzTestingGpuState* _dvz_testing_gpu_state(const void* state)
{
    return (const DvzTestingGpuState*)state;
}



/*************************************************************************************************/
/*  Adapter callbacks                                                                            */
/*************************************************************************************************/

/**
 * Parse one Datoviz-specific runner option.
 *
 * @param state adapter state
 * @param argc argument count
 * @param argv argument values
 * @param index current argument index
 * @return positive when handled, zero when unhandled, negative on error
 */
static int _dvz_testing_gpu_parse_option(void* state, int argc, char** argv, int* index)
{
    DvzTestingGpuState* gpu = (DvzTestingGpuState*)state;
    ANN(gpu);
    ANN(argv);
    ANN(index);
    const char* arg = argv[*index];

    if (arg != NULL && strcmp(arg, "--gpu") == 0)
    {
        if (gpu->cli_seen)
        {
            dvz_fprintf(stderr, "--gpu may be specified only once\n");
            return -1;
        }
        if (*index + 1 >= argc || argv[*index + 1] == NULL || argv[*index + 1][0] == '-')
        {
            dvz_fprintf(stderr, "--gpu requires an ASCII decimal index\n");
            return -1;
        }
        DvzTestingGpuSelection selection = {0};
        dvz_testing_gpu_selection_init(&selection);
        if (!dvz_testing_gpu_selection_set_cli(&selection, argv[*index + 1]))
        {
            dvz_fprintf(stderr, "invalid --gpu index: %s\n", argv[*index + 1]);
            return -1;
        }
        (*index)++;
        gpu->requested_index = selection.requested_index;
        gpu->source = selection.source;
        gpu->cli_seen = true;
        gpu->explicit_selection = true;
        return 1;
    }
    if (arg != NULL && strncmp(arg, "--gpu=", 6) == 0)
    {
        dvz_fprintf(stderr, "use --gpu <index>; --gpu=<index> is not supported\n");
        return -1;
    }
    if (arg != NULL && strcmp(arg, "--list-gpus") == 0)
    {
        if (gpu->list_gpus)
        {
            dvz_fprintf(stderr, "--list-gpus may be specified only once\n");
            return -1;
        }
        gpu->list_gpus = true;
        return 1;
    }
    return 0;
}



/**
 * Resolve environment precedence and validate discovery/listing combinations.
 *
 * @param state adapter state
 * @param argc argument count
 * @param argv argument values
 * @param list whether case-list mode was requested
 * @param list_groups whether group-list mode was requested
 * @param child_process whether this is a child report process
 * @return zero on success
 */
static int _dvz_testing_gpu_configure(
    void* state, int argc, char** argv, bool list, bool list_groups, bool child_process)
{
    DvzTestingGpuState* gpu = (DvzTestingGpuState*)state;
    ANN(gpu);
    gpu->list_mode = list || list_groups;
    gpu->child_process = child_process;

    if (gpu->list_gpus)
    {
        if (argc != 2 || argv[1] == NULL || strcmp(argv[1], "--list-gpus") != 0)
        {
            dvz_fprintf(stderr, "--list-gpus is an exclusive discovery action\n");
            return 1;
        }
        return 0;
    }
    if (gpu->list_mode)
    {
        if (gpu->cli_seen)
        {
            dvz_fprintf(stderr, "--gpu cannot be combined with --list or --list-groups\n");
            return 1;
        }
        return 0;
    }
    if (!gpu->cli_seen)
    {
        DvzTestingGpuSelection selection = {0};
        dvz_testing_gpu_selection_init(&selection);
        if (!dvz_testing_gpu_selection_set_environment(&selection))
        {
            dvz_fprintf(stderr, "invalid DVZ_TEST_GPU index\n");
            return 1;
        }
        if (selection.explicit_selection)
        {
            gpu->requested_index = selection.requested_index;
            gpu->source = selection.source;
            gpu->explicit_selection = true;
        }
    }
    return 0;
}



/**
 * Run exclusive GPU discovery before selecting test cases.
 *
 * @param state adapter state
 * @return positive after successful discovery, zero to continue, negative on failure
 */
static int _dvz_testing_gpu_early_action(void* state)
{
    DvzTestingGpuState* gpu = (DvzTestingGpuState*)state;
    ANN(gpu);
    if (!gpu->list_gpus)
    {
        return 0;
    }
    return _dvz_testing_gpu_resolve(gpu, true) == 0 ? 1 : -1;
}



/**
 * Apply the fail-closed explicit-selection policy to one selected test case.
 *
 * @param state adapter state
 * @param test candidate test case
 * @return positive to include, zero to exclude, negative on policy error
 */
static int _dvz_testing_gpu_filter_case(const void* state, const TstCase* test)
{
    const DvzTestingGpuState* gpu = _dvz_testing_gpu_state(state);
    ANN(gpu);
    ANN(test);
    if (!gpu->explicit_selection || (test->resources & (TST_RES_GPU | TST_RES_VULKAN)) == 0)
    {
        return 1;
    }
    if ((test->run_flags & TST_RUN_CASE_ADAPTER_SUPPORTED) != 0)
    {
        return 1;
    }
    if ((test->run_flags & TST_RUN_CASE_ADAPTER_EXEMPT) != 0)
    {
        return 0;
    }
    dvz_fprintf(
        stderr, "GPU-selected run includes unclassified case %s/%s/%s\n",
        test->module != NULL ? test->module : "default",
        test->group != NULL ? test->group : "default", test->name != NULL ? test->name : "case");
    return -1;
}



/**
 * Resolve GPU metadata after canonical case filtering.
 *
 * @param state adapter state
 * @param case_count selected case count
 * @param cases selected cases
 * @param child_process whether this is a child report process
 * @return zero on success
 */
static int _dvz_testing_gpu_prepare(
    void* state, uint32_t case_count, const TstCase* const* cases, bool child_process)
{
    DvzTestingGpuState* gpu = (DvzTestingGpuState*)state;
    ANN(gpu);
    gpu->child_process = child_process;

    bool needs_gpu = gpu->explicit_selection;
    for (uint32_t i = 0; i < case_count && !needs_gpu; i++)
    {
        ANN(cases);
        if ((cases[i]->resources & (TST_RES_GPU | TST_RES_VULKAN)) != 0)
        {
            needs_gpu = true;
        }
    }
    if (!needs_gpu)
    {
        return 0;
    }
    return _dvz_testing_gpu_resolve(gpu, false);
}



/**
 * Write canonical GPU run metadata.
 *
 * @param state adapter state
 * @param json output JSON buffer
 * @param size output buffer size
 * @return zero on success
 */
static int _dvz_testing_gpu_write_json(const void* state, char* json, size_t size)
{
    const DvzTestingGpuState* gpu = _dvz_testing_gpu_state(state);
    ANN(gpu);
    ANN(json);
    if (!gpu->resolved)
    {
        int written = dvz_snprintf(json, size, "{\"gpu\":null}");
        return written < 0 || (size_t)written >= size ? 1 : 0;
    }

    char name[2048] = {0};
    if (!_dvz_testing_json_escape(gpu->info.name, name, sizeof(name)))
    {
        return 1;
    }
    int written = dvz_snprintf(
        json, size,
        "{\"gpu\":{\"requested_index\":%u,\"selection_source\":\"%s\","
        "\"resolved_index\":%u,\"name\":\"%s\",\"device_type\":\"%s\","
        "\"device_type_raw\":%d,\"vendor_id\":%u,\"device_id\":%u,"
        "\"api_version_raw\":%u,\"driver_version_raw\":%u}}",
        gpu->requested_index, dvz_testing_gpu_source_name(gpu->source), gpu->info.index, name,
        _dvz_testing_gpu_type_name(gpu->info.device_type), (int)gpu->info.device_type,
        gpu->info.vendor_id, gpu->info.device_id, gpu->info.api_version,
        gpu->info.driver_version);
    return written < 0 || (size_t)written >= size ? 1 : 0;
}



/**
 * Print the resolved GPU once in the root process.
 *
 * @param state adapter state
 */
static void _dvz_testing_gpu_report(const void* state)
{
    const DvzTestingGpuState* gpu = _dvz_testing_gpu_state(state);
    ANN(gpu);
    if (!gpu->resolved)
    {
        return;
    }
    dvz_fprintf(
        stdout, "GPU %u: %s (%s, vendor=0x%04x, device=0x%04x)\n", gpu->info.index,
        gpu->info.name, _dvz_testing_gpu_type_name(gpu->info.device_type), gpu->info.vendor_id,
        gpu->info.device_id);
}



/**
 * Print Datoviz-specific test-runner usage.
 *
 * @param state adapter state
 */
static void _dvz_testing_gpu_print_usage(const void* state)
{
    (void)state;
    dvz_fprintf(
        stdout,
        "               [--gpu index] [--list-gpus]\n"
        "Environment: DVZ_TEST_GPU selects an index when --gpu is absent.\n");
}



/**
 * Destroy heap-allocated GPU adapter state.
 *
 * @param state adapter state
 */
static void _dvz_testing_gpu_destroy(void* state)
{
    dvz_free(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Install the Datoviz GPU-selection adapter on a test suite.
 *
 * @param suite test suite
 */
void dvz_testing_install_gpu_adapter(TstSuite* suite)
{
    ANN(suite);
    DvzTestingGpuState* state = (DvzTestingGpuState*)dvz_calloc(1, sizeof(DvzTestingGpuState));
    ANN(state);
    state->source = DVZ_TESTING_GPU_SOURCE_DEFAULT;

    TstRunAdapter adapter = {0};
    adapter.parse_option = _dvz_testing_gpu_parse_option;
    adapter.configure = _dvz_testing_gpu_configure;
    adapter.early_action = _dvz_testing_gpu_early_action;
    adapter.filter_case = _dvz_testing_gpu_filter_case;
    adapter.prepare = _dvz_testing_gpu_prepare;
    adapter.write_json = _dvz_testing_gpu_write_json;
    adapter.report = _dvz_testing_gpu_report;
    adapter.print_usage = _dvz_testing_gpu_print_usage;
    adapter.destroy = _dvz_testing_gpu_destroy;
    adapter.state = state;
    tst_suite_set_run_adapter(suite, &adapter);
}



/**
 * Return the requested GPU index from a test context.
 *
 * @param ctx test context
 * @return requested GPU index, defaulting to zero without an adapter
 */
uint32_t dvz_testing_gpu_index(const TstContext* ctx)
{
    ANN(ctx);
    const DvzTestingGpuState* state = _dvz_testing_gpu_state(tst_context_run_state(ctx));
    return state != NULL ? state->requested_index : 0;
}



/**
 * Return a GPU-context configuration using the context selection.
 *
 * @param ctx test context
 * @return configured GPU-context options
 */
DvzGpuCtxConfig dvz_testing_gpu_ctx_config(const TstContext* ctx)
{
    DvzGpuCtxConfig config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_gpu(&config, dvz_testing_gpu_index(ctx));
    return config;
}



/**
 * Return the requested GPU index from a test suite.
 *
 * @param suite test suite
 * @return requested GPU index, defaulting to zero without an adapter
 */
uint32_t dvz_testing_suite_gpu_index(const TstSuite* suite)
{
    ANN(suite);
    const DvzTestingGpuState* state = _dvz_testing_gpu_state(tst_suite_run_state(suite));
    return state != NULL ? state->requested_index : 0;
}



/**
 * Return a GPU-context configuration using the suite selection.
 *
 * @param suite test suite
 * @return configured GPU-context options
 */
DvzGpuCtxConfig dvz_testing_suite_gpu_ctx_config(const TstSuite* suite)
{
    DvzGpuCtxConfig config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_gpu(&config, dvz_testing_suite_gpu_index(suite));
    return config;
}



/**
 * Copy resolved GPU metadata from a test context.
 *
 * @param ctx test context
 * @param[out] out_info resolved GPU metadata
 * @return true when GPU metadata was resolved
 */
bool dvz_testing_gpu_info(const TstContext* ctx, DvzGpuInfo* out_info)
{
    ANN(ctx);
    ANN(out_info);
    const DvzTestingGpuState* state = _dvz_testing_gpu_state(tst_context_run_state(ctx));
    if (state == NULL || !state->resolved)
    {
        return false;
    }
    *out_info = state->info;
    return true;
}



/**
 * Copy resolved GPU metadata from a test suite.
 *
 * @param suite test suite
 * @param[out] out_info resolved GPU metadata
 * @return true when GPU metadata was resolved
 */
bool dvz_testing_suite_gpu_info(const TstSuite* suite, DvzGpuInfo* out_info)
{
    ANN(suite);
    ANN(out_info);
    const DvzTestingGpuState* state = _dvz_testing_gpu_state(tst_suite_run_state(suite));
    if (state == NULL || !state->resolved)
    {
        return false;
    }
    *out_info = state->info;
    return true;
}
