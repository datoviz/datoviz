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

// #include "spirv.h"
#include "vklite.h"

#define BEGIN_IGNORE_STRICT_PROTOTYPES _Pragma("GCC diagnostic ignored \"-Wstrict-prototypes\"")
#define END_IGNORE_STRICT_PROTOTYPES   _Pragma("GCC diagnostic pop")

#if HAS_SHADERC
#include <shaderc/shaderc.h>
#endif



/*************************************************************************************************/
/*  Functions */
/*************************************************************************************************/

VkShaderModule dvz_shader_compile(DvzGpu* gpu, const char* code, VkShaderStageFlagBits stage)
{
    VkShaderModule module = {0};

#if HAS_SHADERC
    VkDevice device = gpu->device;
    ASSERT(device != VK_NULL_HANDLE);

    log_trace("starting compilation of GLSL shader into SPIR-V");

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
        shaderc_compiler_release(compiler);
        shaderc_compile_options_release(options);
        shaderc_result_release(result);
        return VK_NULL_HANDLE;
    }

    size_t size = shaderc_result_get_length(result);
    const uint32_t* spirv_code = (const uint32_t*)shaderc_result_get_bytes(result);

    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    create_info.pCode = spirv_code;

    if (vkCreateShaderModule(device, &create_info, NULL, &module) != VK_SUCCESS)
    {
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
