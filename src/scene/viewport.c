/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Viewport                                                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/viewport.h"
#include "_cglm.h"



/*************************************************************************************************/
/*  Default viewport                                                                             */
/*************************************************************************************************/

DvzViewport dvz_viewport(vec2 offset, vec2 shape, int flags)
{
    DvzViewport viewport = {0};

    viewport.viewport.x = offset[0];
    viewport.viewport.y = offset[1];
    viewport.viewport.minDepth = +0;
    viewport.viewport.maxDepth = +1;

    viewport.size_framebuffer[0] = viewport.viewport.width = (float)shape[0];
    viewport.size_framebuffer[1] = viewport.viewport.height = (float)shape[1];
    viewport.size_screen[0] = viewport.size_framebuffer[0];
    viewport.size_screen[1] = viewport.size_framebuffer[1];

    viewport.flags = flags;

    return viewport;
}



void dvz_viewport_margins(DvzViewport* viewport, vec4 margins)
{
    ANN(viewport);
    glm_vec4_copy(margins, viewport->margins);
}



DvzViewport dvz_viewport_default(uint32_t width, uint32_t height)
{
    return dvz_viewport((vec2){0, 0}, (vec2){width, height}, 0);
}
