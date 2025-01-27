/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  SPIR-V compilation                                                                           */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "shader.h"
#include "_pointer.h"
#include "vklite.h"

#define BEGIN_IGNORE_STRICT_PROTOTYPES _Pragma("GCC diagnostic ignored \"-Wstrict-prototypes\"")
#define END_IGNORE_STRICT_PROTOTYPES   _Pragma("GCC diagnostic pop")

#if HAS_SHADERC
#include <shaderc/shaderc.h>
#endif



/*************************************************************************************************/
/*  Compilation                                                                                  */
/*************************************************************************************************/

VkShaderModule dvz_compile_glsl(DvzGpu* gpu, const char* code, VkShaderStageFlagBits stage)
{
    VkShaderModule module = {0};

#if HAS_SHADERC
    VkDevice device = gpu->device;
    ASSERT(device != VK_NULL_HANDLE);

    log_info("starting compilation of GLSL shader into SPIR-V");

    shaderc_shader_kind shader_kind;
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        shader_kind = shaderc_vertex_shader;
        break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        shader_kind = shaderc_fragment_shader;
        break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        shader_kind = shaderc_compute_shader;
        break;
    default:
        return VK_NULL_HANDLE;
    }

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, code, strlen(code), shader_kind, "shader.glsl", "main", options);

    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
    {
        log_error("error compiling the shader code: >>>%s<<<", code);
        shaderc_compiler_release(compiler);
        shaderc_compile_options_release(options);
        shaderc_result_release(result);
        return VK_NULL_HANDLE;
    }

    size_t size = shaderc_result_get_length(result);

    // NOTE: fixing this warning on macOS:
    // cast from 'const char *' to 'const uint32_t *' (aka 'const unsigned int *') increases
    // required alignment from 1 to 4 [-Wcast-align]
    // const uint32_t* spirv_code = (const uint32_t*)shaderc_result_get_bytes(result);
    const char* bytes = shaderc_result_get_bytes(result);
    uint32_t* spirv_code = malloc(size);
    if (spirv_code != NULL)
    {
        memcpy(spirv_code, bytes, size);
    }

    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    create_info.pCode = spirv_code;

    if (vkCreateShaderModule(device, &create_info, NULL, &module) != VK_SUCCESS)
    {
        log_error("error creating the shader module");
        shaderc_compiler_release(compiler);
        shaderc_compile_options_release(options);
        shaderc_result_release(result);
        return VK_NULL_HANDLE;
    }

    shaderc_compiler_release(compiler);
    shaderc_compile_options_release(options);
    shaderc_result_release(result);

#else
    log_error("unable to compile shader to SPIRV, Datoviz was not built with shaderc support");
#endif

    return module;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzShader
dvz_shader(DvzShaderFormat format, DvzShaderType type, DvzSize size, char* code, uint32_t* buffer)
{
    DvzShader shader = {0};

    shader.format = format;
    shader.type = type;
    shader.size = size;

    // NOTE: make a copy of the passed buffers, will be destroyed in dvz_shader_destroy()
    // called by pipelib.
    shader.buffer = buffer != NULL ? _cpy(size, buffer) : NULL;
    shader.code = code != NULL ? _cpy(size, code) : NULL;

    dvz_obj_init(&shader.obj);
    return shader;
}



void dvz_shader_destroy(DvzShader* shader)
{
    ANN(shader);
    dvz_obj_destroyed(&shader->obj);
    FREE(shader->code);
    FREE(shader->buffer);
    log_trace("shader destroyed");
}
