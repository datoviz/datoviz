/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/pixel.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/viewset.h"
#include "scene/visual.h"
#include "scene/visuals/basic.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_pixel(DvzBatch* batch, int flags)
{
    return dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
}



void dvz_pixel_alloc(DvzVisual* pixel, uint32_t item_count) { dvz_basic_alloc(pixel, item_count); }



void dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags)
{
    dvz_basic_position(pixel, first, count, values, flags);
}



void dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    dvz_basic_color(pixel, first, count, values, flags);
}



void dvz_pixel_size(DvzVisual* pixel, float size)
{
    dvz_basic_size(pixel, size); //
}
