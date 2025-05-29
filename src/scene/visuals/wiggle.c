/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Wiggle                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/wiggle.h"
#include "../src/resources_utils.h"
#include "_map.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/texture.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);
    ASSERT(count > 0);
    dvz_visual_instance(visual, canvas, 6 * first, 0, 6 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_wiggle(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_wiggle");

    // Vertex attributes.
    int af = DVZ_ATTR_FLAGS_REPEAT_X6;
    dvz_visual_attr(visual, 0, FIELD(DvzWiggleVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 1, FIELD(DvzWiggleVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzWiggleVertex));

    // Slots.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);


    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzWiggleParams));
    dvz_params_attr(
        params, DVZ_WIGGLE_PARAMS_NEGATIVE_COLOR, FIELD(DvzWiggleParams, negative_color));
    dvz_params_attr(
        params, DVZ_WIGGLE_PARAMS_POSITIVE_COLOR, FIELD(DvzWiggleParams, positive_color));
    dvz_params_attr(params, DVZ_WIGGLE_PARAMS_EDGECOLOR, FIELD(DvzWiggleParams, edgecolor));
    dvz_params_attr(params, DVZ_WIGGLE_PARAMS_XRANGE, FIELD(DvzWiggleParams, xrange));
    dvz_params_attr(params, DVZ_WIGGLE_PARAMS_CHANNELS, FIELD(DvzWiggleParams, channels));
    dvz_params_attr(params, DVZ_WIGGLE_PARAMS_SCALE, FIELD(DvzWiggleParams, scale));

    // Default permutation.
    dvz_wiggle_xrange(visual, (vec2){0, 1});
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_SCALE, (float[]){1});

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    // Default position.
    dvz_visual_alloc(visual, 1, 6, 0);
    dvz_wiggle_bounds(visual, (vec2){-1, +1}, (vec2){-1, +1});

    // uv texture coordinates for the quad.
    dvz_visual_quads(visual, 1, 0, 1, (vec4[]){{0, 0, 1, 1}});

    return visual;
}



void dvz_wiggle_bounds(DvzVisual* visual, vec2 xlim, vec2 ylim)
{
    ANN(visual);
    // Position
    dvz_visual_quads(visual, 0, 0, 1, (vec4[]){{xlim[0], ylim[0], xlim[1], ylim[1]}});
}



void dvz_wiggle_color(DvzVisual* visual, DvzColor negative_color, DvzColor positive_color)
{
    ANN(visual);
    // NOTE: convert from cvec4 into vec4 as GLSL uniforms do not support cvec4
    dvz_visual_param(
        visual, 2, DVZ_WIGGLE_PARAMS_NEGATIVE_COLOR, (vec4){COLOR_D2F(negative_color)});
    dvz_visual_param(
        visual, 2, DVZ_WIGGLE_PARAMS_POSITIVE_COLOR, (vec4){COLOR_D2F(positive_color)});
}



void dvz_wiggle_edgecolor(DvzVisual* visual, DvzColor edgecolor)
{
    ANN(visual);
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_EDGECOLOR, (vec4){COLOR_D2F(edgecolor)});
}



void dvz_wiggle_xrange(DvzVisual* visual, vec2 xrange)
{
    ANN(visual);
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_XRANGE, xrange);
}



void dvz_wiggle_scale(DvzVisual* visual, float scale)
{
    ANN(visual);
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_SCALE, &scale);
}



void dvz_wiggle_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, 3, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);

    // Number of channels is the texture height (width is number of samples).
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_CHANNELS, &texture->shape[1]);
}
