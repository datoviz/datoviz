/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz test GPU selection                                                                   */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdlib.h>

#include "_assertions.h"
#include "datoviz_gpu_selection.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEST_GPU_ENV "DVZ_TEST_GPU"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse a strict ASCII-decimal GPU index.
 *
 * @param value selector text
 * @param out_index destination parsed index
 * @return true when the complete value is a uint32 decimal
 */
bool dvz_testing_gpu_parse_index(const char* value, uint32_t* out_index)
{
    ANN(out_index);
    if (value == NULL || value[0] == '\0')
        return false;

    uint32_t parsed = 0;
    for (const unsigned char* p = (const unsigned char*)value; *p != '\0'; p++)
    {
        if (*p < (unsigned char)'0' || *p > (unsigned char)'9')
            return false;
        const uint32_t digit = (uint32_t)(*p - (unsigned char)'0');
        if (parsed > (UINT32_MAX - digit) / 10u)
            return false;
        parsed = parsed * 10u + digit;
    }
    *out_index = parsed;
    return true;
}



/**
 * Initialize a GPU selection to the production-default device.
 *
 * @param selection selection to initialize
 */
void dvz_testing_gpu_selection_init(DvzTestingGpuSelection* selection)
{
    ANN(selection);
    *selection = (DvzTestingGpuSelection){.source = DVZ_TESTING_GPU_SOURCE_DEFAULT};
}



/**
 * Apply a command-line GPU selector.
 *
 * @param selection selection to update
 * @param value strict decimal selector text
 * @return true when the selector is valid
 */
bool dvz_testing_gpu_selection_set_cli(DvzTestingGpuSelection* selection, const char* value)
{
    ANN(selection);
    uint32_t index = 0;
    if (!dvz_testing_gpu_parse_index(value, &index))
        return false;
    selection->requested_index = index;
    selection->source = DVZ_TESTING_GPU_SOURCE_CLI;
    selection->explicit_selection = true;
    return true;
}



/**
 * Apply DVZ_TEST_GPU unless a command-line selector already won precedence.
 *
 * @param selection selection to update
 * @return true when the environment is absent or valid
 */
bool dvz_testing_gpu_selection_set_environment(DvzTestingGpuSelection* selection)
{
    ANN(selection);
    if (selection->explicit_selection)
        return true;
    const char* value = getenv(DVZ_TEST_GPU_ENV);
    if (value == NULL)
        return true;

    uint32_t index = 0;
    if (!dvz_testing_gpu_parse_index(value, &index))
        return false;
    selection->requested_index = index;
    selection->source = DVZ_TESTING_GPU_SOURCE_ENV;
    selection->explicit_selection = true;
    return true;
}



/**
 * Resolve a selected device in an existing Datoviz instance.
 *
 * @param instance instance containing physical-device enumeration
 * @param selection requested selection
 * @param out_info destination resolved device information
 * @param out_count optional enumerated device count
 * @return true when the requested device exists and its information was copied
 */
bool dvz_testing_gpu_selection_resolve(
    DvzInstance* instance, const DvzTestingGpuSelection* selection, DvzGpuInfo* out_info,
    uint32_t* out_count)
{
    ANN(instance);
    ANN(selection);
    ANN(out_info);
    const uint32_t count = dvz_instance_gpu_count(instance);
    if (out_count != NULL)
        *out_count = count;
    if (selection->requested_index >= count)
        return false;
    return dvz_instance_gpu_info(instance, selection->requested_index, out_info);
}



/**
 * Return the human-readable source for a GPU selector.
 *
 * @param source selection source
 * @return static source name
 */
const char* dvz_testing_gpu_source_name(DvzTestingGpuSource source)
{
    switch (source)
    {
    case DVZ_TESTING_GPU_SOURCE_CLI:
        return "cli";
    case DVZ_TESTING_GPU_SOURCE_ENV:
        return "env";
    default:
        return "default";
    }
}
