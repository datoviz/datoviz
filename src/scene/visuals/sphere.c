/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Sphere                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/sphere.h"
#include "datoviz.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "request.h"
#include "scene/graphics.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_sphere(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_sphere");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzSphereVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzSphereVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzSphereVertex, size), DVZ_FORMAT_R32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzSphereVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzSphereParams));
    dvz_params_attr(params, 0, FIELD(DvzSphereParams, light_pos));
    dvz_params_attr(params, 1, FIELD(DvzSphereParams, light_param));

    return visual;
}



void dvz_sphere_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the sphere visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, data);
}



void dvz_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, data);
}



void dvz_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, data);
}



void dvz_sphere_light_pos(DvzVisual* visual, vec3 pos)
{
    ANN(visual);
    vec4 pos_ = {pos[0], pos[1], pos[2], 0};
    dvz_visual_param(visual, 2, 0, pos_);
}



void dvz_sphere_light_params(DvzVisual* visual, vec4 params)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, params);
}
