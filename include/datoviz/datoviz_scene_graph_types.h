/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz_enums.h"
#include "datoviz_keycodes.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMVP DvzMVP;
typedef struct DvzViewport DvzViewport;
typedef struct _VkViewport _VkViewport;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;

    // float time;
};



// NOTE: this corresponds to VkViewport, but we want to avoid the inclusion of vklite.h here.
struct _VkViewport
{
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

// NOTE: must correspond to the shader structure in common.glsl
struct DvzViewport
{
    _VkViewport viewport; // Vulkan viewport
    vec4 margins;

    // Position and size of the viewport in screen coordinates.
    uvec2 offset_screen;
    uvec2 size_screen;

    // Position and size of the viewport in framebuffer coordinates.
    uvec2 offset_framebuffer;
    uvec2 size_framebuffer;

    // NOTE: obsolete?
    int flags;
    // TODO: aspect ratio
};
