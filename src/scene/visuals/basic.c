/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Basic                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/basic.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DEFAULT_SIZE 1.0



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_basic(DvzBatch* batch, DvzPrimitiveTopology topology, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, topology, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzBasicVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzBasicVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzBasicVertex, group), DVZ_FORMAT_R32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzBasicVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzBasicParams));
    dvz_params_attr(params, 0, FIELD(DvzBasicParams, size));

    // Default params.
    dvz_visual_param(visual, 2, 0, (float[]){DEFAULT_SIZE});

    return visual;
}



void dvz_basic_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the visual visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_basic_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_basic_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_basic_group(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_basic_size(DvzVisual* visual, float size)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, &size);
}
