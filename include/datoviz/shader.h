/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Runtime shader compilation                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SHADER_STAGE_NONE = 0,
    DVZ_SHADER_STAGE_VERTEX,
    DVZ_SHADER_STAGE_FRAGMENT,
    DVZ_SHADER_STAGE_COMPUTE,
} DvzShaderStage;

typedef enum
{
    DVZ_SHADER_PROFILE_NONE = 0,
    DVZ_SHADER_PROFILE_GRAPHICS,
    DVZ_SHADER_PROFILE_COMPUTE,
} DvzShaderProfile;

typedef enum
{
    DVZ_SHADER_COMPILE_SUCCESS = 0,
    DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE,
    DVZ_SHADER_COMPILE_PROVIDER_MISSING,
    DVZ_SHADER_COMPILE_PROVIDER_INCOMPATIBLE,
    DVZ_SHADER_COMPILE_INVALID_REQUEST,
    DVZ_SHADER_COMPILE_FAILED,
    DVZ_SHADER_COMPILE_OUT_OF_MEMORY,
    DVZ_SHADER_COMPILE_INTERNAL_ERROR,
} DvzShaderCompileStatus;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzShaderStage stage;
    DvzShaderProfile profile;
    const char* source;
    uint64_t source_size;
    const char* source_name;
    const char* entry_point;
} DvzShaderCompileRequest;

typedef struct
{
    DvzShaderCompileStatus status;
    uint32_t* spirv;
    uint64_t spirv_size;
    char* diagnostics;
    uint64_t diagnostics_size;
} DvzShaderCompileResult;



EXTERN_C_ON

/*************************************************************************************************/
/*  Compiler availability                                                                        */
/*************************************************************************************************/

/**
 * Return the runtime shader compiler status without requiring a GPU context.
 *
 * This function performs thread-safe, once-only provider discovery. A successful status means
 * runtime GLSL compilation is ready.
 *
 * @return current compiler status
 */
DVZ_EXPORT DvzShaderCompileStatus dvz_shader_compiler_status(void);



/**
 * Return whether runtime GLSL compilation is ready.
 *
 * @return true when the shader compiler adapter and provider are available
 */
DVZ_EXPORT bool dvz_shader_compiler_available(void);



/**
 * Return a stable name for a shader compilation status.
 *
 * @param status shader compilation status
 * @return static status name
 */
DVZ_EXPORT const char* dvz_shader_compile_status_name(DvzShaderCompileStatus status);



/*************************************************************************************************/
/*  Compilation                                                                                  */
/*************************************************************************************************/

/**
 * Compile GLSL source bytes to owned SPIR-V.
 *
 * Vertex and fragment shaders require `DVZ_SHADER_PROFILE_GRAPHICS`, which targets Vulkan 1.0 and
 * SPIR-V 1.0. Compute shaders require `DVZ_SHADER_PROFILE_COMPUTE`, which targets Vulkan 1.3 and
 * SPIR-V 1.6. `source_name` must identify the real source for diagnostics. A NULL or empty
 * `entry_point` selects `main`.
 *
 * On every call with a non-NULL result, the result is initialized and receives the returned
 * status. On success it owns aligned SPIR-V words. On failure it normally owns a null-terminated
 * diagnostic string. Call dvz_shader_compile_result_destroy() exactly once before reusing or
 * discarding the result.
 *
 * @param request compilation request
 * @param[out] result owned compilation result
 * @return compilation status
 */
DVZ_EXPORT DvzShaderCompileStatus
dvz_shader_compile(const DvzShaderCompileRequest* request, DvzShaderCompileResult* result);



/**
 * Destroy owned data in a shader compilation result and reset it.
 *
 * This function is idempotent. The SPIR-V and diagnostic buffers use the Datoviz allocator and
 * follow the same ownership contract as dvz_memory_free().
 *
 * @param result compilation result
 */
DVZ_EXPORT void dvz_shader_compile_result_destroy(DvzShaderCompileResult* result);



/**
 * Compile a null-terminated GLSL string through the typed API.
 *
 * This transitional convenience accepts only `vertex`, `fragment`, or `compute`, uses the matching
 * v0.4 target profile, reports errors through the log, and returns owned SPIR-V that must be freed
 * with dvz_memory_free(). New code should use dvz_shader_compile().
 *
 * @param stage shader stage string
 * @param glsl null-terminated GLSL source string
 * @param[out] out_size returned SPIR-V byte size
 * @return owned SPIR-V words, or NULL
 */
DVZ_EXPORT uint32_t* dvz_compile_glsl(const char* stage, const char* glsl, uint64_t* out_size);



EXTERN_C_OFF
