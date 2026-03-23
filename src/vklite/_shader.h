/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Shader internals                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/obj.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vklite/shader.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzShader
{
    DvzObject obj; // used to hold the id in the mapping structure
    DvzDevice* device;
    DvzShaderType type;
    DvzSize size;
    VkShaderModule vk_shader;
    uint32_t* buffer; // only for SPIRV obj_type
};
