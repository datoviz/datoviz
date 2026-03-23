/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Rendering internals                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan_core.h>

#include "datoviz/vklite/rendering.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRendering
{
    VkRenderingInfo info;
    DvzAttachment attachments[DVZ_MAX_ATTACHMENTS];
    DvzAttachment depth;
    DvzAttachment stencil;
};
