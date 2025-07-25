/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Path                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/path.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
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
    dvz_visual_instance(visual, canvas, 4 * first, 0, 4 * count, first_instance, instance_count);
}



static void* repeat_and_shift(DvzSize item_size, uint32_t count, const void* values)
{
    uint32_t new_count = 4 * count;
    void* result = calloc(new_count, item_size);
    if (!result)
    {
        return NULL;
    }

    const uint8_t* src = (const uint8_t*)values;
    uint8_t* dst = (uint8_t*)result;

    // handle the first two items (copy first original item twice)
    memcpy(dst + 0 * item_size, src, item_size);
    memcpy(dst + 1 * item_size, src, item_size);

    // repeat each item 4 times starting from index 2
    for (uint32_t i = 1; i < count; i++)
        for (uint32_t j = 0; j < 4; j++)
        {
            {
                memcpy(dst + ((2 + (i - 1) * 4 + j) * item_size), src + i * item_size, item_size);
            }
        }

    // last two items (copy first original item)
    memcpy(dst + (new_count - 2) * item_size, src, item_size);
    memcpy(dst + (new_count - 1) * item_size, src, item_size);

    return result;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_path(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, flags);
    // DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_path");

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzPathVertex));

    // Vertex attributes.
    int attr_flag = DVZ_ATTR_FLAGS_REPEAT_X4;

    dvz_visual_attr(visual, 0, FIELD(DvzPathVertex, p0), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flag);
    dvz_visual_attr(visual, 1, FIELD(DvzPathVertex, p1), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flag);
    dvz_visual_attr(visual, 2, FIELD(DvzPathVertex, p2), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flag);
    dvz_visual_attr(visual, 3, FIELD(DvzPathVertex, p3), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flag);
    dvz_visual_attr(visual, 4, FIELD(DvzPathVertex, color), DVZ_FORMAT_COLOR, 0);
    dvz_visual_attr(visual, 5, FIELD(DvzPathVertex, linewidth), DVZ_FORMAT_R32_SFLOAT, 0);

    // Uniforms.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzPathParams));
    // dvz_params_attr(params, 0, FIELD(DvzPathParams, linewidth));
    dvz_params_attr(params, 0, FIELD(DvzPathParams, miter_limit));
    dvz_params_attr(params, 1, FIELD(DvzPathParams, cap_type));
    dvz_params_attr(params, 2, FIELD(DvzPathParams, round_join));

    // Default params.
    bool closed = (visual->flags & DVZ_PATH_FLAGS_CLOSED) > 0;
    // dvz_visual_param(visual, 2, 0, (float[]){10.0});
    dvz_visual_param(visual, 2, 0, (float[]){4.0}); // miter_limit
    dvz_visual_param(visual, 2, 1, (int32_t[]){closed ? DVZ_CAP_NONE : DVZ_CAP_ROUND}); // cap_type
    dvz_visual_param(visual, 2, 2, (int32_t[]){DVZ_JOIN_ROUND}); // round_join

    return visual;
}



void dvz_path_alloc(DvzVisual* visual, uint32_t total_point_count)
{
    ANN(visual);
    log_debug("allocating the path visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Allocate the visual.
    dvz_visual_alloc(visual, total_point_count, 4 * total_point_count, 0);
}



void dvz_path_position(
    DvzVisual* visual, uint32_t first, uint32_t point_count, vec3* positions, //
    uint32_t path_count, uint32_t* path_lengths, int flags)
{
    ANN(visual);
    ANN(positions);
    ASSERT(point_count > 0);

    bool closed = (visual->flags & DVZ_PATH_FLAGS_CLOSED) > 0;

    uint32_t path_lengths_1[1] = {point_count};
    if (path_count <= 1)
    {
        path_count = 1;
        path_lengths = path_lengths_1;
    }

    // Compute the total number of vertices, which is the sum of all path lengths.
    uint32_t total_length = 0;
    int32_t l = 0;
    for (uint32_t i = 0; i < path_count; i++)
    {
        l = (int32_t)path_lengths[i];
        total_length += (uint32_t)l;
    }

    uint32_t k = 0;
    uint32_t src_offset = 0;
    int32_t i0 = 0, i1 = 0, i2 = 0, i3 = 0;
    vec3* p0 = (vec3*)calloc(total_length, sizeof(vec3));
    vec3* p1 = (vec3*)calloc(total_length, sizeof(vec3));
    vec3* p2 = (vec3*)calloc(total_length, sizeof(vec3));
    vec3* p3 = (vec3*)calloc(total_length, sizeof(vec3));
    for (uint32_t j = 0; j < path_count; j++)
    {
        l = (int32_t)path_lengths[j];
        for (int32_t i = 0; i < l; i++)
        {
            i0 = i - 1;
            i1 = i + 0;
            i2 = i + 1;
            i3 = i + 2;

            if (!closed)
            {
                i0 = MAX(i0, 0);
                i2 = MIN(i2, l - 1);
                i3 = MIN(i3, l - 1);
            }
            else
            {
                i0 = i0 < 0 ? i0 + l : i0;
                i2 = i2 >= l ? i2 - l : i2;
                i3 = i3 >= l ? i3 - l : i3;
            }

            ASSERT(0 <= i0 && i0 < l);
            ASSERT(0 <= i1 && i1 < l);
            ASSERT(0 <= i2 && i2 < l);
            ASSERT(0 <= i3 && i3 < l);

            _vec3_copy(positions[src_offset + (uint32_t)i0], p0[k]);
            _vec3_copy(positions[src_offset + (uint32_t)i1], p1[k]);
            _vec3_copy(positions[src_offset + (uint32_t)i2], p2[k]);
            _vec3_copy(positions[src_offset + (uint32_t)i3], p3[k]);

            k++;
        }
        src_offset += (uint32_t)l;
    }
    ASSERT(k == total_length);

    // NOTE: we did not use REPEAT attr flag for position as we do the repeat manually with a
    // shift.
    dvz_visual_data(visual, 0, first, total_length, (void*)p0);
    dvz_visual_data(visual, 1, first, total_length, (void*)p1);
    dvz_visual_data(visual, 2, first, total_length, (void*)p2);
    dvz_visual_data(visual, 3, first, total_length, (void*)p3);

    FREE(p0);
    FREE(p1);
    FREE(p2);
    FREE(p3);
}



void dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    ANN(visual);
    void* reps = repeat_and_shift(sizeof(DvzColor), count, values);
    dvz_visual_data(visual, 4, 4 * first, 4 * count, (void*)reps);
    FREE(reps);
}



void dvz_path_linewidth(
    DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    void* reps = repeat_and_shift(sizeof(float), count, values);
    dvz_visual_data(visual, 5, 4 * first, 4 * count, (void*)reps);
    FREE(reps);
}



void dvz_path_cap(DvzVisual* visual, DvzCapType cap)
{
    ANN(visual);
    // NOTE: this is safe because a copy is made immediately.
    dvz_visual_param(visual, 2, 1, (int32_t[]){(int32_t)cap});
}



void dvz_path_join(DvzVisual* visual, DvzJoinType join)
{
    ANN(visual);
    // NOTE: this is safe because a copy is made immediately.
    dvz_visual_param(visual, 2, 2, (int32_t[]){(int32_t)join});
}
