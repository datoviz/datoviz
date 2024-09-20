/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  SPIR-V compilation                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SPIRV
#define DVZ_HEADER_SPIRV



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include <vulkan/vulkan.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGpu DvzGpu;
typedef struct DvzShader DvzShader;



/*************************************************************************************************/
/*  Structures                                                                                   */
/*************************************************************************************************/

struct DvzShader
{
    DvzObject obj; // used to hold the id in the mapping structure

    DvzShaderFormat format;
    DvzShaderType type;
    DvzSize size;

    char* code;       // only for GLSL obj_type
    uint32_t* buffer; // only for SPIRV obj_type
};



/*************************************************************************************************/
/*  Compilation                                                                                  */
/*************************************************************************************************/

VkShaderModule dvz_compile_glsl(DvzGpu* gpu, const char* code, VkShaderStageFlagBits stage);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzShader
dvz_shader(DvzShaderFormat format, DvzShaderType type, DvzSize size, char* code, uint32_t* buffer);


void dvz_shader_destroy(DvzShader* shader);



#endif
