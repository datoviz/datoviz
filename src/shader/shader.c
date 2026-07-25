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

#include <pthread.h>
#include <stdbool.h>
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
static DvzShaderCompilerState _shaderc_state = DVZ_SHADER_COMPILER_UNINITIALIZED;

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
    _shaderc_state = DVZ_SHADER_COMPILER_READY;
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
            _shaderc_state = DVZ_SHADER_COMPILER_PROVIDER_MISSING;
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
        _shaderc_state = DVZ_SHADER_COMPILER_PROVIDER_MISSING;
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
            _shaderc_state = DVZ_SHADER_COMPILER_PROVIDER_INCOMPATIBLE;                           \
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
    _shaderc_state = DVZ_SHADER_COMPILER_READY;
#endif
#else
    _shaderc_state = DVZ_SHADER_COMPILER_BUILT_WITHOUT_ADAPTER;
#endif
}



/**
 * Ensure that provider discovery has completed.
 *
 * @return provider state
 */
DvzShaderCompilerState _dvz_shader_compiler_state(void)
{
    int status = pthread_once(&_shaderc_once, _shaderc_initialize_once);
    if (status != 0)
    {
        log_error("shaderc provider initialization failed with pthread status %d", status);
        return DVZ_SHADER_COMPILER_PROVIDER_INCOMPATIBLE;
    }
    return _shaderc_state;
}



/*************************************************************************************************/
/*  Compilation                                                                                  */
/*************************************************************************************************/

/**
 * Return the shaderc shader kind for a stage string.
 *
 * @param stage shader stage string
 * @return shaderc shader kind, or infer-from-source when the stage is unknown
 */
static shaderc_shader_kind _shaderc_kind(const char* stage)
{
#if DVZ_HAS_SHADERC
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return shaderc_glsl_vertex_shader;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return shaderc_glsl_fragment_shader;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return shaderc_glsl_compute_shader;
    return shaderc_glsl_infer_from_source;
#else
    (void)stage;
    return 0;
#endif
}



/**
 * Compile GLSL source bytes into owned SPIR-V.
 *
 * @param stage shader stage string
 * @param code GLSL source bytes
 * @param code_size GLSL source byte size
 * @param source_name source filename used in diagnostics
 * @param entry_point shader entry point
 * @param spv output pointer to aligned SPIR-V words, freed by the caller
 * @param spv_size output SPIR-V byte size
 * @return true when compilation succeeds, false otherwise
 */
bool _dvz_shader_compile_glsl(
    const char* stage, const char* code, size_t code_size, const char* source_name,
    const char* entry_point, uint32_t** spv, uint64_t* spv_size)
{
    ANN(stage);
    ANN(code);
    ANN(source_name);
    ANN(entry_point);
    ANN(spv);
    ANN(spv_size);
    *spv = NULL;
    *spv_size = 0;

#if DVZ_HAS_SHADERC
    if (code_size == 0)
    {
        log_error("GLSL compilation request for '%s' has empty source", source_name);
        return false;
    }

    shaderc_shader_kind kind = _shaderc_kind(stage);
    if (kind == shaderc_glsl_infer_from_source)
    {
        log_error("GLSL compilation request for '%s' has invalid stage '%s'", source_name, stage);
        return false;
    }
    if (_dvz_shader_compiler_state() != DVZ_SHADER_COMPILER_READY)
        return false;

    shaderc_compiler_t compiler = _shaderc.compiler_initialize();
    if (compiler == NULL)
        return false;
    shaderc_compile_options_t options = _shaderc.compile_options_initialize();
    if (options == NULL)
    {
        _shaderc.compiler_release(compiler);
        return false;
    }

    _shaderc.compile_options_set_source_language(options, shaderc_source_language_glsl);
    if (kind == shaderc_glsl_compute_shader)
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

    shaderc_compilation_result_t result = _shaderc.compile_into_spv(
        compiler, code, code_size, kind, source_name, entry_point, options);

    _shaderc.compile_options_release(options);
    _shaderc.compiler_release(compiler);

    if (result == NULL)
        return false;
    if (_shaderc.result_get_compilation_status(result) != shaderc_compilation_status_success)
    {
        log_error(
            "GLSL compilation failed for '%s' (%s): %s", source_name, stage,
            _shaderc.result_get_error_message(result));
        _shaderc.result_release(result);
        return false;
    }

    const char* bytes = _shaderc.result_get_bytes(result);
    uint64_t size = (uint64_t)_shaderc.result_get_length(result);
    if (bytes == NULL || size == 0 || size % sizeof(uint32_t) != 0)
    {
        _shaderc.result_release(result);
        return false;
    }

    uint32_t* out = (uint32_t*)dvz_calloc((size_t)(size / sizeof(uint32_t)), sizeof(uint32_t));
    if (out == NULL)
    {
        _shaderc.result_release(result);
        return false;
    }
    dvz_memcpy(out, (size_t)size, bytes, (size_t)size);
    _shaderc.result_release(result);

    *spv = out;
    *spv_size = size;
    return true;
#else
    (void)code_size;
    log_error(
        "runtime GLSL compilation unavailable for '%s': Datoviz was built without shaderc "
        "support",
        source_name);
    return false;
#endif
}
