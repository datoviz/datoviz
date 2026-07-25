/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Runtime shader compiler service                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DVZ_HAS_SHADERC
#include "shaderc/shaderc.h"
#else
typedef int shaderc_shader_kind;
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_dynload.h"
#include "_log.h"
#include "_shader.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

#if DVZ_HAS_SHADERC
typedef struct
{
    shaderc_compiler_t (*compiler_initialize)(void);
    void (*compiler_release)(shaderc_compiler_t);
    shaderc_compile_options_t (*compile_options_initialize)(void);
    void (*compile_options_release)(shaderc_compile_options_t);
    void (*compile_options_set_source_language)(
        shaderc_compile_options_t, shaderc_source_language);
    void (*compile_options_set_target_env)(
        shaderc_compile_options_t, shaderc_target_env, uint32_t);
    void (*compile_options_set_target_spirv)(
        shaderc_compile_options_t, shaderc_spirv_version);
    shaderc_compilation_result_t (*compile_into_spv)(
        shaderc_compiler_t, const char*, size_t, shaderc_shader_kind, const char*, const char*,
        const shaderc_compile_options_t);
    shaderc_compilation_status (*result_get_compilation_status)(shaderc_compilation_result_t);
    const char* (*result_get_error_message)(shaderc_compilation_result_t);
    const char* (*result_get_bytes)(shaderc_compilation_result_t);
    size_t (*result_get_length)(shaderc_compilation_result_t);
    void (*result_release)(shaderc_compilation_result_t);
} ShadercSyms;
#endif



/*************************************************************************************************/
/*  Process-global provider state                                                                */
/*************************************************************************************************/

static pthread_once_t _shaderc_once = PTHREAD_ONCE_INIT;
static DvzShaderCompileStatus _shaderc_state = DVZ_SHADER_COMPILE_INTERNAL_ERROR;

#if DVZ_HAS_SHADERC
#if DVZ_SHADERC_STATIC
static ShadercSyms _shaderc = {
    .compiler_initialize                 = shaderc_compiler_initialize,
    .compiler_release                    = shaderc_compiler_release,
    .compile_options_initialize           = shaderc_compile_options_initialize,
    .compile_options_release              = shaderc_compile_options_release,
    .compile_options_set_source_language  = shaderc_compile_options_set_source_language,
    .compile_options_set_target_env       = shaderc_compile_options_set_target_env,
    .compile_options_set_target_spirv     = shaderc_compile_options_set_target_spirv,
    .compile_into_spv                     = shaderc_compile_into_spv,
    .result_get_compilation_status        = shaderc_result_get_compilation_status,
    .result_get_error_message             = shaderc_result_get_error_message,
    .result_get_bytes                     = shaderc_result_get_bytes,
    .result_get_length                    = shaderc_result_get_length,
    .result_release                       = shaderc_result_release,
};
#else
static ShadercSyms _shaderc = {0};
#endif
#endif



/*************************************************************************************************/
/*  Provider discovery                                                                           */
/*************************************************************************************************/

#if DVZ_HAS_SHADERC && !DVZ_SHADERC_STATIC
/**
 * Try the wheel runtime directories for a shaderc provider.
 *
 * @param filename configured shaderc runtime filename
 * @return dynamic-library handle, or NULL
 */
static DvzDynLib _shaderc_open_runtime_dirs(const char* filename)
{
    ANN(filename);
    const char* dirs = getenv("DVZ_WHEEL_RUNTIME_DIRS");
    if (dirs == NULL || dirs[0] == '\0')
        return NULL;

    const char* basename = strrchr(filename, '/');
#if defined(_WIN32)
    const char* backslash = strrchr(filename, '\\');
    if (basename == NULL || (backslash != NULL && backslash > basename))
        basename = backslash;
#endif
    filename = basename != NULL ? basename + 1 : filename;
    if (filename[0] == '\0')
        return NULL;

#if defined(_WIN32)
    const char sep = ';';
#else
    const char sep = ':';
#endif

    const char* cursor = dirs;
    while (*cursor != '\0')
    {
        const char* end = strchr(cursor, sep);
        size_t dir_len = end == NULL ? strlen(cursor) : (size_t)(end - cursor);
        if (dir_len > 0)
        {
            char path[4096] = {0};
            int rc = snprintf(path, sizeof(path), "%.*s/%s", (int)dir_len, cursor, filename);
            if (rc > 0 && (size_t)rc < sizeof(path))
            {
                DvzDynLib lib = dvz_dynlib_open(path);
                if (lib != NULL)
                    return lib;
            }
        }
        if (end == NULL)
            break;
        cursor = end + 1;
    }
    return NULL;
}
#endif



/**
 * Initialize the shaderc adapter exactly once.
 */
static void _shaderc_initialize_once(void)
{
#if DVZ_HAS_SHADERC
#if DVZ_SHADERC_STATIC
    _shaderc_state = DVZ_SHADER_COMPILE_SUCCESS;
#else
#ifndef DVZ_SHADERC_LIB_PATH
#define DVZ_SHADERC_LIB_PATH "libshaderc_shared.so.1"
#endif
    const char* env_path = getenv("DVZ_SHADERC_RUNTIME_LIBRARY");
    DvzDynLib lib = NULL;
    if (env_path != NULL && env_path[0] != '\0')
    {
        lib = dvz_dynlib_open(env_path);
        if (lib == NULL)
        {
            _shaderc_state = DVZ_SHADER_COMPILE_PROVIDER_MISSING;
            log_error(
                "runtime GLSL compilation unavailable: DVZ_SHADERC_RUNTIME_LIBRARY could not "
                "be loaded: %s",
                env_path);
            return;
        }
    }
    if (lib == NULL)
        lib = dvz_dynlib_open(DVZ_SHADERC_LIB_PATH);
    if (lib == NULL)
        lib = _shaderc_open_runtime_dirs(DVZ_SHADERC_LIB_PATH);
#if defined(__APPLE__)
    if (lib == NULL && strcmp(DVZ_SHADERC_LIB_PATH, "@rpath/libshaderc_shared.1.dylib") == 0)
        lib = dvz_dynlib_open("@loader_path/libshaderc_shared.1.dylib");
#endif
    if (lib == NULL)
    {
        _shaderc_state = DVZ_SHADER_COMPILE_PROVIDER_MISSING;
        log_error(
            "runtime GLSL compilation unavailable: could not load shaderc runtime "
            DVZ_SHADERC_LIB_PATH);
        log_error(
            "install shaderc or set DVZ_SHADERC_RUNTIME_LIBRARY/DVZ_WHEEL_RUNTIME_DIRS so the "
            "runtime library is packaged or discoverable");
        return;
    }

#define _SC_SYM(field, name)                                                                      \
    {                                                                                             \
        void* _p = dvz_dynlib_sym(lib, name);                                                     \
        if (_p == NULL)                                                                           \
        {                                                                                         \
            _shaderc_state = DVZ_SHADER_COMPILE_PROVIDER_INCOMPATIBLE;                           \
            log_error("shaderc provider is incompatible: symbol '%s' not found", name);          \
            dvz_dynlib_close(lib);                                                                \
            return;                                                                               \
        }                                                                                         \
        dvz_memcpy(&_shaderc.field, sizeof(_shaderc.field), &_p, sizeof(_p));                     \
    }

    _SC_SYM(compiler_initialize, "shaderc_compiler_initialize")
    _SC_SYM(compiler_release, "shaderc_compiler_release")
    _SC_SYM(compile_options_initialize, "shaderc_compile_options_initialize")
    _SC_SYM(compile_options_release, "shaderc_compile_options_release")
    _SC_SYM(compile_options_set_source_language, "shaderc_compile_options_set_source_language")
    _SC_SYM(compile_options_set_target_env, "shaderc_compile_options_set_target_env")
    _SC_SYM(compile_options_set_target_spirv, "shaderc_compile_options_set_target_spirv")
    _SC_SYM(compile_into_spv, "shaderc_compile_into_spv")
    _SC_SYM(result_get_compilation_status, "shaderc_result_get_compilation_status")
    _SC_SYM(result_get_error_message, "shaderc_result_get_error_message")
    _SC_SYM(result_get_bytes, "shaderc_result_get_bytes")
    _SC_SYM(result_get_length, "shaderc_result_get_length")
    _SC_SYM(result_release, "shaderc_result_release")
#undef _SC_SYM

    /* The provider remains resident so the resolved symbols stay valid for the process lifetime. */
    _shaderc_state = DVZ_SHADER_COMPILE_SUCCESS;
#endif
#else
    _shaderc_state = DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE;
#endif
}



/**
 * Ensure that provider discovery has completed.
 *
 * @return provider state
 */
DvzShaderCompileStatus dvz_shader_compiler_status(void)
{
    int status = pthread_once(&_shaderc_once, _shaderc_initialize_once);
    if (status != 0)
    {
        log_error("shaderc provider initialization failed with pthread status %d", status);
        return DVZ_SHADER_COMPILE_PROVIDER_INCOMPATIBLE;
    }
    return _shaderc_state;
}



/*************************************************************************************************/
/*  Compilation                                                                                  */
/*************************************************************************************************/

/**
 * Return the shaderc shader kind for a typed stage.
 *
 * @param stage shader stage
 * @return shaderc shader kind, or infer-from-source when the stage is unknown
 */
static shaderc_shader_kind _shaderc_kind(DvzShaderStage stage)
{
#if DVZ_HAS_SHADERC
    if (stage == DVZ_SHADER_STAGE_VERTEX)
        return shaderc_glsl_vertex_shader;
    if (stage == DVZ_SHADER_STAGE_FRAGMENT)
        return shaderc_glsl_fragment_shader;
    if (stage == DVZ_SHADER_STAGE_COMPUTE)
        return shaderc_glsl_compute_shader;
    return shaderc_glsl_infer_from_source;
#else
    (void)stage;
    return 0;
#endif
}



/**
 * Set owned formatted diagnostics on a compilation result.
 *
 * @param result compilation result
 * @param format printf-style diagnostic format
 */
static void _shader_result_diagnostics(DvzShaderCompileResult* result, const char* format, ...)
{
    ANN(result);
    ANN(format);
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    int length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0)
    {
        va_end(args);
        return;
    }

    result->diagnostics = (char*)dvz_malloc((size_t)length + 1);
    if (result->diagnostics != NULL)
    {
        vsnprintf(result->diagnostics, (size_t)length + 1, format, args);
        result->diagnostics_size = (uint64_t)length;
    }
    va_end(args);
}



/**
 * Return whether a stage and target profile form a supported v0.4 pair.
 *
 * @param stage shader stage
 * @param profile target profile
 * @return true when the pair is valid
 */
static bool _shader_profile_valid(DvzShaderStage stage, DvzShaderProfile profile)
{
    if (stage == DVZ_SHADER_STAGE_COMPUTE)
        return profile == DVZ_SHADER_PROFILE_COMPUTE;
    if (stage == DVZ_SHADER_STAGE_VERTEX || stage == DVZ_SHADER_STAGE_FRAGMENT)
        return profile == DVZ_SHADER_PROFILE_GRAPHICS;
    return false;
}



bool dvz_shader_compiler_available(void)
{
    return dvz_shader_compiler_status() == DVZ_SHADER_COMPILE_SUCCESS;
}



const char* dvz_shader_compile_status_name(DvzShaderCompileStatus status)
{
    switch (status)
    {
    case DVZ_SHADER_COMPILE_SUCCESS:
        return "success";
    case DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE:
        return "adapter-unavailable";
    case DVZ_SHADER_COMPILE_PROVIDER_MISSING:
        return "provider-missing";
    case DVZ_SHADER_COMPILE_PROVIDER_INCOMPATIBLE:
        return "provider-incompatible";
    case DVZ_SHADER_COMPILE_INVALID_REQUEST:
        return "invalid-request";
    case DVZ_SHADER_COMPILE_FAILED:
        return "compilation-failed";
    case DVZ_SHADER_COMPILE_OUT_OF_MEMORY:
        return "out-of-memory";
    case DVZ_SHADER_COMPILE_INTERNAL_ERROR:
        return "internal-error";
    default:
        return "unknown";
    }
}



DvzShaderCompileStatus
dvz_shader_compile(const DvzShaderCompileRequest* request, DvzShaderCompileResult* result)
{
    if (result == NULL)
        return DVZ_SHADER_COMPILE_INVALID_REQUEST;
    memset(result, 0, sizeof(DvzShaderCompileResult));
    result->status = DVZ_SHADER_COMPILE_INVALID_REQUEST;

    if (request == NULL)
    {
        _shader_result_diagnostics(result, "shader compilation request is NULL");
        return result->status;
    }
    if (request->source == NULL || request->source_size == 0)
    {
        _shader_result_diagnostics(result, "shader source is empty");
        return result->status;
    }
    if (request->source_size > SIZE_MAX)
    {
        _shader_result_diagnostics(result, "shader source is too large for this platform");
        return result->status;
    }
    if (request->source_name == NULL || request->source_name[0] == '\0')
    {
        _shader_result_diagnostics(result, "shader source_name is required");
        return result->status;
    }
    if (!_shader_profile_valid(request->stage, request->profile))
    {
        _shader_result_diagnostics(
            result, "shader stage %d and target profile %d are incompatible",
            (int)request->stage, (int)request->profile);
        return result->status;
    }

    DvzShaderCompileStatus provider_status = dvz_shader_compiler_status();
    if (provider_status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        result->status = provider_status;
        if (provider_status == DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE)
            _shader_result_diagnostics(
                result, "Datoviz was built without the runtime shader compiler adapter");
        else if (provider_status == DVZ_SHADER_COMPILE_PROVIDER_MISSING)
            _shader_result_diagnostics(
                result,
                "the shaderc runtime provider is missing; check "
                "DVZ_SHADERC_RUNTIME_LIBRARY and DVZ_WHEEL_RUNTIME_DIRS");
        else
            _shader_result_diagnostics(
                result, "the shaderc runtime provider is incompatible or failed initialization");
        return result->status;
    }

#if DVZ_HAS_SHADERC
    shaderc_shader_kind kind = _shaderc_kind(request->stage);
    shaderc_compiler_t compiler = _shaderc.compiler_initialize();
    if (compiler == NULL)
    {
        result->status = DVZ_SHADER_COMPILE_INTERNAL_ERROR;
        _shader_result_diagnostics(result, "shaderc compiler initialization failed");
        return result->status;
    }
    shaderc_compile_options_t options = _shaderc.compile_options_initialize();
    if (options == NULL)
    {
        _shaderc.compiler_release(compiler);
        result->status = DVZ_SHADER_COMPILE_INTERNAL_ERROR;
        _shader_result_diagnostics(result, "shaderc compile-options initialization failed");
        return result->status;
    }

    _shaderc.compile_options_set_source_language(options, shaderc_source_language_glsl);
    if (request->profile == DVZ_SHADER_PROFILE_COMPUTE)
    {
        _shaderc.compile_options_set_target_env(
            options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        _shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);
    }
    else
    {
        _shaderc.compile_options_set_target_env(
            options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        _shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_0);
    }

    const char* entry_point =
        request->entry_point != NULL && request->entry_point[0] != '\0'
            ? request->entry_point
            : "main";
    shaderc_compilation_result_t compiled = _shaderc.compile_into_spv(
        compiler, request->source, (size_t)request->source_size, kind, request->source_name,
        entry_point, options);

    _shaderc.compile_options_release(options);
    _shaderc.compiler_release(compiler);

    if (compiled == NULL)
    {
        result->status = DVZ_SHADER_COMPILE_INTERNAL_ERROR;
        _shader_result_diagnostics(
            result, "shaderc returned no result for '%s'", request->source_name);
        return result->status;
    }
    if (_shaderc.result_get_compilation_status(compiled) != shaderc_compilation_status_success)
    {
        result->status = DVZ_SHADER_COMPILE_FAILED;
        const char* message = _shaderc.result_get_error_message(compiled);
        _shader_result_diagnostics(
            result, "%s", message != NULL ? message : "shaderc compilation failed");
        _shaderc.result_release(compiled);
        return result->status;
    }

    const char* bytes = _shaderc.result_get_bytes(compiled);
    uint64_t size = (uint64_t)_shaderc.result_get_length(compiled);
    if (bytes == NULL || size == 0 || size % sizeof(uint32_t) != 0)
    {
        _shaderc.result_release(compiled);
        result->status = DVZ_SHADER_COMPILE_INTERNAL_ERROR;
        _shader_result_diagnostics(
            result, "shaderc returned invalid SPIR-V for '%s'", request->source_name);
        return result->status;
    }

    result->spirv =
        (uint32_t*)dvz_calloc((size_t)(size / sizeof(uint32_t)), sizeof(uint32_t));
    if (result->spirv == NULL)
    {
        _shaderc.result_release(compiled);
        result->status = DVZ_SHADER_COMPILE_OUT_OF_MEMORY;
        _shader_result_diagnostics(
            result, "unable to allocate %" PRIu64 " SPIR-V bytes", size);
        return result->status;
    }
    dvz_memcpy(result->spirv, (size_t)size, bytes, (size_t)size);
    _shaderc.result_release(compiled);

    result->spirv_size = size;
    result->status = DVZ_SHADER_COMPILE_SUCCESS;
    return result->status;
#else
    result->status = DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE;
    _shader_result_diagnostics(
        result, "Datoviz was built without the runtime shader compiler adapter");
    return result->status;
#endif
}



void dvz_shader_compile_result_destroy(DvzShaderCompileResult* result)
{
    if (result == NULL)
        return;
    dvz_free(result->spirv);
    dvz_free(result->diagnostics);
    memset(result, 0, sizeof(DvzShaderCompileResult));
}



/**
 * Convert a legacy stage string to the typed stage and profile.
 *
 * @param stage stage string
 * @param[out] typed_stage typed stage
 * @param[out] profile target profile
 * @return whether the stage is recognized
 */
static bool _shader_legacy_stage(
    const char* stage, DvzShaderStage* typed_stage, DvzShaderProfile* profile)
{
    ANN(stage);
    ANN(typed_stage);
    ANN(profile);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
    {
        *typed_stage = DVZ_SHADER_STAGE_VERTEX;
        *profile = DVZ_SHADER_PROFILE_GRAPHICS;
        return true;
    }
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
    {
        *typed_stage = DVZ_SHADER_STAGE_FRAGMENT;
        *profile = DVZ_SHADER_PROFILE_GRAPHICS;
        return true;
    }
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
    {
        *typed_stage = DVZ_SHADER_STAGE_COMPUTE;
        *profile = DVZ_SHADER_PROFILE_COMPUTE;
        return true;
    }
    return false;
}



bool _dvz_shader_compile_glsl(
    const char* stage, const char* code, size_t code_size, const char* source_name,
    const char* entry_point, uint32_t** spv, uint64_t* spv_size)
{
    ANN(stage);
    ANN(code);
    ANN(source_name);
    ANN(spv);
    ANN(spv_size);
    *spv = NULL;
    *spv_size = 0;

    DvzShaderStage typed_stage = DVZ_SHADER_STAGE_NONE;
    DvzShaderProfile profile = DVZ_SHADER_PROFILE_NONE;
    if (!_shader_legacy_stage(stage, &typed_stage, &profile))
    {
        log_error("GLSL compilation request for '%s' has invalid stage '%s'", source_name, stage);
        return false;
    }

    DvzShaderCompileRequest request = {
        .stage = typed_stage,
        .profile = profile,
        .source = code,
        .source_size = (uint64_t)code_size,
        .source_name = source_name,
        .entry_point = entry_point,
    };
    DvzShaderCompileResult result = {0};
    DvzShaderCompileStatus status = dvz_shader_compile(&request, &result);
    if (status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        log_error(
            "GLSL compilation failed for '%s' (%s): %s", source_name,
            dvz_shader_compile_status_name(status),
            result.diagnostics != NULL ? result.diagnostics : "no diagnostics");
        dvz_shader_compile_result_destroy(&result);
        return false;
    }

    *spv = result.spirv;
    *spv_size = result.spirv_size;
    result.spirv = NULL;
    dvz_shader_compile_result_destroy(&result);
    return true;
}



uint32_t* dvz_compile_glsl(const char* stage, const char* glsl, uint64_t* out_size)
{
    ANN(stage);
    ANN(glsl);
    ANN(out_size);
    *out_size = 0;
    uint32_t* spv = NULL;
    if (!_dvz_shader_compile_glsl(
            stage, glsl, strlen(glsl), "<memory>", "main", &spv, out_size))
        return NULL;
    return spv;
}
