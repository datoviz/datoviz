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
#include "_cglm.h"
#include "_map.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/array.h"
#include "scene/baker.h"
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



static void _quads(DvzVisual* visual, uint32_t attr_idx, vec4 tl_br, uint32_t nch)
{
    uint32_t first = 0, count = 1;

    // Quad triangulation with 3 triangles = 6 vertices.
    float* positions = (float*)calloc(6 * count, nch * sizeof(float));
    float x0 = tl_br[0], y0 = tl_br[1];
    float x1 = tl_br[2], y1 = tl_br[3];
    for (uint32_t i = 0; i < count; i++)
    {
        positions[6 * nch * i + 0 * nch + 0] = x0; // top left
        positions[6 * nch * i + 0 * nch + 1] = y0;

        positions[6 * nch * i + 1 * nch + 0] = x0; // bottom left
        positions[6 * nch * i + 1 * nch + 1] = y1;

        positions[6 * nch * i + 2 * nch + 0] = x1; // bottom right
        positions[6 * nch * i + 2 * nch + 1] = y1;

        positions[6 * nch * i + 3 * nch + 0] = x1; // bottom right
        positions[6 * nch * i + 3 * nch + 1] = y1;

        positions[6 * nch * i + 4 * nch + 0] = x1; // top right
        positions[6 * nch * i + 4 * nch + 1] = y0;

        positions[6 * nch * i + 5 * nch + 0] = x0; // top left
        positions[6 * nch * i + 5 * nch + 1] = y0;
    }

    dvz_visual_data(visual, attr_idx, 6 * first, 6 * count, (void*)positions);
    FREE(positions);
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
    dvz_visual_attr(visual, 0, FIELD(DvzWiggleVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzWiggleVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzWiggleVertex));

    // Slots.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT); // params
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX); // texture


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

    // Default position.
    dvz_visual_alloc(visual, 1, 6, 0);

    // Default bounds.
    dvz_wiggle_bounds(visual, (vec2){-1, +1}, (vec2){-1, +1});

    // uv texture coordinates for the quad.
    _quads(visual, 1, (vec4){0, 0, 1, 1}, 2);

    // DEBUG
    // DvzWiggleVertex* vertices =
    //     (DvzWiggleVertex*)visual->baker->vertex_bindings[0].dual.array->data;
    // for (uint32_t i = 0; i < 6; i++)
    // {
    //     printf("pos: ");
    //     glm_vec3_print(vertices[i].pos, stdout);
    //     printf("uv : ");
    //     glm_vec2_print(vertices[i].uv, stdout);
    // }

    // Default range.

    // Default texture scale.
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_SCALE, (float[]){1});

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_wiggle_bounds(DvzVisual* visual, vec2 xlim, vec2 ylim)
{
    ANN(visual);
    // Position
    _quads(visual, 0, (vec4){xlim[0], ylim[1], xlim[1], ylim[0]}, 3);
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

    uint32_t channels = texture->shape[1]; // texture height

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, 3, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);

    // Number of channels is the texture height (width is number of samples).
    dvz_visual_param(visual, 2, DVZ_WIGGLE_PARAMS_CHANNELS, &channels);

    float m = 2.0 / channels;
    dvz_wiggle_xrange(visual, (vec2){m, 1 - m});
}
