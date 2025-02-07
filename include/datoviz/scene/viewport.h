/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Viewport                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VIEWPORT
#define DVZ_HEADER_VIEWPORT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

static void dvz_viewport_print(DvzViewport* viewport)
{
    printf(
        "viewport: screen %dx%d, fb %dx%d, margins %.0f %.0f %.0f %.0f\n", //
        viewport->size_screen[0], viewport->size_screen[1],                //
        viewport->size_framebuffer[0], viewport->size_framebuffer[1],      //
        viewport->margins[0], viewport->margins[1], viewport->margins[2], viewport->margins[3]);
}



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a viewport.
 *
 * @param offset the viewport's offset
 * @param shape the viewport's shape
 * @returns the viewport
 */
DvzViewport dvz_viewport(vec2 offset, vec2 shape, int flags);



/**
 */
void dvz_viewport_margins(DvzViewport* viewport, vec4 margins);



EXTERN_C_OFF


#endif
