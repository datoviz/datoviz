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

#include <stddef.h>
#include <stdint.h>

#include "datoviz/shader.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Internal functions                                                                          */
/*************************************************************************************************/

bool _dvz_shader_compile_glsl(
    const char* stage, const char* code, size_t code_size, const char* source_name,
    const char* entry_point, uint32_t** spv, uint64_t* spv_size);



EXTERN_C_OFF
