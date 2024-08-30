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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VkShaderModule dvz_shader_compile(DvzGpu* gpu, const char* code, VkShaderStageFlagBits stage);



#endif
