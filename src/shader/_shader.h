/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal shader compiler service                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_SHADER_COMPILER_UNINITIALIZED = 0,
    DVZ_SHADER_COMPILER_READY,
    DVZ_SHADER_COMPILER_BUILT_WITHOUT_ADAPTER,
    DVZ_SHADER_COMPILER_PROVIDER_MISSING,
    DVZ_SHADER_COMPILER_PROVIDER_INCOMPATIBLE,
} DvzShaderCompilerState;



EXTERN_C_ON

/*************************************************************************************************/
/*  Internal functions                                                                          */
/*************************************************************************************************/

DvzShaderCompilerState _dvz_shader_compiler_state(void);

bool _dvz_shader_compile_glsl(
    const char* stage, const char* code, size_t code_size, const char* source_name,
    const char* entry_point, uint32_t** spv, uint64_t* spv_size);



EXTERN_C_OFF
