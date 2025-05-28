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
    dvz_visual_attr(visual, 1, FIELD(DvzBasicVertex, color), DVZ_FORMAT_COLOR, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzBasicVertex, group), DVZ_FORMAT_R32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzBasicVertex));

    // Slots.
    _common_setup(visual);
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



void dvz_basic_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
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



DvzVisual* dvz_basic_shape(DvzBatch* batch, DvzShape* shape, int flags)
{
    ANN(batch);
    ANN(shape);
    ANN(shape->pos);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    // NOTE: set the visual flag to indexed or non-indexed (default) depending on whether the shape
    // has an index buffer or not.
    flags |= (index_count > 0 ? DVZ_VISUAL_FLAGS_INDEXED : DVZ_VISUAL_FLAGS_DEFAULT);
    DvzVisual* visual = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);

    dvz_visual_alloc(visual, vertex_count, vertex_count, index_count);
    dvz_basic_position(visual, 0, vertex_count, shape->pos, 0);
    if (shape->color != NULL)
    {
        dvz_basic_color(visual, 0, vertex_count, shape->color, 0);
    }

    return visual;
}
