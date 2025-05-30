/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Slice                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/slice.h"
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
#include "scene/visuals/volume.h"



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

DvzVisual* dvz_slice(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_slice");

    // Depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE);

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzSliceVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzSliceVertex, uvw), DVZ_FORMAT_R32G32B32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzSliceVertex));

    // Volume specialization constants.
    volume_specialization(visual);

    // Slots.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzSliceParams));

    dvz_params_attr(params, 0, FIELD(DvzSliceParams, alpha));

    dvz_visual_param(visual, 2, 0, (float[]){1}); // alpha

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_slice_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the slice visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, 6 * item_count, 0);
}



void dvz_slice_position(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags)
{
    ANN(visual);
    // 0 - 3
    // |   |
    // 1 - 2
    //
    // 0 1 2 - 2 3 0

    // Quad triangulation with 3 triangles = 6 vertices.
    vec3* positions = (vec3*)calloc(6 * count, sizeof(vec3));
    float x0 = 0, y0 = 0, z0 = 0;
    float x1 = 0, y1 = 0, z1 = 0;
    float x2 = 0, y2 = 0, z2 = 0;
    float x3 = 0, y3 = 0, z3 = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        x0 = p0[i][0];
        y0 = p0[i][1];
        z0 = p0[i][2];

        x1 = p1[i][0];
        y1 = p1[i][1];
        z1 = p1[i][2];

        x2 = p2[i][0];
        y2 = p2[i][1];
        z2 = p2[i][2];

        x3 = p3[i][0];
        y3 = p3[i][1];
        z3 = p3[i][2];

        positions[6 * i + 0][0] = x0; // top left
        positions[6 * i + 0][1] = y0;
        positions[6 * i + 0][2] = z0;

        positions[6 * i + 1][0] = x1; // bottom left
        positions[6 * i + 1][1] = y1;
        positions[6 * i + 1][2] = z1;

        positions[6 * i + 2][0] = x2; // bottom right
        positions[6 * i + 2][1] = y2;
        positions[6 * i + 2][2] = z2;

        positions[6 * i + 3][0] = x2; // bottom right
        positions[6 * i + 3][1] = y2;
        positions[6 * i + 3][2] = z2;

        positions[6 * i + 4][0] = x3; // top right
        positions[6 * i + 4][1] = y3;
        positions[6 * i + 4][2] = z3;

        positions[6 * i + 5][0] = x0; // top left
        positions[6 * i + 5][1] = y0;
        positions[6 * i + 5][2] = z0;
    }

    dvz_visual_data(visual, 0, 6 * first, 6 * count, (void*)positions);
    FREE(positions);
}



void dvz_slice_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags)
{
    ANN(visual);
    // 0 - 3
    // |   |
    // 1 - 2
    //
    // 0 1 2 - 2 3 0

    // Quad triangulation with 3 triangles = 6 vertices.
    vec3* uvw = (vec3*)calloc(6 * count, sizeof(vec3));
    float u0 = 0, v0 = 0, w0 = 0;
    float u1 = 0, v1 = 0, w1 = 0;
    float u2 = 0, v2 = 0, w2 = 0;
    float u3 = 0, v3 = 0, w3 = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        u0 = uvw0[i][0];
        v0 = uvw0[i][1];
        w0 = uvw0[i][2];

        u1 = uvw1[i][0];
        v1 = uvw1[i][1];
        w1 = uvw1[i][2];

        u2 = uvw2[i][0];
        v2 = uvw2[i][1];
        w2 = uvw2[i][2];

        u3 = uvw3[i][0];
        v3 = uvw3[i][1];
        w3 = uvw3[i][2];

        uvw[6 * i + 0][0] = u0; // top left
        uvw[6 * i + 0][1] = v0;
        uvw[6 * i + 0][2] = w0;

        uvw[6 * i + 1][0] = u1; // bottom left
        uvw[6 * i + 1][1] = v1;
        uvw[6 * i + 1][2] = w1;

        uvw[6 * i + 2][0] = u2; // bottom right
        uvw[6 * i + 2][1] = v2;
        uvw[6 * i + 2][2] = w2;

        uvw[6 * i + 3][0] = u2; // bottom right
        uvw[6 * i + 3][1] = v2;
        uvw[6 * i + 3][2] = w2;

        uvw[6 * i + 4][0] = u3; // top right
        uvw[6 * i + 4][1] = v3;
        uvw[6 * i + 4][2] = w3;

        uvw[6 * i + 5][0] = u0; // top left
        uvw[6 * i + 5][1] = v0;
        uvw[6 * i + 5][2] = w0;
    }

    dvz_visual_data(visual, 1, 6 * first, 6 * count, (void*)uvw);
    FREE(uvw);
}



void dvz_slice_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, 3, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);
}



void dvz_slice_alpha(DvzVisual* visual, float alpha)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, (float[]){alpha});
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth, void* data)
{
    ANN(batch);
    ANN(data);
    ASSERT(width > 0);
    ASSERT(height > 0);

    uvec3 shape = {width, height, depth};
    DvzSize size = width * height * depth * _format_size(format);
    DvzId tex = dvz_create_tex(batch, DVZ_TEX_3D, format, shape, 0).id;
    dvz_upload_tex(batch, tex, DVZ_ZERO_OFFSET, shape, size, data, 0);

    return tex;
}
