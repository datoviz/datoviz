/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/marker.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/scene.h"
#include "scene/texture.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions */
/*************************************************************************************************/

DvzVisual* dvz_marker(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_marker");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzMarkerVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzMarkerVertex, size), DVZ_FORMAT_R32_SFLOAT, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzMarkerVertex, angle), DVZ_FORMAT_R32_SFLOAT, 0);
    dvz_visual_attr(visual, 3, FIELD(DvzMarkerVertex, color), DVZ_FORMAT_COLOR, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzMarkerVertex));

    // Uniforms.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzMarkerParams));
    dvz_params_attr(params, 0, FIELD(DvzMarkerParams, edgecolor));
    dvz_params_attr(params, 1, FIELD(DvzMarkerParams, linewidth));
    dvz_params_attr(params, 2, FIELD(DvzMarkerParams, tex_scale));

    // Default texture to avoid Vulkan warning with unbound texture slot.
    dvz_visual_tex(
        visual, 3, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID, DVZ_ZERO_OFFSET);

    // Default specialization constant values.
    // Specialization constant #0: mode.
    // Specialization constant #1: aspect.
    // Specialization constant #2: shape.
    dvz_marker_mode(visual, DVZ_MARKER_MODE_CODE);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_DISC);

    return visual;
}



void dvz_marker_mode(DvzVisual* visual, DvzMarkerMode mode)
{
    ANN(visual);
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int32_t), (int32_t[]){(int32_t)mode});
}



void dvz_marker_aspect(DvzVisual* visual, DvzMarkerAspect aspect)
{
    ANN(visual);
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int32_t), (int32_t[]){(int32_t)aspect});
}



void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape)
{
    ANN(visual);
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int32_t), (int32_t[]){(int32_t)shape});
}



void dvz_marker_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the marker visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_marker_position(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_marker_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_marker_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_marker_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_marker_edgecolor(DvzVisual* visual, DvzColor color)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, (vec4){COLOR_D2F(color)});
}



void dvz_marker_linewidth(DvzVisual* visual, float width)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, &width);
}



void dvz_marker_tex_scale(DvzVisual* visual, float scale)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 2, &scale);
}



void dvz_marker_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, 3, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);
}
