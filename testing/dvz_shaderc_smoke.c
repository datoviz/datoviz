/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Source-build shaderc smoke                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "datoviz/shader.h"



/*************************************************************************************************/
/*  Concurrent first use                                                                         */
/*************************************************************************************************/

typedef struct
{
    bool succeeded;
} ShaderCompileThread;



/**
 * Compile one shader on a worker thread.
 *
 * @param user_data worker result
 * @return NULL
 */
static void* _compile_thread(void* user_data)
{
    ShaderCompileThread* thread = (ShaderCompileThread*)user_data;
    static const char glsl[] = "#version 450\n"
                               "void main() { gl_Position = vec4(0.0); }\n";
    DvzShaderCompileRequest request = {
        .stage = DVZ_SHADER_STAGE_VERTEX,
        .profile = DVZ_SHADER_PROFILE_GRAPHICS,
        .source = glsl,
        .source_size = sizeof(glsl) - 1,
        .source_name = "concurrent_first_use.vert",
    };
    DvzShaderCompileResult result = {0};
    DvzShaderCompileStatus status = dvz_shader_compile(&request, &result);
    thread->succeeded =
        status == DVZ_SHADER_COMPILE_SUCCESS &&
        result.spirv_size >= 5 * sizeof(uint32_t) && result.spirv[0] == 0x07230203;
    dvz_shader_compile_result_destroy(&result);
    return NULL;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzShaderCompileStatus compiler_status = dvz_shader_compiler_status();
    if (argc == 2 && strcmp(argv[1], "--expect-provider-missing") == 0)
    {
        if (compiler_status != DVZ_SHADER_COMPILE_PROVIDER_MISSING ||
            dvz_shader_compiler_available())
        {
            fprintf(
                stderr, "expected provider-missing, got %s\n",
                dvz_shader_compile_status_name(compiler_status));
            return 1;
        }
        printf("source-build shaderc missing-provider smoke passed\n");
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--expect-provider-incompatible") == 0)
    {
        if (compiler_status != DVZ_SHADER_COMPILE_PROVIDER_INCOMPATIBLE ||
            dvz_shader_compiler_available())
        {
            fprintf(
                stderr, "expected provider-incompatible, got %s\n",
                dvz_shader_compile_status_name(compiler_status));
            return 1;
        }
        printf("source-build shaderc incompatible-provider smoke passed\n");
        return 0;
    }

#if !DVZ_HAS_SHADERC
    if (dvz_shader_compiler_available() ||
        compiler_status != DVZ_SHADER_COMPILE_ADAPTER_UNAVAILABLE)
    {
        fprintf(stderr, "disabled shader compiler adapter returned an invalid status\n");
        return 1;
    }
    printf("source-build shaderc disabled-adapter smoke passed\n");
    return 0;
#endif

    if (!dvz_shader_compiler_available() ||
        compiler_status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        fprintf(stderr, "runtime shader compiler is unavailable\n");
        return 1;
    }

    enum
    {
        THREAD_COUNT = 8,
    };
    pthread_t threads[THREAD_COUNT] = {0};
    ShaderCompileThread results[THREAD_COUNT] = {0};
    for (uint32_t i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&threads[i], NULL, _compile_thread, &results[i]) != 0)
        {
            fprintf(stderr, "unable to create shader compilation thread %u\n", i);
            return 1;
        }
    }
    for (uint32_t i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_join(threads[i], NULL) != 0 || !results[i].succeeded)
        {
            fprintf(stderr, "concurrent shader compilation failed on thread %u\n", i);
            return 1;
        }
    }

    static const char glsl[] = "#version 450\n"
                               "layout(local_size_x = 1) in;\n"
                               "void main() {}\n";

    DvzShaderCompileRequest request = {
        .stage = DVZ_SHADER_STAGE_COMPUTE,
        .profile = DVZ_SHADER_PROFILE_COMPUTE,
        .source = glsl,
        .source_size = sizeof(glsl) - 1,
        .source_name = "shaderc_smoke.comp",
        .entry_point = "main",
    };
    DvzShaderCompileResult result = {0};
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_SUCCESS)
    {
        fprintf(
            stderr, "dvz_shader_compile failed: %s\n",
            result.diagnostics != NULL ? result.diagnostics : "no diagnostics");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    uint64_t size = result.spirv_size;
    uint32_t* spv = result.spirv;
    if (size < 5 * sizeof(uint32_t) || size % sizeof(uint32_t) != 0)
    {
        fprintf(stderr, "invalid SPIR-V size: %llu\n", (unsigned long long)size);
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    if (spv[0] != 0x07230203)
    {
        fprintf(stderr, "invalid SPIR-V magic: 0x%08x\n", spv[0]);
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    if (spv[1] != 0x00010600)
    {
        fprintf(stderr, "unexpected SPIR-V version: 0x%08x\n", spv[1]);
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }

    const uint32_t word_count = (uint32_t)(size / sizeof(uint32_t));
    for (uint32_t i = 5; i < word_count;)
    {
        const uint32_t instruction_words = spv[i] >> 16;
        const uint32_t opcode = spv[i] & 0xffff;
        if (instruction_words == 0 || instruction_words > word_count - i)
        {
            fprintf(stderr, "invalid SPIR-V instruction at word %u\n", i);
            dvz_shader_compile_result_destroy(&result);
            return 1;
        }
        /* OpDecorate <target> BuiltIn WorkgroupSize. */
        if (opcode == 71 && instruction_words >= 4 && spv[i + 2] == 11 && spv[i + 3] == 25)
        {
            fprintf(stderr, "deprecated SPIR-V WorkgroupSize built-in found\n");
            dvz_shader_compile_result_destroy(&result);
            return 1;
        }
        i += instruction_words;
    }

    dvz_shader_compile_result_destroy(&result);
    dvz_shader_compile_result_destroy(&result);

    static const char fragment[] = "#version 450\n"
                                   "layout(location = 0) out vec4 out_color;\n"
                                   "void main() { out_color = vec4(1.0); }\n";
    request.stage = DVZ_SHADER_STAGE_FRAGMENT;
    request.profile = DVZ_SHADER_PROFILE_GRAPHICS;
    request.source = fragment;
    request.source_size = sizeof(fragment) - 1;
    request.source_name = "shaderc_smoke.frag";
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_SUCCESS ||
        result.spirv_size < 5 * sizeof(uint32_t) || result.spirv[0] != 0x07230203 ||
        result.spirv[1] != 0x00010000)
    {
        fprintf(
            stderr, "fragment compilation failed: %s\n",
            result.diagnostics != NULL ? result.diagnostics : "no diagnostics");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    dvz_shader_compile_result_destroy(&result);

    static const char malformed[] = "#version 450\nvoid main() { this is not GLSL; }\n";
    request.stage = DVZ_SHADER_STAGE_VERTEX;
    request.profile = DVZ_SHADER_PROFILE_GRAPHICS;
    request.source = malformed;
    request.source_size = sizeof(malformed) - 1;
    request.source_name = "intentional_failure.vert";
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_FAILED ||
        result.diagnostics == NULL ||
        strstr(result.diagnostics, "intentional_failure.vert") == NULL)
    {
        fprintf(stderr, "malformed GLSL did not return filename-bearing diagnostics\n");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    dvz_shader_compile_result_destroy(&result);

    request.source_size = 0;
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_INVALID_REQUEST)
    {
        fprintf(stderr, "empty GLSL did not return invalid-request\n");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    dvz_shader_compile_result_destroy(&result);

    request.source = glsl;
    request.source_size = sizeof(glsl) - 1;
    request.stage = DVZ_SHADER_STAGE_COMPUTE;
    request.profile = DVZ_SHADER_PROFILE_GRAPHICS;
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_INVALID_REQUEST)
    {
        fprintf(stderr, "invalid stage/profile pair was accepted\n");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    dvz_shader_compile_result_destroy(&result);

    request.stage = DVZ_SHADER_STAGE_NONE;
    request.profile = DVZ_SHADER_PROFILE_GRAPHICS;
    if (dvz_shader_compile(&request, &result) != DVZ_SHADER_COMPILE_INVALID_REQUEST)
    {
        fprintf(stderr, "invalid shader stage was accepted\n");
        dvz_shader_compile_result_destroy(&result);
        return 1;
    }
    dvz_shader_compile_result_destroy(&result);

    uint64_t legacy_size = 0;
    uint32_t* legacy_spv = dvz_compile_glsl("compute", glsl, &legacy_size);
    if (legacy_spv == NULL || legacy_size == 0)
    {
        fprintf(stderr, "legacy GLSL convenience wrapper failed\n");
        dvz_free(legacy_spv);
        return 1;
    }
    dvz_free(legacy_spv);

    printf("source-build shaderc compute smoke produced %llu bytes\n", (unsigned long long)size);
    return 0;
}
