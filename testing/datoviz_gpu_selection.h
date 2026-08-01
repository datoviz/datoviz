/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#ifndef DVZ_DATOVIZ_GPU_SELECTION_HEADER
#define DVZ_DATOVIZ_GPU_SELECTION_HEADER


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TESTING_GPU_SOURCE_DEFAULT = 0,
    DVZ_TESTING_GPU_SOURCE_ENV = 1,
    DVZ_TESTING_GPU_SOURCE_CLI = 2,
} DvzTestingGpuSource;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzTestingGpuSelection
{
    uint32_t requested_index;
    DvzTestingGpuSource source;
    bool explicit_selection;
} DvzTestingGpuSelection;


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

bool dvz_testing_gpu_parse_index(const char* value, uint32_t* out_index);

void dvz_testing_gpu_selection_init(DvzTestingGpuSelection* selection);

bool dvz_testing_gpu_selection_set_cli(DvzTestingGpuSelection* selection, const char* value);

bool dvz_testing_gpu_selection_set_environment(DvzTestingGpuSelection* selection);

bool dvz_testing_gpu_selection_resolve(
    DvzInstance* instance, const DvzTestingGpuSelection* selection, DvzGpuInfo* out_info,
    uint32_t* out_count);

const char* dvz_testing_gpu_source_name(DvzTestingGpuSource source);

EXTERN_C_OFF

#endif
